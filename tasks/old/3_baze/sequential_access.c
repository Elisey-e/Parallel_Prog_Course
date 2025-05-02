#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// Глобальная переменная, доступ к которой будут получать все потоки
int shared_variable = 0;

// Мьютекс для защиты доступа к глобальной переменной
pthread_mutex_t mutex;

// Условная переменная для организации последовательного доступа
pthread_cond_t cond;

// Переменная для отслеживания текущего потока, которому разрешено войти в критическую секцию
int current_thread = 0;

// Общее количество потоков
int total_threads;

// Структура для передачи данных в поток
typedef struct {
    int thread_id;  // ID потока (0, 1, 2, ...)
} thread_data_t;

// Функция, которую выполняет каждый поток
void* thread_function(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    int id = data->thread_id;
    
    // Получаем блокировку мьютекса
    pthread_mutex_lock(&mutex);
    
    // Ждем своей очереди
    while (current_thread != id) {
        pthread_cond_wait(&cond, &mutex);
    }
    
    // Выполняем действие над глобальной переменной
    shared_variable++;
    
    // Выводим сообщение
    printf("Поток %d: увеличил переменную, текущее значение = %d\n", id, shared_variable);
    
    // Обновляем счетчик текущего потока
    current_thread = (current_thread + 1) % total_threads;
    
    // Сигнализируем другим потокам, что они могут проверить свою очередь
    pthread_cond_broadcast(&cond);
    
    // Освобождаем мьютекс
    pthread_mutex_unlock(&mutex);
    
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    // Проверка аргументов командной строки
    if (argc != 2) {
        printf("Использование: %s <количество_потоков>\n", argv[0]);
        return 1;
    }
    
    // Определяем количество потоков
    total_threads = atoi(argv[1]);
    
    if (total_threads <= 0) {
        printf("Количество потоков должно быть положительным\n");
        return 1;
    }
    
    // Инициализация мьютекса и условной переменной
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    
    // Создаем массив идентификаторов потоков
    pthread_t* threads = (pthread_t*)malloc(total_threads * sizeof(pthread_t));
    
    // Создаем массив структур для передачи данных в потоки
    thread_data_t* thread_data = (thread_data_t*)malloc(total_threads * sizeof(thread_data_t));
    
    // Проверка на ошибки выделения памяти
    if (threads == NULL || thread_data == NULL) {
        printf("Ошибка выделения памяти\n");
        return 1;
    }
    
    // Создаем потоки
    for (int i = 0; i < total_threads; i++) {
        thread_data[i].thread_id = i;
        
        int rc = pthread_create(&threads[i], NULL, thread_function, (void*)&thread_data[i]);
        if (rc) {
            printf("Ошибка: код возврата из pthread_create() равен %d\n", rc);
            free(threads);
            free(thread_data);
            pthread_mutex_destroy(&mutex);
            pthread_cond_destroy(&cond);
            return 1;
        }
    }
    
    // Ждем завершения всех потоков
    for (int i = 0; i < total_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Выводим итоговое значение переменной
    printf("\nИтоговое значение shared_variable = %d\n", shared_variable);
    
    // Освобождаем ресурсы
    free(threads);
    free(thread_data);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    
    return 0;
}