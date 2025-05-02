#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Структура для передачи данных в поток
typedef struct {
    int thread_id;      // Номер потока
    int total_threads;  // Общее количество потоков
    long long n;        // Общее число элементов ряда
    double partial_sum; // Частичная сумма, вычисленная потоком
} thread_data_t;

// Функция, выполняемая каждым потоком
void* calculate_partial_sum(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    
    // Частичная сумма для данного потока
    double sum = 0.0;
    
    // Каждый поток обрабатывает элементы i, где i % total_threads == thread_id
    for (long long i = data->thread_id + 1; i <= data->n; i += data->total_threads) {
        sum += 1.0 / i;
    }
    
    // Сохраняем результат в структуре
    data->partial_sum = sum;
    
    printf("Thread %d calculated partial sum: %f\n", data->thread_id, sum);
    
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    // Проверка аргументов командной строки
    if (argc != 3) {
        printf("Usage: %s <N> <num_threads>\n", argv[0]);
        printf("  N: Number of elements in the harmonic series\n");
        printf("  num_threads: Number of threads to use\n");
        return 1;
    }
    
    // Разбор аргументов
    long long n = atoll(argv[1]);
    int num_threads = atoi(argv[2]);
    
    // Проверка корректности аргументов
    if (n <= 0 || num_threads <= 0) {
        printf("Error: N and num_threads must be positive integers\n");
        return 1;
    }
    
    // Ограничиваем количество потоков, если оно больше N
    if (num_threads > n) {
        num_threads = n;
        printf("Reducing number of threads to %d (equal to N)\n", num_threads);
    }
    
    printf("Calculating partial sum of harmonic series up to %lld using %d threads\n", n, num_threads);
    
    // Создаем массивы для потоков и данных
    pthread_t* threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    thread_data_t* thread_data = (thread_data_t*)malloc(num_threads * sizeof(thread_data_t));
    
    if (threads == NULL || thread_data == NULL) {
        printf("Error: Memory allocation failed\n");
        return 1;
    }
    
    // Создаем потоки
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].total_threads = num_threads;
        thread_data[i].n = n;
        thread_data[i].partial_sum = 0.0;
        
        int rc = pthread_create(&threads[i], NULL, calculate_partial_sum, (void*)&thread_data[i]);
        if (rc) {
            printf("ERROR: return code from pthread_create() is %d\n", rc);
            free(threads);
            free(thread_data);
            return 1;
        }
    }
    
    // Ждем завершения всех потоков
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Суммируем результаты всех потоков
    double total_sum = 0.0;
    for (int i = 0; i < num_threads; i++) {
        total_sum += thread_data[i].partial_sum;
    }
    
    printf("\nThe partial sum of the harmonic series up to %lld is: %.15f\n", n, total_sum);
    
    // Освобождаем память
    free(threads);
    free(thread_data);
    
    return 0;
}