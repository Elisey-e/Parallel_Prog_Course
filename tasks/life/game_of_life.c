#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <mpi.h>
#include <omp.h>

// Режимы работы программы
#define DEMO_MODE 0
#define PERFORMANCE_MODE 1

// Для демонстрационного режима
#define DEMO_WIDTH 50
#define DEMO_HEIGHT 30
#define DEMO_STEPS 100
#define DEMO_DELAY_MS 200

// Для режима измерения производительности
#define PERF_WIDTH 5000
#define PERF_HEIGHT 5000
#define PERF_STEPS 1000

// Структура для описания "живой" клетки в оптимизированной версии
typedef struct {
    int x, y;
} LiveCell;

// Функция для инициализации случайного состояния поля
void randomInitialization(unsigned char *grid, int width, int height) {
    int i, j;
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            grid[i * width + j] = (rand() % 4 == 0) ? 1 : 0;  // 25% вероятность "живой" клетки
        }
    }
}

// Функция для инициализации поля с планером (glider)
void gliderInitialization(unsigned char *grid, int width, int height) {
    memset(grid, 0, width * height * sizeof(unsigned char));
    
    // Размещение планера в верхнем левом углу
    int startX = width / 4;
    int startY = height / 4;
    
    grid[(startY) * width + (startX + 1)] = 1;
    grid[(startY + 1) * width + (startX + 2)] = 1;
    grid[(startY + 2) * width + (startX)] = 1;
    grid[(startY + 2) * width + (startX + 1)] = 1;
    grid[(startY + 2) * width + (startX + 2)] = 1;
}

// Функция для вывода текущего состояния поля в консоль (для демо-режима)
void printGrid(unsigned char *grid, int width, int height) {
    int i, j;
    printf("\033[H\033[J"); // Очистка экрана
    
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            printf("%c ", grid[i * width + j] ? '#' : '.');
        }
        printf("\n");
    }
    
    // Задержка для удобства просмотра
    struct timespec ts;
    ts.tv_sec = DEMO_DELAY_MS / 1000;
    ts.tv_nsec = (DEMO_DELAY_MS % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

// Функция для подсчета живых соседей клетки (с учетом тороидальной формы поля)
int countLiveNeighbors(unsigned char *grid, int width, int height, int x, int y) {
    int count = 0;
    int i, j;
    
    for (i = -1; i <= 1; i++) {
        for (j = -1; j <= 1; j++) {
            if (i == 0 && j == 0) continue;  // Пропускаем саму клетку
            
            // Тороидальные координаты
            int nx = (x + j + width) % width;
            int ny = (y + i + height) % height;
            
            count += grid[ny * width + nx];
        }
    }
    
    return count;
}

// Основная функция для вычисления следующего состояния игры
void computeNextGeneration(unsigned char *currentGrid, unsigned char *nextGrid, 
                          int width, int height, int startRow, int endRow) {
    int i, j;
    
    #pragma omp parallel for private(j)
    for (i = startRow; i < endRow; i++) {
        for (j = 0; j < width; j++) {
            int neighbors = countLiveNeighbors(currentGrid, width, height, j, i);
            int idx = i * width + j;
            
            if (currentGrid[idx]) {
                // Живая клетка
                nextGrid[idx] = (neighbors == 2 || neighbors == 3) ? 1 : 0;
            } else {
                // Мертвая клетка
                nextGrid[idx] = (neighbors == 3) ? 1 : 0;
            }
        }
    }
}

// Проверка, изменилась ли область за последнюю итерацию
bool hasAreaChanged(unsigned char *oldGrid, unsigned char *newGrid, int width, int rowStart, int rowEnd) {
    int size = (rowEnd - rowStart) * width;
    return memcmp(oldGrid + rowStart * width, newGrid + rowStart * width, size) != 0;
}

// Основная функция программы
int main(int argc, char *argv[]) {
    int rank, size, provided;
    int width, height, steps;
    int mode = PERFORMANCE_MODE;  // По умолчанию режим измерения производительности
    unsigned char *currentGrid = NULL, *nextGrid = NULL;
    unsigned char *localCurrentGrid = NULL, *localNextGrid = NULL;
    double startTime, endTime;
    
    // Инициализация MPI с поддержкой многопоточности
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Обработка аргументов командной строки
    if (argc > 1) {
        if (strcmp(argv[1], "demo") == 0) {
            mode = DEMO_MODE;
        }
    }
    
    // Установка параметров в зависимости от режима
    if (mode == DEMO_MODE) {
        width = DEMO_WIDTH;
        height = DEMO_HEIGHT;
        steps = DEMO_STEPS;
    } else {
        width = PERF_WIDTH;
        height = PERF_HEIGHT;
        steps = PERF_STEPS;
    }
    
    // Выделение памяти для всего поля (только в корневом процессе)
    if (rank == 0) {
        currentGrid = (unsigned char *)malloc(width * height * sizeof(unsigned char));
        nextGrid = (unsigned char *)malloc(width * height * sizeof(unsigned char));
        
        if (!currentGrid || !nextGrid) {
            fprintf(stderr, "Ошибка выделения памяти для полного поля\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        // Инициализация поля
        srand(time(NULL));
        if (mode == DEMO_MODE) {
            // Для демо используем планер
            gliderInitialization(currentGrid, width, height);
        } else {
            // Для измерения производительности используем случайное распределение
            randomInitialization(currentGrid, width, height);
        }
    }
    
    // Вычисление размера части поля для каждого процесса
    int rowsPerProcess = height / size;
    int localHeight = rowsPerProcess;
    
    // Распределяем оставшиеся строки
    if (rank < (height % size)) {
        localHeight++;
    }
    
    // Выделение памяти для локальных буферов с учетом строк-призраков
    int localBufferHeight = localHeight + 2;  // +2 для верхней и нижней граничных строк
    localCurrentGrid = (unsigned char *)malloc(width * localBufferHeight * sizeof(unsigned char));
    localNextGrid = (unsigned char *)malloc(width * localBufferHeight * sizeof(unsigned char));
    
    if (!localCurrentGrid || !localNextGrid) {
        fprintf(stderr, "Процесс %d: Ошибка выделения памяти для локального буфера\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    // Вычисление начальных строк для каждого процесса
    int *sendcounts = NULL;
    int *displs = NULL;
    int *startRows = NULL;
    int *endRows = NULL;
    
    if (rank == 0) {
        sendcounts = (int *)malloc(size * sizeof(int));
        displs = (int *)malloc(size * sizeof(int));
        startRows = (int *)malloc(size * sizeof(int));
        endRows = (int *)malloc(size * sizeof(int));
        
        if (!sendcounts || !displs || !startRows || !endRows) {
            fprintf(stderr, "Ошибка выделения памяти для служебных массивов\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        int currentRow = 0;
        for (int i = 0; i < size; i++) {
            startRows[i] = currentRow;
            
            int currentRowsPerProcess = rowsPerProcess;
            if (i < (height % size)) {
                currentRowsPerProcess++;
            }
            
            endRows[i] = currentRow + currentRowsPerProcess;
            sendcounts[i] = currentRowsPerProcess * width;
            displs[i] = currentRow * width;
            
            currentRow += currentRowsPerProcess;
        }
    }
    
    // Рассылка начальных данных всем процессам
    MPI_Scatter(startRows, 1, MPI_INT, &startRows, 1, MPI_INT, 0, MPI_COMM_WORLD);
    int startRow = startRows;
    
    // Рассылка слоев данных каждому процессу
    MPI_Scatterv(currentGrid, sendcounts, displs, MPI_UNSIGNED_CHAR,
                localCurrentGrid + width, localHeight * width, MPI_UNSIGNED_CHAR,
                0, MPI_COMM_WORLD);
    
    // Определение соседних процессов с учетом тороидальной структуры
    int prevRank = (rank - 1 + size) % size;
    int nextRank = (rank + 1) % size;
    
    // Буферы для проверки изменений
    bool *hasSendTopChanged = (bool *)calloc(steps, sizeof(bool));
    bool *hasSendBottomChanged = (bool *)calloc(steps, sizeof(bool));
    bool *hasLocalAreaChanged = (bool *)calloc(steps, sizeof(bool));
    
    // Установить начальное состояние первой итерации как "изменилось"
    hasSendTopChanged[0] = hasSendBottomChanged[0] = hasLocalAreaChanged[0] = true;
    
    // Запуск таймера для измерения производительности
    MPI_Barrier(MPI_COMM_WORLD);
    startTime = MPI_Wtime();
    
    // Основной цикл моделирования
    for (int step = 0; step < steps; step++) {
        MPI_Request sendTopRequest, sendBottomRequest, recvTopRequest, recvBottomRequest;
        
        // Обмен верхней и нижней границами с соседними процессами (если произошли изменения)
        if (step == 0 || hasSendTopChanged[step-1]) {
            MPI_Isend(localCurrentGrid + width, width, MPI_UNSIGNED_CHAR, prevRank, 0,
                    MPI_COMM_WORLD, &sendTopRequest);
        }
        
        if (step == 0 || hasSendBottomChanged[step-1]) {
            MPI_Isend(localCurrentGrid + localHeight * width, width, MPI_UNSIGNED_CHAR, nextRank, 1,
                    MPI_COMM_WORLD, &sendBottomRequest);
        }
        
        // Получение строк-призраков
        MPI_Irecv(localCurrentGrid, width, MPI_UNSIGNED_CHAR, prevRank, 1,
                MPI_COMM_WORLD, &recvTopRequest);
        
        MPI_Irecv(localCurrentGrid + (localHeight + 1) * width, width, MPI_UNSIGNED_CHAR, nextRank, 0,
                MPI_COMM_WORLD, &recvBottomRequest);
        
        // Ожидание завершения всех передач данных
        MPI_Wait(&recvTopRequest, MPI_STATUS_IGNORE);
        MPI_Wait(&recvBottomRequest, MPI_STATUS_IGNORE);
        
        if (step > 0 && hasSendTopChanged[step-1]) {
            MPI_Wait(&sendTopRequest, MPI_STATUS_IGNORE);
        }
        
        if (step > 0 && hasSendBottomChanged[step-1]) {
            MPI_Wait(&sendBottomRequest, MPI_STATUS_IGNORE);
        }
        
        // Вычисление следующего поколения для локальной области
        computeNextGeneration(localCurrentGrid, localNextGrid, width, localBufferHeight, 1, localHeight + 1);
        
        // Проверка, изменилась ли локальная область
        hasLocalAreaChanged[step] = hasAreaChanged(localCurrentGrid, localNextGrid, width, 1, localHeight + 1);
        
        // Проверка изменений верхней и нижней строк для следующей итерации
        if (step < steps - 1) {
            hasSendTopChanged[step] = hasAreaChanged(localCurrentGrid, localNextGrid, width, 1, 2);
            hasSendBottomChanged[step] = hasAreaChanged(localCurrentGrid, localNextGrid, width, localHeight, localHeight + 1);
        }
        
        // Сбор всего поля в корневом процессе для визуализации (только в демо-режиме)
        if (mode == DEMO_MODE) {
            MPI_Gatherv(localNextGrid + width, localHeight * width, MPI_UNSIGNED_CHAR,
                      currentGrid, sendcounts, displs, MPI_UNSIGNED_CHAR,
                      0, MPI_COMM_WORLD);
            
            if (rank == 0) {
                printGrid(currentGrid, width, height);
                printf("Шаг: %d/%d\n", step + 1, steps);
            }
        }
        
        // Обмен буферами для следующей итерации
        unsigned char *temp = localCurrentGrid;
        localCurrentGrid = localNextGrid;
        localNextGrid = temp;
    }
    
    // Сбор конечного состояния
    MPI_Gatherv(localCurrentGrid + width, localHeight * width, MPI_UNSIGNED_CHAR,
              currentGrid, sendcounts, displs, MPI_UNSIGNED_CHAR,
              0, MPI_COMM_WORLD);
    
    // Замер времени выполнения
    endTime = MPI_Wtime();
    
    // Вывод результатов измерения производительности
    if (rank == 0) {
        double elapsedTime = endTime - startTime;
        printf("Режим: %s\n", mode == DEMO_MODE ? "Демонстрационный" : "Измерение производительности");
        printf("Размер поля: %d x %d\n", width, height);
        printf("Количество итераций: %d\n", steps);
        printf("Количество процессов MPI: %d\n", size);
        printf("Количество потоков OpenMP на процесс: %d\n", omp_get_max_threads());
        printf("Общее время выполнения: %.4f сек\n", elapsedTime);
        printf("Производительность: %.2f миллионов клеток в секунду\n", 
               (double)(width * height * steps) / elapsedTime / 1000000.0);
    }
    
    // Освобождение памяти
    free(localCurrentGrid);
    free(localNextGrid);
    free(hasSendTopChanged);
    free(hasSendBottomChanged);
    free(hasLocalAreaChanged);
    
    if (rank == 0) {
        free(currentGrid);
        free(nextGrid);
        free(sendcounts);
        free(displs);
        free(startRows);
        free(endRows);
    }
    
    MPI_Finalize();
    return 0;
}
