#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char **argv) {
    int rank, size;
    MPI_File fh;
    MPI_Status status;
    char filename[] = "ranks.txt";
    int *all_ranks = NULL;
    int *rank_buffer = NULL;
    int i;

    // Инициализация MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Создаем буфер для хранения всех рангов
    if (rank == 0) {
        all_ranks = (int *)malloc(size * sizeof(int));
    }

    // Сбор всех рангов на процессе с rank = 0
    MPI_Gather(&rank, 1, MPI_INT, all_ranks, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Сортировка рангов в обратном порядке (только на процессе с rank = 0)
    if (rank == 0) {
        // Создаем буфер для отсортированных рангов
        rank_buffer = (int *)malloc(size * sizeof(int));
        
        // Заполняем буфер рангами в обратном порядке (от N-1 до 0)
        for (i = 0; i < size; i++) {
            rank_buffer[i] = size - 1 - i;
        }
        
        printf("Ранги будут записаны в порядке: ");
        for (i = 0; i < size; i++) {
            printf("%d ", rank_buffer[i]);
        }
        printf("\n");
    }

    // Создание и открытие файла с использованием коллективной операции
    MPI_File_open(MPI_COMM_WORLD, filename, 
                  MPI_MODE_CREATE | MPI_MODE_WRONLY, 
                  MPI_INFO_NULL, &fh);

    // Выделяем буфер для каждого процесса
    int my_rank_to_write;
    char buffer[20]; // Буфер для записи ранга с переносом строки
    int buffer_len;

    // Распределяем отсортированные ранги среди всех процессов
    MPI_Scatter(rank_buffer, 1, MPI_INT, &my_rank_to_write, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Каждый процесс форматирует строку для записи
    buffer_len = sprintf(buffer, "Rank %d\n", my_rank_to_write);

    // Определение смещения для каждого процесса
    MPI_Offset offset;
    int *recvcounts = NULL;
    int *displs = NULL;

    if (rank == 0) {
        recvcounts = (int *)malloc(size * sizeof(int));
        displs = (int *)malloc(size * sizeof(int));
    }

    // Сбор длин буферов от всех процессов
    MPI_Gather(&buffer_len, 1, MPI_INT, recvcounts, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Вычисление смещений на процессе с rank = 0
    if (rank == 0) {
        displs[0] = 0;
        for (i = 1; i < size; i++) {
            displs[i] = displs[i-1] + recvcounts[i-1];
        }
    }

    // Отправка смещений всем процессам
    MPI_Scatter(displs, 1, MPI_INT, &offset, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Запись в файл с использованием явного смещения
    MPI_File_write_at_all(fh, offset, buffer, buffer_len, MPI_CHAR, &status);

    // Закрытие файла
    MPI_File_close(&fh);

    if (rank == 0) {
        printf("Файл %s успешно создан и заполнен\n", filename);
        free(all_ranks);
        free(rank_buffer);
        free(recvcounts);
        free(displs);
    }

    MPI_Finalize();
    return 0;
}