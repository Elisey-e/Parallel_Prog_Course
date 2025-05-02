#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

// Параметры сетки
#define T_MAX 1.0     // максимальное время
#define X_MAX 1.0     // размер расчетной области по x
#define K 100         // количество шагов по времени
#define M 100         // количество шагов по пространству

// Начальное условие u(x,0)
double initial_condition(double x) {
    return sin(2.0 * M_PI * x);
}

// Граничные условия u(0,t) и u(X_MAX,t)
double boundary_condition(double t) {
    return 0.0;
}

// Функция источника f(x,t)
double source_function(double x, double t) {
    return 0.0;
}

int main(int argc, char **argv) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Шаги сетки
    double tau = T_MAX / K;  // шаг по времени
    double h = X_MAX / M;    // шаг по пространству
    
    // Расчет локальных границ для каждого процесса
    int local_M = M / size;
    if (rank < M % size) local_M++;
    
    // Определение глобальных индексов начала и конца для каждого процесса
    int start_m = 0;
    int tmp = 0;
    for (int i = 0; i < rank; i++) {
        tmp = M / size;
        if (i < M % size) tmp++;
        start_m += tmp;
    }
    int end_m = start_m + local_M - 1;
    
    // Выделение памяти для трех временных слоев с учетом ghost cells
    double **u = (double **)malloc((K+1) * sizeof(double *));
    for (int k = 0; k <= K; k++) {
        u[k] = (double *)malloc((local_M+2) * sizeof(double)); // +2 для ghost cells
    }
    
    // Заполнение начальных условий для k=0
    for (int i = 0; i <= local_M+1; i++) {
        int m = start_m + i - 1; // Глобальный индекс с учетом ghost cells
        
        if (m >= 0 && m <= M) {
            double x = m * h;
            u[0][i] = initial_condition(x);
        }
    }
    
    // Используем схему первого порядка для вычисления k=1
    for (int i = 1; i <= local_M; i++) {
        int m = start_m + i - 1; // Глобальный индекс без ghost cells
        
        if (m > 0 && m < M) {
            double x = m * h;
            u[1][i] = u[0][i] + tau * (-(u[0][i+1] - u[0][i-1])/(2*h) + source_function(x, 0));
        } else {
            // Граничные условия для k=1
            u[1][i] = boundary_condition(tau);
        }
    }
    
    // Основной цикл по времени
    for (int k = 1; k < K; k++) {
        // Обмен ghost cells между процессами
        double send_left = u[k][1];
        double send_right = u[k][local_M];
        double recv_left = 0.0, recv_right = 0.0;
        
        MPI_Status status;
        
        // Отправка вправо, прием слева
        if (rank > 0) {
            MPI_Sendrecv(&send_left, 1, MPI_DOUBLE, rank-1, 0,
                         &recv_left, 1, MPI_DOUBLE, rank-1, 0,
                         MPI_COMM_WORLD, &status);
            u[k][0] = recv_left;
        } else {
            // Граничное условие для левой границы
            u[k][0] = boundary_condition(k * tau);
        }
        
        // Отправка влево, прием справа
        if (rank < size - 1) {
            MPI_Sendrecv(&send_right, 1, MPI_DOUBLE, rank+1, 0,
                         &recv_right, 1, MPI_DOUBLE, rank+1, 0,
                         MPI_COMM_WORLD, &status);
            u[k][local_M+1] = recv_right;
        } else {
            // Граничное условие для правой границы
            u[k][local_M+1] = boundary_condition(k * tau);
        }
        
        // Вычисление следующего временного слоя по схеме "крест"
        for (int i = 1; i <= local_M; i++) {
            int m = start_m + i - 1; // Глобальный индекс
            
            if (m > 0 && m < M) {
                double x = m * h;
                double t = k * tau;
                
                // Схема "крест"
                u[k+1][i] = u[k-1][i] - (tau/h) * (u[k][i+1] - u[k][i-1]) + 
                             2 * tau * source_function(x, t);
            } else {
                // Граничное условие
                u[k+1][i] = boundary_condition((k+1) * tau);
            }
        }
    }
    
    // Сбор результатов на процессе с rank=0 для вывода
    if (rank == 0) {
        // Буфер для хранения всех результатов
        double **global_u = (double **)malloc((K+1) * sizeof(double *));
        for (int k = 0; k <= K; k++) {
            global_u[k] = (double *)malloc((M+1) * sizeof(double));
            
            // Копирование локальных данных процесса 0
            for (int i = 1; i <= local_M; i++) {
                int m = start_m + i - 1;
                global_u[k][m] = u[k][i];
            }
        }
        
        // Получение данных от других процессов
        for (int p = 1; p < size; p++) {
            int p_local_M = M / size;
            if (p < M % size) p_local_M++;
            
            int p_start_m = 0;
            int tmp = 0;
            for (int i = 0; i < p; i++) {
                tmp = M / size;
                if (i < M % size) tmp++;
                p_start_m += tmp;
            }
            
            // Получение данных для каждого временного слоя
            for (int k = 0; k <= K; k++) {
                double *buffer = (double *)malloc(p_local_M * sizeof(double));
                MPI_Recv(buffer, p_local_M, MPI_DOUBLE, p, k, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                // Копирование данных в глобальный массив
                for (int i = 0; i < p_local_M; i++) {
                    int m = p_start_m + i;
                    global_u[k][m] = buffer[i];
                }
                free(buffer);
            }
        }
        
        // Вывод зависимости u(t) в точке x=X_MAX/2
        int middle_x = M/2;
        printf("Зависимость u(t) в точке x=%.3f:\n", middle_x * h);
        for (int k = 0; k <= K; k += 5) {  // Выводим каждые 5 временных шагов
            printf("t=%.3f, u=%.6f\n", k * tau, global_u[k][middle_x]);
        }
        
        // Вывод зависимости u(x) при t=T_MAX/2
        int middle_t = K/2;
        printf("\nЗависимость u(x) при t=%.3f:\n", middle_t * tau);
        for (int m = 0; m <= M; m += 5) {  // Выводим каждые 5 пространственных шагов
            printf("x=%.3f, u=%.6f\n", m * h, global_u[middle_t][m]);
        }
        
        // Освобождение памяти
        for (int k = 0; k <= K; k++) {
            free(global_u[k]);
        }
        free(global_u);
    } else {
        // Отправка результатов на процесс с rank=0
        for (int k = 0; k <= K; k++) {
            // Копирование данных без ghost cells в буфер
            double *buffer = (double *)malloc(local_M * sizeof(double));
            for (int i = 0; i < local_M; i++) {
                buffer[i] = u[k][i+1];
            }
            
            // Отправка буфера
            MPI_Send(buffer, local_M, MPI_DOUBLE, 0, k, MPI_COMM_WORLD);
            free(buffer);
        }
    }
    
    // Освобождение памяти
    for (int k = 0; k <= K; k++) {
        free(u[k]);
    }
    free(u);
    
    MPI_Finalize();
    return 0;
}
