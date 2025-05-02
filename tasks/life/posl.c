/**
 * Игра "Жизнь" Конвея - последовательная реализация
 * 
 * Правила:
 * 1. Если рядом с "пустой" клеткой ровно 3 "живые" клетки, она становится "живой"
 * 2. Если у "живой" клетки 2 или 3 "живых" соседа, она остается "живой"
 * 3. В остальных случаях клетка становится/остается "пустой"
 * 
 * Поле имеет тороидальную топологию (края соединяются)
 * 
 * Режимы работы:
 * 1. Демонстрационный (небольшое поле, пошаговое отображение)
 * 2. Измерение производительности (большое поле, без отображения)
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <time.h>
 #include <unistd.h>
 #include <stdbool.h>
 
 // Размеры поля для демонстрационного режима
 #define DEMO_WIDTH 20
 #define DEMO_HEIGHT 20
 
 // Размеры поля для режима измерения производительности
 #define PERF_WIDTH 2000
 #define PERF_HEIGHT 2000
 
 // Количество временных шагов для режима производительности
 #define PERF_STEPS 1000
 
 // Задержка между шагами для демонстрационного режима (в микросекундах)
 #define DEMO_DELAY 100000
 
 // Типы начальных конфигураций
 typedef enum {
     RANDOM,     // Случайное распределение
     GLIDER,     // Планер
     BLINKER,    // Мигалка
     GLIDER_GUN  // Пушка, выстреливающая планеры
 } InitPattern;
 
 // Функция для инициализации поля
 void initializeField(bool **field, int width, int height, InitPattern pattern) {
     // Сначала очищаем поле
     for (int i = 0; i < height; i++) {
         for (int j = 0; j < width; j++) {
             field[i][j] = false;
         }
     }
     
     // Заполняем поле в зависимости от выбранного паттерна
     switch (pattern) {
         case RANDOM:
             // Случайное распределение, примерно 25% живых клеток
             for (int i = 0; i < height; i++) {
                 for (int j = 0; j < width; j++) {
                     field[i][j] = (rand() % 4 == 0);
                 }
             }
             break;
             
         case GLIDER:
             // Планер в левом верхнем углу
             if (width >= 3 && height >= 3) {
                 field[0][1] = true;
                 field[1][2] = true;
                 field[2][0] = true;
                 field[2][1] = true;
                 field[2][2] = true;
             }
             break;
             
         case BLINKER:
             // Мигалка в центре
             if (width >= 3 && height >= 3) {
                 int centerX = width / 2;
                 int centerY = height / 2;
                 field[centerY][centerX-1] = true;
                 field[centerY][centerX] = true;
                 field[centerY][centerX+1] = true;
             }
             break;
             
         case GLIDER_GUN:
             // Пушка Госпера, выстреливающая планеры
             if (width >= 40 && height >= 20) {
                 // Левый блок
                 field[5][1] = true;
                 field[5][2] = true;
                 field[6][1] = true;
                 field[6][2] = true;
                 
                 // Правый блок
                 field[3][35] = true;
                 field[3][36] = true;
                 field[4][35] = true;
                 field[4][36] = true;
                 
                 // Левая часть пушки
                 field[5][11] = true;
                 field[6][11] = true;
                 field[7][11] = true;
                 field[4][12] = true;
                 field[8][12] = true;
                 field[3][13] = true;
                 field[9][13] = true;
                 field[3][14] = true;
                 field[9][14] = true;
                 field[6][15] = true;
                 field[4][16] = true;
                 field[8][16] = true;
                 field[5][17] = true;
                 field[6][17] = true;
                 field[7][17] = true;
                 field[6][18] = true;
                 
                 // Правая часть пушки
                 field[3][21] = true;
                 field[4][21] = true;
                 field[5][21] = true;
                 field[3][22] = true;
                 field[4][22] = true;
                 field[5][22] = true;
                 field[2][23] = true;
                 field[6][23] = true;
                 field[1][25] = true;
                 field[2][25] = true;
                 field[6][25] = true;
                 field[7][25] = true;
             }
             break;
     }
 }
 
 // Вывод поля на экран в демонстрационном режиме
 void displayField(bool **field, int width, int height) {
     // Очистка экрана (ANSI escape code)
     printf("\033[H\033[J");
     
     for (int i = 0; i < height; i++) {
         for (int j = 0; j < width; j++) {
             printf("%c ", field[i][j] ? '#' : '.');
         }
         printf("\n");
     }
 }
 
 // Подсчет живых соседей для клетки с учетом тороидальной топологии
 int countLiveNeighbors(bool **field, int width, int height, int x, int y) {
     int count = 0;
     
     for (int dy = -1; dy <= 1; dy++) {
         for (int dx = -1; dx <= 1; dx++) {
             // Пропускаем саму клетку (dx=0, dy=0)
             if (dx == 0 && dy == 0) continue;
             
             // Вычисляем координаты соседа с учетом тороидальной топологии
             int nx = (x + dx + width) % width;
             int ny = (y + dy + height) % height;
             
             if (field[ny][nx]) count++;
         }
     }
     
     return count;
 }
 
 // Выполнение одного шага эволюции
 void evolveField(bool **current, bool **next, int width, int height) {
     for (int y = 0; y < height; y++) {
         for (int x = 0; x < width; x++) {
             int neighbors = countLiveNeighbors(current, width, height, x, y);
             
             if (current[y][x]) {
                 // Правило для живой клетки
                 next[y][x] = (neighbors == 2 || neighbors == 3);
             } else {
                 // Правило для пустой клетки
                 next[y][x] = (neighbors == 3);
             }
         }
     }
 }
 
 // Подсчет количества живых клеток на поле
 int countLiveCells(bool **field, int width, int height) {
     int count = 0;
     
     for (int y = 0; y < height; y++) {
         for (int x = 0; x < width; x++) {
             if (field[y][x]) count++;
         }
     }
     
     return count;
 }
 
 // Проверка, изменилось ли поле
 bool hasFieldChanged(bool **field1, bool **field2, int width, int height) {
     for (int y = 0; y < height; y++) {
         for (int x = 0; x < width; x++) {
             if (field1[y][x] != field2[y][x]) {
                 return true;
             }
         }
     }
     return false;
 }
 
 // Основная функция для демонстрационного режима
 void demoMode() {
     int width = DEMO_WIDTH;
     int height = DEMO_HEIGHT;
     
     // Выделяем память для двух полей (текущее и следующее)
     bool **currentField = (bool**)malloc(height * sizeof(bool*));
     bool **nextField = (bool**)malloc(height * sizeof(bool*));
     
     for (int i = 0; i < height; i++) {
         currentField[i] = (bool*)malloc(width * sizeof(bool));
         nextField[i] = (bool*)malloc(width * sizeof(bool));
     }
     
     // Выбор начальной конфигурации
     printf("Выберите начальную конфигурацию:\n");
     printf("1. Случайное распределение\n");
     printf("2. Планер\n");
     printf("3. Мигалка\n");
     printf("4. Пушка Госпера\n");
     
     int choice;
     scanf("%d", &choice);
     
     InitPattern pattern;
     switch (choice) {
         case 2: pattern = GLIDER; break;
         case 3: pattern = BLINKER; break;
         case 4: pattern = GLIDER_GUN; break;
         default: pattern = RANDOM; break;
     }
     
     // Инициализация поля
     initializeField(currentField, width, height, pattern);
     
     int generation = 0;
     bool stable = false;
     
     // Основной цикл эволюции
     while (!stable) {
         // Отображаем текущее состояние поля
         displayField(currentField, width, height);
         printf("Поколение: %d, Живых клеток: %d\n", 
                generation, countLiveCells(currentField, width, height));
         
         // Вычисляем следующее поколение
         evolveField(currentField, nextField, width, height);
         
         // Проверяем, изменилось ли поле
         stable = !hasFieldChanged(currentField, nextField, width, height);
         
         // Меняем местами текущее и следующее поля
         bool **temp = currentField;
         currentField = nextField;
         nextField = temp;
         
         generation++;
         
         // Задержка для визуализации
         usleep(DEMO_DELAY);
         
         // Проверка на нажатие клавиши для выхода
         printf("Нажмите Enter для продолжения, 'q' и Enter для выхода...\n");
         char c = getchar();
         if (c == 'q') break;
     }
     
     // Освобождаем память
     for (int i = 0; i < height; i++) {
         free(currentField[i]);
         free(nextField[i]);
     }
     free(currentField);
     free(nextField);
     
     if (stable) {
         printf("Поле стабилизировалось после %d поколений.\n", generation);
     }
 }
 
 // Основная функция для режима измерения производительности
 void performanceMode() {
     int width = PERF_WIDTH;
     int height = PERF_HEIGHT;
     int steps = PERF_STEPS;
     
     printf("Запуск режима измерения производительности...\n");
     printf("Размер поля: %dx%d, количество шагов: %d\n", width, height, steps);
     
     // Выделяем память для двух полей (текущее и следующее)
     bool **currentField = (bool**)malloc(height * sizeof(bool*));
     bool **nextField = (bool**)malloc(height * sizeof(bool*));
     
     for (int i = 0; i < height; i++) {
         currentField[i] = (bool*)malloc(width * sizeof(bool));
         nextField[i] = (bool*)malloc(width * sizeof(bool));
     }
     
     // Инициализация поля случайным образом
     initializeField(currentField, width, height, RANDOM);
     
     // Замеряем время выполнения
     clock_t start = clock();
     
     for (int step = 0; step < steps; step++) {
         // Вычисляем следующее поколение
         evolveField(currentField, nextField, width, height);
         
         // Проверяем, не стабилизировалось ли поле
         if (!hasFieldChanged(currentField, nextField, width, height)) {
             printf("Поле стабилизировалось после %d шагов.\n", step + 1);
             break;
         }
         
         // Меняем местами текущее и следующее поля
         bool **temp = currentField;
         currentField = nextField;
         nextField = temp;
         
         // Выводим прогресс каждые 100 шагов
         if ((step + 1) % 100 == 0) {
             printf("Выполнено шагов: %d/%d\n", step + 1, steps);
         }
     }
     
     clock_t end = clock();
     double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
     
     printf("Выполнение завершено.\n");
     printf("Общее время выполнения: %.2f секунд\n", time_spent);
     printf("Время на один шаг: %.5f секунд\n", time_spent / steps);
     printf("Количество живых клеток в конечном состоянии: %d\n", 
            countLiveCells(currentField, width, height));
     
     // Освобождаем память
     for (int i = 0; i < height; i++) {
         free(currentField[i]);
         free(nextField[i]);
     }
     free(currentField);
     free(nextField);
 }
 
 int main() {
     // Инициализация генератора случайных чисел
     srand(time(NULL));
     
     printf("Игра 'Жизнь' Джона Конвея\n");
     printf("==========================\n\n");
     printf("Выберите режим работы:\n");
     printf("1. Демонстрационный режим\n");
     printf("2. Режим измерения производительности\n");
     
     int choice;
     scanf("%d", &choice);
     getchar(); // Считываем символ новой строки после ввода
     
     if (choice == 2) {
         performanceMode();
     } else {
         demoMode();
     }
     
     return 0;
 }