#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>

int main() {
    pthread_attr_t attr;
    int ret;
    
    // Инициализация структуры атрибутов потока с дефолтными значениями
    ret = pthread_attr_init(&attr);
    if (ret != 0) {
        fprintf(stderr, "Ошибка при инициализации атрибутов потока: %s\n", strerror(ret));
        return 1;
    }
    
    printf("Значения атрибутов потока по умолчанию:\n\n");
    
    // 1. Получаем scope (область видимости потока)
    int scope;
    ret = pthread_attr_getscope(&attr, &scope);
    if (ret == 0) {
        printf("1. Scope (область видимости потока): ");
        if (scope == PTHREAD_SCOPE_SYSTEM)
            printf("PTHREAD_SCOPE_SYSTEM (1) - системный уровень планирования\n");
        else if (scope == PTHREAD_SCOPE_PROCESS)
            printf("PTHREAD_SCOPE_PROCESS (0) - процессный уровень планирования\n");
        else
            printf("Неизвестное значение: %d\n", scope);
    } else {
        fprintf(stderr, "Ошибка при получении scope: %s\n", strerror(ret));
    }
    
    // 2. Получаем detachstate (состояние отсоединения)
    int detachstate;
    ret = pthread_attr_getdetachstate(&attr, &detachstate);
    if (ret == 0) {
        printf("2. Detachstate (состояние отсоединения): ");
        if (detachstate == PTHREAD_CREATE_JOINABLE)
            printf("PTHREAD_CREATE_JOINABLE (0) - поток с возможностью соединения\n");
        else if (detachstate == PTHREAD_CREATE_DETACHED)
            printf("PTHREAD_CREATE_DETACHED (1) - отсоединенный поток\n");
        else
            printf("Неизвестное значение: %d\n", detachstate);
    } else {
        fprintf(stderr, "Ошибка при получении detachstate: %s\n", strerror(ret));
    }
    
    // 3. Получаем stackaddr (адрес стека)
    void *stackaddr;
    size_t stacksize;
    ret = pthread_attr_getstack(&attr, &stackaddr, &stacksize);
    if (ret == 0) {
        printf("3. Stackaddr (адрес стека): %p\n", stackaddr);
    } else {
        fprintf(stderr, "Ошибка при получении stackaddr: %s\n", strerror(ret));
    }
    
    // 4. Получаем stacksize (размер стека)
    // (stacksize уже получен выше в вызове pthread_attr_getstack)
    printf("4. Stacksize (размер стека): %zu байт", stacksize);
    if (stacksize >= 1024*1024) {
        printf(" (%.2f МБ)\n", (float)stacksize / (1024*1024));
    } else if (stacksize >= 1024) {
        printf(" (%.2f КБ)\n", (float)stacksize / 1024);
    } else {
        printf("\n");
    }
    
    // 5. Получаем inheritsched (наследуемость планирования)
    int inheritsched;
    ret = pthread_attr_getinheritsched(&attr, &inheritsched);
    if (ret == 0) {
        printf("5. Inheritsched (наследуемость планирования): ");
        if (inheritsched == PTHREAD_INHERIT_SCHED)
            printf("PTHREAD_INHERIT_SCHED (0) - параметры планирования наследуются от создающего потока\n");
        else if (inheritsched == PTHREAD_EXPLICIT_SCHED)
            printf("PTHREAD_EXPLICIT_SCHED (1) - параметры планирования берутся из атрибутов\n");
        else
            printf("Неизвестное значение: %d\n", inheritsched);
    } else {
        fprintf(stderr, "Ошибка при получении inheritsched: %s\n", strerror(ret));
    }
    
    // 6. Получаем schedpolicy (политика планирования)
    int schedpolicy;
    ret = pthread_attr_getschedpolicy(&attr, &schedpolicy);
    if (ret == 0) {
        printf("6. Schedpolicy (политика планирования): ");
        if (schedpolicy == SCHED_OTHER)
            printf("SCHED_OTHER (0) - стандартное разделение времени\n");
        else if (schedpolicy == SCHED_FIFO)
            printf("SCHED_FIFO (1) - планирование 'первым пришел - первым обслужен'\n");
        else if (schedpolicy == SCHED_RR)
            printf("SCHED_RR (2) - планирование с алгоритмом 'карусель' (Round Robin)\n");
#ifdef SCHED_BATCH
        else if (schedpolicy == SCHED_BATCH)
            printf("SCHED_BATCH (3) - пакетное планирование\n");
#endif
#ifdef SCHED_IDLE
        else if (schedpolicy == SCHED_IDLE)
            printf("SCHED_IDLE (5) - планирование в режиме простоя\n");
#endif
        else
            printf("Неизвестное значение: %d\n", schedpolicy);
    } else {
        fprintf(stderr, "Ошибка при получении schedpolicy: %s\n", strerror(ret));
    }
    
    // 7. Получаем schedparam (параметры планирования)
    struct sched_param param;
    ret = pthread_attr_getschedparam(&attr, &param);
    if (ret == 0) {
        printf("7. Schedparam.sched_priority (приоритет): %d\n", param.sched_priority);
    } else {
        fprintf(stderr, "Ошибка при получении schedparam: %s\n", strerror(ret));
    }
    
    // 8. Получаем guardsize (размер защитной области стека)
    size_t guardsize;
    ret = pthread_attr_getguardsize(&attr, &guardsize);
    if (ret == 0) {
        printf("8. Guardsize (размер защитной области стека): %zu байт", guardsize);
        if (guardsize >= 1024*1024) {
            printf(" (%.2f МБ)\n", (float)guardsize / (1024*1024));
        } else if (guardsize >= 1024) {
            printf(" (%.2f КБ)\n", (float)guardsize / 1024);
        } else {
            printf("\n");
        }
    } else {
        fprintf(stderr, "Ошибка при получении guardsize: %s\n", strerror(ret));
    }
    
    // Освобождаем ресурсы, занятые атрибутами
    pthread_attr_destroy(&attr);
    
    return 0;
}