#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

// Функция, которую мы интегрируем
// Изменить функцию можно здесь
double f(double x) {
    return sin(x) * exp(-0.1 * x * x);
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

// Тэги для MPI сообщений
#define TASK_TAG 1
#define RESULT_TAG 2
#define TERMINATE_TAG 3

// Структура задачи
typedef struct {
    double a;
    double b;
    double tol;
} Task;

// Структура результата
typedef struct {
    double result;
    int eval_count;
} Result;

int main(int argc, char *argv[]) {
    int rank, size, eval_count = 0, total_eval_count = 0;
    double a, b, tol, local_result = 0.0, global_result = 0.0;
    double start_time, end_time, total_time;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Выделяем отдельный процесс как мастер
    int master = 0;
    int num_workers = size - 1;
    
    if (rank == master) {
        // Проверка аргументов командной строки
        if (argc < 4) {
            printf("Использование: %s <нижняя_граница> <верхняя_граница> <допустимая_погрешность>\n", argv[0]);
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }
        
        a = atof(argv[1]);
        b = atof(argv[2]);
        tol = atof(argv[3]);
        
        printf("Численное интегрирование функции на интервале [%g, %g] с точностью %g\n", a, b, tol);
        printf("Количество процессов: %d (1 мастер + %d рабочих)\n", size, num_workers);
        
        // Если нет рабочих процессов, выполняем последовательно
        if (num_workers == 0) {
            start_time = MPI_Wtime();
            global_result = adaptive_integrate(a, b, tol, &total_eval_count);
            end_time = MPI_Wtime();
            total_time = end_time - start_time;
            
            printf("\n=== Результаты ===\n");
            printf("Результат интегрирования: %g\n", global_result);
            printf("Время выполнения: %g сек.\n", total_time);
            printf("Вызовов функции: %d\n", total_eval_count);
            
            MPI_Finalize();
            return 0;
        }
    }
    
    // Рассылаем параметры всем процессам
    MPI_Bcast(&a, 1, MPI_DOUBLE, master, MPI_COMM_WORLD);
    MPI_Bcast(&b, 1, MPI_DOUBLE, master, MPI_COMM_WORLD);
    MPI_Bcast(&tol, 1, MPI_DOUBLE, master, MPI_COMM_WORLD);
    
    // Синхронизируем начало работы и запускаем таймер
    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();
    
    if (rank == master) {
        // Мастер-процесс
        
        // Создаем начальные подзадачи - больше, чем число рабочих
        int num_initial_tasks = num_workers * 4;
        double segment_len = (b - a) / num_initial_tasks;
        
        // Очередь задач
        Task *task_queue = (Task*)malloc(num_initial_tasks * sizeof(Task));
        for (int i = 0; i < num_initial_tasks; i++) {
            task_queue[i].a = a + i * segment_len;
            task_queue[i].b = a + (i + 1) * segment_len;
            task_queue[i].tol = tol / num_initial_tasks;
        }
        
        int next_task = 0;
        int tasks_completed = 0;
        
        // Отправляем первую задачу каждому рабочему
        for (int worker = 1; worker <= num_workers && next_task < num_initial_tasks; worker++, next_task++) {
            MPI_Send(&task_queue[next_task], sizeof(Task), MPI_BYTE, worker, TASK_TAG, MPI_COMM_WORLD);
        }
        
        // Обрабатываем результаты и отправляем новые задачи
        Result result;
        MPI_Status status;
        
        while (tasks_completed < num_initial_tasks) {
            // Получаем результат от любого рабочего
            MPI_Recv(&result, sizeof(Result), MPI_BYTE, MPI_ANY_SOURCE, RESULT_TAG, MPI_COMM_WORLD, &status);
            
            // Обновляем общий результат
            global_result += result.result;
            total_eval_count += result.eval_count;
            tasks_completed++;
            
            // Если есть еще задачи, отправляем следующую задачу
            if (next_task < num_initial_tasks) {
                MPI_Send(&task_queue[next_task], sizeof(Task), MPI_BYTE, status.MPI_SOURCE, TASK_TAG, MPI_COMM_WORLD);
                next_task++;
            } else {
                // Отправляем сигнал завершения
                MPI_Send(NULL, 0, MPI_BYTE, status.MPI_SOURCE, TERMINATE_TAG, MPI_COMM_WORLD);
            }
        }
        
        // Освобождаем память
        free(task_queue);
        
    } else {
        // Рабочие процессы
        Task task;
        Result result;
        MPI_Status status;
        
        while (1) {
            // Получаем задачу от мастера
            MPI_Probe(master, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            
            if (status.MPI_TAG == TERMINATE_TAG) {
                // Получаем сигнал завершения
                MPI_Recv(NULL, 0, MPI_BYTE, master, TERMINATE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                break;
            } else {
                // Получаем задачу
                MPI_Recv(&task, sizeof(Task), MPI_BYTE, master, TASK_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                // Вычисляем интеграл для полученного участка
                int task_eval_count = 0;
                double task_result = adaptive_integrate(task.a, task.b, task.tol, &task_eval_count);
                
                // Формируем и отправляем результат
                result.result = task_result;
                result.eval_count = task_eval_count;
                MPI_Send(&result, sizeof(Result), MPI_BYTE, master, RESULT_TAG, MPI_COMM_WORLD);
                
                // Аккумулируем для статистики
                local_result += task_result;
                eval_count += task_eval_count;
            }
        }
    }
    
    // Завершаем измерение времени
    MPI_Barrier(MPI_COMM_WORLD);
    end_time = MPI_Wtime();
    total_time = end_time - start_time;
    
    // Дополнительно запускаем последовательное вычисление для сравнения на процессе 0
    double seq_result = 0.0, seq_time = 0.0;
    int seq_eval_count = 0;
    
    if (rank == master) {
        start_time = MPI_Wtime();
        seq_result = adaptive_integrate(a, b, tol, &seq_eval_count);
        end_time = MPI_Wtime();
        seq_time = end_time - start_time;
        
        // Выводим результаты
        printf("\n=== Результаты ===\n");
        printf("Результат интегрирования: %g\n", global_result);
        printf("Проверка (последовательный алгоритм): %g\n", seq_result);
        printf("Разница: %g\n", fabs(global_result - seq_result));
        
        printf("\n=== Производительность ===\n");
        printf("Время параллельного выполнения: %g сек.\n", total_time);
        printf("Время последовательного выполнения: %g сек.\n", seq_time);
        printf("Ускорение: %g\n", seq_time / total_time);
        printf("Эффективность: %g%%\n", (seq_time / total_time / num_workers) * 100);
        
        printf("\n=== Статистика ===\n");
        printf("Всего вызовов функции: %d\n", total_eval_count);
        printf("Вызовов функции в последовательном алгоритме: %d\n", seq_eval_count);
    }
    
    MPI_Finalize();
    return 0;
}