#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
    int rank, size;
    int children_per_process = 4;  // Каждый процесс запускает 4 новых процесса
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    printf("Parent process %d of %d started\n", rank, size);
    
    // Проверка доступности MPI_Comm_spawn
    int *universe_size = NULL;
    int flag;
    MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_UNIVERSE_SIZE, &universe_size, &flag);
    
    if (!flag || *universe_size == MPI_UNDEFINED) {
        printf("Warning: MPI_UNIVERSE_SIZE not set or supported\n");
    } else {
        printf("MPI_UNIVERSE_SIZE = %d\n", *universe_size);
        
        // Проверка, достаточно ли ресурсов для запуска всех процессов
        if (*universe_size < size * children_per_process) {
            printf("Warning: Not enough resources available for all processes. "
                   "Need %d processes, but only %d are available.\n", 
                   size * children_per_process, *universe_size);
            
            // Корректировка количества запускаемых процессов
            children_per_process = (*universe_size) / size;
            if (children_per_process == 0) {
                printf("Error: Cannot spawn any child processes.\n");
                MPI_Finalize();
                return 1;
            }
            printf("Adjusted children_per_process to %d\n", children_per_process);
        }
    }
    
    // Каждый процесс запускает по 4 (или скорректированное количество) новых процессов
    MPI_Comm intercomm;
    char child_program[] = "./child";
    char *args[] = {NULL};
    int errors[children_per_process];
    
    MPI_Info info;
    MPI_Info_create(&info);
    
    // Запуск дочерних процессов
    int err = MPI_Comm_spawn(child_program, args, children_per_process, 
                           info, 0, MPI_COMM_WORLD, &intercomm, errors);
    
    if (err != MPI_SUCCESS) {
        printf("Error spawning child processes: %d\n", err);
        MPI_Finalize();
        return 1;
    }
    
    // Проверяем ошибки при запуске дочерних процессов
    for (int i = 0; i < children_per_process; i++) {
        if (errors[i] != MPI_SUCCESS) {
            printf("Error spawning child process %d: %d\n", i, errors[i]);
        }
    }
    
    // Получаем размер и ранг интеркоммуникатора для родительской части
    int intercomm_size;
    MPI_Comm_remote_size(intercomm, &intercomm_size);
    printf("Parent process %d: Spawned %d child processes\n", rank, intercomm_size);
    
    // Ожидаем сообщения от дочерних процессов об успешном завершении
    // круговой пересылки данных
    int result;
    MPI_Status status;
    
    // Получаем сообщение от первого дочернего процесса
    MPI_Recv(&result, 1, MPI_INT, 0, 0, intercomm, &status);
    printf("Parent process %d: Received confirmation from child process: %d\n", rank, result);
    
    // Завершаем работу
    MPI_Info_free(&info);
    MPI_Finalize();
    return 0;
}