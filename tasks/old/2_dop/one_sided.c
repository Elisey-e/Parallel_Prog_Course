#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    // Проверка аргументов командной строки
    if (argc != 2) {
        if (world_rank == 0) {
            printf("Использование: mpirun -np <количество процессов> %s <N>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    int N = atoi(argv[1]);

    // Распределение работы между процессами
    int chunk_size = N / world_size;
    int remainder = N % world_size;

    int start = world_rank * chunk_size + 1;
    int end = (world_rank + 1) * chunk_size;

    if (world_rank == world_size - 1) {
        end += remainder;
    }

    // Вычисление локальной суммы
    double local_sum = 0.0;
    for (int i = start; i <= end; i++) {
        local_sum += 1.0 / i;
    }

    // Создание окна MPI для one-sided коммуникации
    MPI_Win win;
    double *global_sum_ptr = NULL;
    
    if (world_rank == 0) {
        // Только процесс с рангом 0 выделяет память для глобальной суммы
        global_sum_ptr = (double*)malloc(sizeof(double));
        *global_sum_ptr = 0.0;
    }
    
    // Создание окна MPI, привязанной к буферу global_sum_ptr
    MPI_Win_create(global_sum_ptr, (world_rank == 0) ? sizeof(double) : 0, 
                   sizeof(double), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    
    // Начало эпохи доступа (RMA)
    MPI_Win_fence(0, win);
    
    // Каждый процесс атомарно добавляет свою локальную сумму к глобальной
    MPI_Accumulate(&local_sum, 1, MPI_DOUBLE, 0, 0, 1, MPI_DOUBLE, MPI_SUM, win);
    
    // Завершение эпохи доступа
    MPI_Win_fence(0, win);
    
    // Вывод результата
    if (world_rank == 0) {
        printf("Сумма ряда для N = %d: %.16f\n", N, *global_sum_ptr);
        free(global_sum_ptr);
    }
    
    // Освобождение окна MPI
    MPI_Win_free(&win);

    MPI_Finalize();
    return 0;
}
