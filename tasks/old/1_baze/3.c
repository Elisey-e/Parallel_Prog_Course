#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    int message = 0;

    if (world_rank == 0) {
        message = 0; 
        printf("Процесс %d: начальное значение message = %d\n", world_rank, message);
    }

    if (world_size > 1) {
        int next_rank = (world_rank + 1) % world_size;
        int prev_rank = (world_rank - 1 + world_size) % world_size;

        if (world_rank != 0) {
            MPI_Recv(&message, 1, MPI_INT, prev_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("Процесс %d: получил message = %d\n", world_rank, message);
        }

        message += 1;

        MPI_Send(&message, 1, MPI_INT, next_rank, 0, MPI_COMM_WORLD);

        if (world_rank == 0) {
            MPI_Recv(&message, 1, MPI_INT, prev_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("Процесс %d: получил message = %d (финальное значение)\n", world_rank, message);
        }
    } else {
        printf("Процесс %d: message = %d (только один процесс)\n", world_rank, message);
    }

    MPI_Finalize();
    return 0;
}