#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Структура для передачи данных в поток
typedef struct {
    int thread_id;
    int total_threads;
} thread_data_t;

// Функция, которую будет выполнять каждый поток
void* print_hello(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    
    printf("Hello World from thread %d of %d\n", 
           data->thread_id, 
           data->total_threads);
    
    pthread_exit(NULL);
}

int main() {
    // Определяем количество потоков
    int num_threads = 4; // Можно изменить по вашему желанию
    
    // Создаем массив идентификаторов потоков
    pthread_t threads[num_threads];
    
    // Создаем массив структур для передачи данных в потоки
    thread_data_t thread_data[num_threads];
    
    // Создаем потоки
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i + 1; // Нумеруем с 1
        thread_data[i].total_threads = num_threads;
        
        int rc = pthread_create(&threads[i], NULL, print_hello, (void*)&thread_data[i]);
        if (rc) {
            printf("ERROR: return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }
    
    // Ждем завершения всех потоков
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("All threads have completed\n");
    pthread_exit(NULL);
}
