#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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

int main() {
    double tau = T_MAX / K;  // шаг по времени
    double h = X_MAX / M;    // шаг по пространству
    
    // Выделение памяти для трех временных слоев
    double **u = (double **)malloc((K+1) * sizeof(double *));
    for (int k = 0; k <= K; k++) {
        u[k] = (double *)malloc((M+1) * sizeof(double));
    }
    
    // Заполнение начальных условий для k=0 и k=1
    for (int m = 0; m <= M; m++) {
        double x = m * h;
        u[0][m] = initial_condition(x);
    }
    
    // Используем схему первого порядка для вычисления k=1
    for (int m = 1; m < M; m++) {
        double x = m * h;
        double t = tau;
        u[1][m] = u[0][m] + tau * (-(u[0][m+1] - u[0][m-1])/(2*h) + source_function(x, 0));
    }
    
    // Граничные условия для k=1
    u[1][0] = boundary_condition(tau);
    u[1][M] = boundary_condition(tau);
    
    // Основной цикл решения по схеме "крест"
    for (int k = 1; k < K; k++) {
        // Внутренние точки
        for (int m = 1; m < M; m++) {
            double x = m * h;
            double t = k * tau;
            
            // Схема "крест"
            u[k+1][m] = u[k-1][m] - (tau/h) * (u[k][m+1] - u[k][m-1]) + 
                         2 * tau * source_function(x, t);
        }
        
        // Граничные условия
        u[k+1][0] = boundary_condition((k+1) * tau);
        u[k+1][M] = boundary_condition((k+1) * tau);
    }
    
    // Вывод зависимости u(t) в точке x=X_MAX/2
    int middle_x = M/2;
    printf("Зависимость u(t) в точке x=%.3f:\n", middle_x * h);
    for (int k = 0; k <= K; k += 5) {  // Выводим каждые 5 временных шагов
        printf("t=%.3f, u=%.6f\n", k * tau, u[k][middle_x]);
    }
    
    // Вывод зависимости u(x) при t=T_MAX/2
    int middle_t = 20;
    printf("\nЗависимость u(x) при t=%.3f:\n", middle_t * tau);
    for (int m = 0; m <= M; m += 5) {  // Выводим каждые 5 пространственных шагов
        printf("x=%.3f, u=%.6f\n", m * h, u[middle_t][m]);
    }
    
    // Освобождение памяти
    for (int k = 0; k <= K; k++) {
        free(u[k]);
    }
    free(u);
    
    return 0;
}
