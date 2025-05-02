#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define DELAY 2  // Задержка в секундах

void measure_time(int rank, int size, int message_size, const char* mode) {
    char* message = (char*)malloc(message_size * sizeof(char));
    double start_time, end_time;

    if (rank == 0) {
        // Процесс 0 отправляет сообщение процессу 1
        start_time = MPI_Wtime();
        if (strcmp(mode, "MPI_Send") == 0) {
            MPI_Send(message, message_size, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        } else if (strcmp(mode, "MPI_Ssend") == 0) {
            MPI_Ssend(message, message_size, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        } else if (strcmp(mode, "MPI_Rsend") == 0) {
            MPI_Rsend(message, message_size, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        } else if (strcmp(mode, "MPI_Bsend") == 0) {
            MPI_Bsend(message, message_size, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        }
        end_time = MPI_Wtime();
        printf("Process 0: %s took %f seconds for message size %d\n", mode, end_time - start_time, message_size);
        sleep(DELAY);  // Задержка перед получением
        MPI_Recv(message, message_size, MPI_CHAR, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else if (rank == 1) {
        // Процесс 1 получает сообщение от процесса 0
        sleep(DELAY);  // Задержка перед получением
        MPI_Recv(message, message_size, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send(message, message_size, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
    }

    free(message);
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 2) {
        if (rank == 0) {
            printf("This program requires exactly 2 processes.\n");
        }
        MPI_Finalize();
        return 1;
    }

    int message_sizes[] = {1, 10, 100, 1024, 10240, 102400};
    int num_sizes = 6;

    for (int i = 0; i < num_sizes; i++) {
        measure_time(rank, size, message_sizes[i], "MPI_Send");
        measure_time(rank, size, message_sizes[i], "MPI_Ssend");
        measure_time(rank, size, message_sizes[i], "MPI_Rsend");
    }

    MPI_Finalize();
    return 0;
}