#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int rank, size;
    int parent_rank;
    MPI_Comm parent_comm;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Получаем коммуникатор родителя
    MPI_Comm_get_parent(&parent_comm);
    
    if (parent_comm == MPI_COMM_NULL) {
        printf("Error: No parent communicator available\n");
        MPI_Finalize();
        return 1;
    }
    
    // Имитация работы - вывод информации о своем процессе
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    printf("Child process %d of %d running on %s\n", rank, size, hostname);
    
    // Получаем количество родительских процессов
    int parent_size;
    MPI_Comm_remote_size(parent_comm, &parent_size);
    
    // Определяем, к какому родительскому процессу относится этот дочерний процесс
    // Для простоты: parent_rank = rank / 4 (если каждый запускает по 4)
    parent_rank = rank / (size / parent_size);
    
    // Круговая пересылка данных между дочерними процессами
    int send_data = rank + 100;  // Данные для отправки
    int recv_data = 0;           // Данные для получения
    int next_rank = (rank + 1) % size;  // Ранг следующего процесса в кольце
    int prev_rank = (rank - 1 + size) % size;  // Ранг предыдущего процесса в кольце
    
    MPI_Status status;
    
    // Реализация круговой пересылки
    if (rank == 0) {
        // Первый процесс отправляет данные следующему
        MPI_Send(&send_data, 1, MPI_INT, next_rank, 0, MPI_COMM_WORLD);
        printf("Child process %d: Sent %d to process %d\n", rank, send_data, next_rank);
        
        // Получаем данные от последнего процесса
        MPI_Recv(&recv_data, 1, MPI_INT, prev_rank, 0, MPI_COMM_WORLD, &status);
        printf("Child process %d: Received %d from process %d\n", rank, recv_data, prev_rank);
        
        // Отправляем подтверждение завершения родительскому процессу
        int success = 1;
        MPI_Send(&success, 1, MPI_INT, parent_rank, 0, parent_comm);
        printf("Child process %d: Sent completion confirmation to parent %d\n", rank, parent_rank);
    } else {
        // Остальные процессы сначала получают данные, затем отправляют
        MPI_Recv(&recv_data, 1, MPI_INT, prev_rank, 0, MPI_COMM_WORLD, &status);
        printf("Child process %d: Received %d from process %d\n", rank, recv_data, prev_rank);
        
        // Модифицируем данные
        send_data = recv_data + 1;
        
        // Отправляем данные дальше
        MPI_Send(&send_data, 1, MPI_INT, next_rank, 0, MPI_COMM_WORLD);
        printf("Child process %d: Sent %d to process %d\n", rank, send_data, next_rank);
    }
    
    // Завершаем работу
    MPI_Finalize();
    return 0;
}