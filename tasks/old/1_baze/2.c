#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    if (argc != 2) {
        if (world_rank == 0) {
            printf("Использование: mpirun -np <количество процессов> %s <N>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    int N = atoi(argv[1]);

    int chunk_size = N / world_size;
    int remainder = N % world_size;

    int start = world_rank * chunk_size + 1;
    int end = (world_rank + 1) * chunk_size;

    if (world_rank == world_size - 1) {
        end += remainder;
    }

    double local_sum = 0.0;
    for (int i = start; i <= end; i++) {
        local_sum += 1.0 / i;
    }

    double global_sum = 0.0;
    MPI_Reduce(&local_sum, &global_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (world_rank == 0) {
        printf("Сумма ряда для N = %d: %.16f\n", N, global_sum);
    }

    MPI_Finalize();
    return 0;
}