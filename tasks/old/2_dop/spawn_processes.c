#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
    int rank, size, universe_size, parent_rank = -1;
    MPI_Comm parent_comm, intercomm;
    char child_app[] = "hello_world";
    char *spawn_args[] = { NULL };
    int child_processes = 4;
    int errors[4];
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Get_processor_name(processor_name, &name_len);

    // Проверяем, был ли процесс создан с помощью MPI_Comm_spawn
    MPI_Comm_get_parent(&parent_comm);
    if (parent_comm == MPI_COMM_NULL) {
        // Родительский процесс
        // Проверяем доступность ресурсов для запуска новых процессов
        MPI_Attr_get(MPI_COMM_WORLD, MPI_UNIVERSE_SIZE, &universe_size, &parent_rank);
        
        if (universe_size == MPI_UNDEFINED || universe_size < size * child_processes) {
            printf("Внимание: MPI_UNIVERSE_SIZE не определен или недостаточно ресурсов.\n");
            printf("Требуется минимум %d процессов, доступно: %d\n", 
                   size * child_processes, universe_size);
            universe_size = size * child_processes; // Предполагаем, что ресурсы доступны
        }

        printf("Родительский процесс %d/%d на узле %s запускает %d дочерних процесса\n", 
               rank, size, processor_name, child_processes);
        
        // Каждый процесс запускает child_processes новых процессов
        MPI_Comm_spawn(child_app, spawn_args, child_processes, 
                       MPI_INFO_NULL, 0, MPI_COMM_WORLD, &intercomm, errors);
        
        // Проверка на ошибки при запуске
        for (int i = 0; i < child_processes; i++) {
            if (errors[i] != MPI_SUCCESS) {
                printf("Ошибка при запуске процесса %d: %d\n", i, errors[i]);
            }
        }

        // Круговая пересылка: родитель отправляет свой ранг первому дочернему процессу
        if (rank == 0) {
            MPI_Send(&rank, 1, MPI_INT, 0, 0, intercomm);
            printf("Родитель %d: отправил данные дочернему процессу 0\n", rank);
        }

        // Получение сообщения от последнего дочернего процесса для закрытия круга
        if (rank == size - 1) {
            int message;
            MPI_Recv(&message, 1, MPI_INT, child_processes - 1, 0, intercomm, MPI_STATUS_IGNORE);
            printf("Родитель %d: получил сообщение %d от последнего дочернего процесса\n", 
                   rank, message);
        }
    } else {
        // Дочерний процесс
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &size);
        
        printf("Дочерний процесс %d/%d запущен на узле %s\n", rank, size, processor_name);
        
        // Круговая пересылка между дочерними процессами
        int message;
        if (rank == 0) {
            // Первый дочерний процесс получает сообщение от родителя
            MPI_Recv(&message, 1, MPI_INT, 0, 0, parent_comm, MPI_STATUS_IGNORE);
            message++; // Увеличиваем счетчик
            printf("Дочерний %d: получил %d от родителя, отправляю %d следующему\n", 
                   rank, message-1, message);
            MPI_Send(&message, 1, MPI_INT, (rank + 1) % size, 0, MPI_COMM_WORLD);
        } else if (rank == size - 1) {
            // Последний дочерний процесс отправляет сообщение обратно родителю
            MPI_Recv(&message, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            message++; // Увеличиваем счетчик
            printf("Дочерний %d: получил %d от предыдущего, отправляю %d родителю\n", 
                   rank, message-1, message);
            MPI_Send(&message, 1, MPI_INT, size - 1, 0, parent_comm);
        } else {
            // Промежуточные процессы передают сообщение дальше
            MPI_Recv(&message, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            message++; // Увеличиваем счетчик
            printf("Дочерний %d: получил %d от предыдущего, отправляю %d следующему\n", 
                   rank, message-1, message);
            MPI_Send(&message, 1, MPI_INT, (rank + 1) % size, 0, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();
    return 0;
}