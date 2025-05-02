#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#define SERVER_RANK 0
#define MAX_STRING 100

int main(int argc, char *argv[]) {
    int rank, size;
    char message[MAX_STRING];
    MPI_Status status;

    // Инициализация MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        fprintf(stderr, "Для работы требуется минимум 2 процесса (1 сервер и 1 клиент)\n");
        MPI_Finalize();
        return 1;
    }

    if (rank == SERVER_RANK) {
        // Код сервера
        printf("Сервер запущен (ранг %d)\n", rank);
        
        // Принимаем сообщения от всех клиентов
        for (int client = 1; client < size; client++) {
            MPI_Recv(message, MAX_STRING, MPI_CHAR, client, 0, MPI_COMM_WORLD, &status);
            printf("Сервер получил сообщение от клиента %d: %s\n", client, message);
            
            // Отправляем ответ клиенту
            sprintf(message, "Привет, клиент %d! Ваше сообщение получено.", client);
            MPI_Send(message, strlen(message) + 1, MPI_CHAR, client, 0, MPI_COMM_WORLD);
        }
        
        printf("Сервер завершил работу\n");
    } else {
        // Код клиента
        printf("Клиент %d запущен\n", rank);
        
        // Формируем сообщение для сервера
        sprintf(message, "Привет от клиента %d!", rank);
        
        // Отправляем сообщение серверу
        printf("Клиент %d отправляет сообщение серверу: %s\n", rank, message);
        MPI_Send(message, strlen(message) + 1, MPI_CHAR, SERVER_RANK, 0, MPI_COMM_WORLD);
        
        // Получаем ответ от сервера
        MPI_Recv(message, MAX_STRING, MPI_CHAR, SERVER_RANK, 0, MPI_COMM_WORLD, &status);
        printf("Клиент %d получил ответ: %s\n", rank, message);
    }

    // Завершение работы MPI
    MPI_Finalize();
    return 0;
}