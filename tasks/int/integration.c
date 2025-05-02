#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <sys/time.h>

// Функция, которую мы интегрируем
// Изменить функцию можно здесь
double f(double x) {
    return sin(1/(x * x * x) );
    // return sin(x) * exp(-0.1 * x * x);
    // return sin(x / 3.1415)
}

// Правило Симпсона для одного сегмента
double simpson(double a, double b) {
    double h = b - a;
    return h / 6.0 * (f(a) + 4.0 * f((a + b) / 2.0) + f(b));
}

// Адаптивное интегрирование на отрезке
double adaptive_integrate(double a, double b, double tol, int *eval_count) {
    double c = (a + b) / 2.0;
    double whole = simpson(a, b);
    double left = simpson(a, c);
    double right = simpson(c, b);
    double diff = fabs(left + right - whole);
    
    (*eval_count) += 5; // Учитываем вызовы функции f()
    
    if (diff <= 15.0 * tol) {
        return left + right;
    } else {
        double tol_half = tol / 2.0;
        return adaptive_integrate(a, c, tol_half, eval_count) + 
               adaptive_integrate(c, b, tol_half, eval_count);
    }
}

// Структура для хранения задач
typedef struct {
    double a;        // Нижняя граница интегрирования
    double b;        // Верхняя граница интегрирования
    double tol;      // Допустимая погрешность
    double result;   // Результат
    int eval_count;  // Количество вызовов функции
} Task;

// Очередь задач
typedef struct {
    Task *tasks;           // Массив задач
    int capacity;          // Вместимость очереди
    int size;              // Текущий размер
    int front;             // Указатель на начало очереди
    int rear;              // Указатель на конец очереди
    pthread_mutex_t mutex; // Мьютекс для синхронизации
    pthread_cond_t not_empty; // Условная переменная для ожидания задач
    pthread_cond_t not_full;  // Условная переменная для ожидания места
    int finished;          // Флаг завершения работы
} TaskQueue;

// Глобальные переменные
TaskQueue task_queue;
double global_result = 0.0;      // Общий результат
int total_eval_count = 0;        // Общее количество вызовов функции
pthread_mutex_t result_mutex;    // Мьютекс для обновления результата

// Инициализация очереди задач
void init_queue(int capacity) {
    task_queue.tasks = (Task*)malloc(capacity * sizeof(Task));
    task_queue.capacity = capacity;
    task_queue.size = 0;
    task_queue.front = 0;
    task_queue.rear = -1;
    task_queue.finished = 0;
    pthread_mutex_init(&task_queue.mutex, NULL);
    pthread_cond_init(&task_queue.not_empty, NULL);
    pthread_cond_init(&task_queue.not_full, NULL);
    pthread_mutex_init(&result_mutex, NULL);
}

// Освобождение ресурсов очереди
void destroy_queue() {
    free(task_queue.tasks);
    pthread_mutex_destroy(&task_queue.mutex);
    pthread_cond_destroy(&task_queue.not_empty);
    pthread_cond_destroy(&task_queue.not_full);
    pthread_mutex_destroy(&result_mutex);
}

// Добавление задачи в очередь
void enqueue(Task task) {
    pthread_mutex_lock(&task_queue.mutex);
    
    // Ждем, пока очередь не освободится
    while (task_queue.size == task_queue.capacity && !task_queue.finished) {
        pthread_cond_wait(&task_queue.not_full, &task_queue.mutex);
    }
    
    // Проверяем, не завершена ли работа
    if (task_queue.finished) {
        pthread_mutex_unlock(&task_queue.mutex);
        return;
    }
    
    // Добавляем задачу
    task_queue.rear = (task_queue.rear + 1) % task_queue.capacity;
    task_queue.tasks[task_queue.rear] = task;
    task_queue.size++;
    
    // Сигнализируем о наличии задачи
    pthread_cond_signal(&task_queue.not_empty);
    pthread_mutex_unlock(&task_queue.mutex);
}

// Извлечение задачи из очереди
int dequeue(Task *task) {
    pthread_mutex_lock(&task_queue.mutex);
    
    // Ждем, пока в очереди появится задача
    while (task_queue.size == 0 && !task_queue.finished) {
        pthread_cond_wait(&task_queue.not_empty, &task_queue.mutex);
    }
    
    // Проверяем, не пуста ли очередь
    if (task_queue.size == 0) {
        pthread_mutex_unlock(&task_queue.mutex);
        return 0; // Очередь пуста и работа завершена
    }
    
    // Извлекаем задачу
    *task = task_queue.tasks[task_queue.front];
    task_queue.front = (task_queue.front + 1) % task_queue.capacity;
    task_queue.size--;
    
    // Сигнализируем о наличии места в очереди
    pthread_cond_signal(&task_queue.not_full);
    pthread_mutex_unlock(&task_queue.mutex);
    
    return 1; // Задача успешно извлечена
}

// Устанавливаем флаг завершения работы
void finish_queue() {
    pthread_mutex_lock(&task_queue.mutex);
    task_queue.finished = 1;
    pthread_cond_broadcast(&task_queue.not_empty); // Будим все потоки
    pthread_cond_broadcast(&task_queue.not_full);  // Будим все потоки
    pthread_mutex_unlock(&task_queue.mutex);
}

// Функция для рабочего потока
void* worker_thread(void *arg) {
    Task task;
    
    while (dequeue(&task)) {
        // Вычисляем интеграл для полученного участка
        int task_eval_count = 0;
        double task_result = adaptive_integrate(task.a, task.b, task.tol, &task_eval_count);
        
        // Обновляем глобальный результат
        pthread_mutex_lock(&result_mutex);
        global_result += task_result;
        total_eval_count += task_eval_count;
        pthread_mutex_unlock(&result_mutex);
    }
    
    return NULL;
}

// Функция для получения текущего времени в секундах
double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

int main(int argc, char *argv[]) {
    // Проверка аргументов командной строки
    if (argc < 5) {
        printf("Использование: %s <нижняя_граница> <верхняя_граница> <допустимая_погрешность> <число_потоков>\n", argv[0]);
        return 1;
    }
    
    double a = atof(argv[1]);
    double b = atof(argv[2]);
    double tol = atof(argv[3]);
    int num_threads = atoi(argv[4]);
    
    printf("Численное интегрирование функции на интервале [%g, %g] с точностью %g\n", a, b, tol);
    printf("Количество потоков: %d\n", num_threads);
    
    // Если потоков нет, выполняем последовательно
    if (num_threads <= 0) {
        double start_time = get_time();
        int seq_eval_count = 0;
        double seq_result = adaptive_integrate(a, b, tol, &seq_eval_count);
        double end_time = get_time();
        
        printf("\n=== Результаты ===\n");
        printf("Результат интегрирования: %g\n", seq_result);
        printf("Время выполнения: %g сек.\n", end_time - start_time);
        printf("Вызовов функции: %d\n", seq_eval_count);
        
        return 0;
    }
    
    // Инициализация очереди задач
    int num_initial_tasks = num_threads * 4;
    init_queue(num_initial_tasks * 2); // Вместимость в два раза больше для буферизации
    
    // Создаем начальные задачи
    double segment_len = (b - a) / num_initial_tasks;
    Task initial_task;
    
    for (int i = 0; i < num_initial_tasks; i++) {
        initial_task.a = a + i * segment_len;
        initial_task.b = a + (i + 1) * segment_len;
        initial_task.tol = tol / num_initial_tasks;
        enqueue(initial_task);
    }
    
    // Создаем потоки
    pthread_t *threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    
    // Запускаем таймер
    double start_time = get_time();
    
    // Запускаем рабочие потоки
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, worker_thread, NULL);
    }
    
    // Ожидаем завершения работы потоков
    finish_queue(); // Устанавливаем флаг завершения
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Завершаем измерение времени
    double end_time = get_time();
    double parallel_time = end_time - start_time;
    
    // Последовательное вычисление для сравнения
    start_time = get_time();
    int seq_eval_count = 0;
    double seq_result = adaptive_integrate(a, b, tol, &seq_eval_count);
    end_time = get_time();
    double seq_time = end_time - start_time;
    
    // Выводим результаты
    printf("\n=== Результаты ===\n");
    printf("Результат интегрирования (параллельный): %g\n", global_result);
    printf("Проверка (последовательный алгоритм): %g\n", seq_result);
    printf("Разница: %g\n", fabs(global_result - seq_result));
    
    printf("\n=== Производительность ===\n");
    printf("Время параллельного выполнения: %g сек.\n", parallel_time);
    printf("Время последовательного выполнения: %g сек.\n", seq_time);
    printf("Ускорение: %g\n", seq_time / parallel_time);
    printf("Эффективность: %g%%\n", (seq_time / parallel_time / num_threads) * 100);
    
    printf("\n=== Статистика ===\n");
    printf("Всего вызовов функции (параллельно): %d\n", total_eval_count);
    printf("Вызовов функции (последовательно): %d\n", seq_eval_count);
    
    // Освобождаем ресурсы
    free(threads);
    destroy_queue();
    
    return 0;
}