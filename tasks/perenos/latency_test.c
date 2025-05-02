#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

#define NUM_ITERATIONS 1000   // Количество повторений для точного измерения
#define WARMUP_ITERATIONS 10  // Количество "разогревающих" итераций

int main(int argc, char *argv[]) {
    int rank, size;
    int i;
    double start_time, end_time, elapsed_time;
    double min_time = 1e10, max_time = 0, avg_time = 0;
    int message = 42;  // Данные для отправки
    
    // Инициализация MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Проверяем, что у нас ровно два процесса
    if (size != 2) {
        if (rank == 0) {
            printf("Эта программа должна запускаться ровно с двумя процессами!\n");
        }
        MPI_Finalize();
        return 1;
    }
    
    // Разогрев - выполняем несколько итераций без измерения времени
    for (i = 0; i < WARMUP_ITERATIONS; i++) {
        if (rank == 0) {
            MPI_Send(&message, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
            MPI_Recv(&message, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        } else {
            MPI_Recv(&message, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(&message, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
    }
    
    // Синхронизируем процессы перед началом измерений
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Проводим NUM_ITERATIONS измерений
    for (i = 0; i < NUM_ITERATIONS; i++) {
        MPI_Barrier(MPI_COMM_WORLD);  // Синхронизация перед каждым измерением
        
        if (rank == 0) {
            // Процесс 0 отправляет сообщение и замеряет время
            start_time = MPI_Wtime();
            
            MPI_Send(&message, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
            MPI_Recv(&message, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            end_time = MPI_Wtime();
            elapsed_time = (end_time - start_time) * 1000000.0; // Конвертируем в микросекунды
            
            // Обновляем статистику
            if (elapsed_time < min_time) min_time = elapsed_time;
            if (elapsed_time > max_time) max_time = elapsed_time;
            avg_time += elapsed_time;
        } else {
            // Процесс 1 получает сообщение и отправляет обратно
            MPI_Recv(&message, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(&message, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
    }
    
    // Выводим результаты
    if (rank == 0) {
        avg_time /= NUM_ITERATIONS;
        printf("Результаты измерения задержки ping-pong (для %d итераций):\n", NUM_ITERATIONS);
        printf("Минимальное время (RTT): %.2f мкс\n", min_time);
        printf("Максимальное время (RTT): %.2f мкс\n", max_time);
        printf("Среднее время (RTT): %.2f мкс\n", avg_time);
    }
    
    MPI_Finalize();
    return 0;
}