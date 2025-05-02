#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <mpi.h>

// Функция для слияния двух отсортированных массивов
void merge(int *arr, int l, int m, int r) {
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;
    
    // Создаем временные массивы
    int *L = (int*)malloc(n1 * sizeof(int));
    int *R = (int*)malloc(n2 * sizeof(int));
    
    // Копируем данные во временные массивы L[] и R[]
    for (i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];
    
    // Слияние временных массивов обратно в arr[l..r]
    i = 0;
    j = 0;
    k = l;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        } else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }
    
    // Копируем оставшиеся элементы L[], если таковые есть
    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }
    
    // Копируем оставшиеся элементы R[], если таковые есть
    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }
    
    free(L);
    free(R);
}

// Последовательная сортировка слиянием
void mergeSort(int *arr, int l, int r) {
    if (l < r) {
        int m = l + (r - l) / 2;
        
        // Сортируем первую и вторую половины
        mergeSort(arr, l, m);
        mergeSort(arr, m + 1, r);
        
        // Слияние отсортированных половин
        merge(arr, l, m, r);
    }
}

// Быстрая сортировка для комбинированного алгоритма
void quickSort(int *arr, int low, int high) {
    if (low < high) {
        // Разделение
        int pivot = arr[high];
        int i = (low - 1);
        
        for (int j = low; j <= high - 1; j++) {
            if (arr[j] < pivot) {
                i++;
                // Обмен элементов
                int temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
            }
        }
        // Обмен с элементом pivot
        int temp = arr[i + 1];
        arr[i + 1] = arr[high];
        arr[high] = temp;
        int pi = i + 1;
        
        // Рекурсивно сортируем элементы до и после разделения
        quickSort(arr, low, pi - 1);
        quickSort(arr, pi + 1, high);
    }
}

// Генерация массива для разных случаев
void generateArray(int *arr, int size, int caseType) {
    int i;
    switch (caseType) {
        case 0: // Лучший случай - уже отсортированный массив
            for (i = 0; i < size; i++) {
                arr[i] = i;
            }
            break;
        case 1: // Типовой случай - случайный массив
            srand(time(NULL));
            for (i = 0; i < size; i++) {
                arr[i] = rand() % size;
            }
            break;
        case 2: // Худший случай - обратно отсортированный массив
            for (i = 0; i < size; i++) {
                arr[i] = size - i - 1;
            }
            break;
        default:
            printf("Неизвестный тип случая\n");
            exit(1);
    }
}

// Проверка правильности сортировки
int isSorted(int *arr, int size) {
    for (int i = 1; i < size; i++) {
        if (arr[i - 1] > arr[i]) {
            return 0;
        }
    }
    return 1;
}

// Комбинированный алгоритм: быстрая сортировка для малых частей, сортировка слиянием для остальных
void hybridSort(int *arr, int l, int r, int threshold) {
    if (l < r) {
        if (r - l <= threshold) {
            // Используем быструю сортировку для малых массивов
            quickSort(arr, l, r);
        } else {
            // Используем сортировку слиянием для больших массивов
            int m = l + (r - l) / 2;
            hybridSort(arr, l, m, threshold);
            hybridSort(arr, m + 1, r, threshold);
            merge(arr, l, m, r);
        }
    }
}

int main(int argc, char *argv[]) {
    int rank, size, n, local_n;
    int *data = NULL, *local_data = NULL;
    int *sorted_data = NULL, *result = NULL;
    double start_time, end_time;
    double seq_time, par_time;
    int caseType = 1; // По умолчанию типовой случай
    int arraySize = 10000; // По умолчанию размер массива
    int threshold = 50; // Порог для гибридной сортировки
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Обработка аргументов командной строки
    if (argc > 1) {
        arraySize = atoi(argv[1]);
    }
    if (argc > 2) {
        caseType = atoi(argv[2]);
    }
    if (argc > 3) {
        threshold = atoi(argv[3]);
    }
    
    // Вычисляем количество элементов для каждого процесса
    n = arraySize;
    local_n = n / size;
    
    // Выделяем память для локальных данных
    local_data = (int*)malloc(local_n * sizeof(int));
    
    // Процесс 0 инициализирует массив и распределяет данные
    if (rank == 0) {
        printf("Размер массива: %d, Тип случая: %d, Количество процессов: %d\n", n, caseType, size);
        
        // Выделяем память для данных и результата на процессе 0
        data = (int*)malloc(n * sizeof(int));
        result = (int*)malloc(n * sizeof(int));
        
        // Генерируем массив в зависимости от типа случая
        generateArray(data, n, caseType);
        
        // Последовательная сортировка для сравнения времени
        sorted_data = (int*)malloc(n * sizeof(int));
        memcpy(sorted_data, data, n * sizeof(int));
        
        printf("Запуск последовательной сортировки слиянием...\n");
        start_time = MPI_Wtime();
        mergeSort(sorted_data, 0, n - 1);
        end_time = MPI_Wtime();
        seq_time = end_time - start_time;
        printf("Время последовательной сортировки слиянием: %.6f секунд\n", seq_time);
        
        // Проверка правильности сортировки
        if (isSorted(sorted_data, n)) {
            printf("Последовательная сортировка выполнена корректно\n");
        } else {
            ;
        }
        
        // Очищаем отсортированный массив и повторно копируем исходные данные
        memcpy(sorted_data, data, n * sizeof(int));
        
        printf("Запуск последовательной гибридной сортировки...\n");
        start_time = MPI_Wtime();
        hybridSort(sorted_data, 0, n - 1, threshold);
        end_time = MPI_Wtime();
        double hybrid_seq_time = end_time - start_time;
        printf("Время последовательной гибридной сортировки: %.6f секунд\n", hybrid_seq_time);
        
        // Проверка правильности сортировки
        if (isSorted(sorted_data, n)) {
            printf("Последовательная гибридная сортировка выполнена корректно\n");
        } else {
            ;
        }
        
        free(sorted_data);
        
        printf("Запуск параллельной сортировки...\n");
    }
    
    // Запуск таймера для параллельной версии
    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();
    
    // Распределяем данные между процессами
    if (rank == 0) {
        // Распределение данных
        for (int i = 1; i < size; i++) {
            MPI_Send(&data[i * local_n], local_n, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
        // Копируем данные для процесса 0
        memcpy(local_data, data, local_n * sizeof(int));
    } else {
        // Принимаем данные
        MPI_Recv(local_data, local_n, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    
    // Каждый процесс выполняет локальную сортировку
    mergeSort(local_data, 0, local_n - 1);
    
    // Собираем отсортированные данные
    if (rank == 0) {
        // Копируем отсортированные данные процесса 0
        memcpy(result, local_data, local_n * sizeof(int));
        
        // Получаем и сливаем данные от других процессов
        int *temp = (int*)malloc(local_n * sizeof(int));
        int *merged = (int*)malloc(n * sizeof(int));
        
        for (int i = 1; i < size; i++) {
            MPI_Recv(temp, local_n, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            // Сливаем полученные данные с текущим результатом
            memcpy(merged, result, i * local_n * sizeof(int));
            merge(merged, 0, i * local_n - 1, (i + 1) * local_n - 1);
            memcpy(result, merged, (i + 1) * local_n * sizeof(int));
        }
        
        free(temp);
        free(merged);
    } else {
        // Отправляем отсортированные данные процессу 0
        MPI_Send(local_data, local_n, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }
    
    // Остановка таймера
    MPI_Barrier(MPI_COMM_WORLD);
    end_time = MPI_Wtime();
    par_time = end_time - start_time;
    
    // Вывод результатов на процессе 0
    if (rank == 0) {
        printf("Время параллельной сортировки: %.6f секунд\n", par_time);
        printf("Ускорение: %.2f\n", seq_time / par_time);
        
        // Проверка правильности параллельной сортировки
        if (isSorted(result, n)) {
            printf("Параллельная сортировка выполнена корректно\n");
        } else {
            ;
        }
        
        // Теперь запустим параллельную гибридную сортировку
        printf("Запуск параллельной гибридной сортировки...\n");
    }
    
    // Запуск таймера для параллельной гибридной версии
    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();
    
    // Распределяем данные между процессами для гибридной сортировки
    if (rank == 0) {
        // Распределение данных
        for (int i = 1; i < size; i++) {
            MPI_Send(&data[i * local_n], local_n, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
        // Копируем данные для процесса 0
        memcpy(local_data, data, local_n * sizeof(int));
    } else {
        // Принимаем данные
        MPI_Recv(local_data, local_n, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    
    // Каждый процесс выполняет локальную гибридную сортировку
    hybridSort(local_data, 0, local_n - 1, threshold);
    
    // Собираем отсортированные данные
    if (rank == 0) {
        // Копируем отсортированные данные процесса 0
        memcpy(result, local_data, local_n * sizeof(int));
        
        // Получаем и сливаем данные от других процессов
        int *temp = (int*)malloc(local_n * sizeof(int));
        int *merged = (int*)malloc(n * sizeof(int));
        
        for (int i = 1; i < size; i++) {
            MPI_Recv(temp, local_n, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            // Сливаем полученные данные с текущим результатом
            memcpy(merged, result, i * local_n * sizeof(int));
            merge(merged, 0, i * local_n - 1, (i + 1) * local_n - 1);
            memcpy(result, merged, (i + 1) * local_n * sizeof(int));
        }
        
        free(temp);
        free(merged);
    } else {
        // Отправляем отсортированные данные процессу 0
        MPI_Send(local_data, local_n, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }
    
    // Остановка таймера
    MPI_Barrier(MPI_COMM_WORLD);
    end_time = MPI_Wtime();
    double hybrid_par_time = end_time - start_time;
    
    // Вывод результатов на процессе 0
    if (rank == 0) {
        printf("Время параллельной гибридной сортировки: %.6f секунд\n", hybrid_par_time);
        printf("Ускорение гибридного алгоритма: %.2f\n", seq_time / hybrid_par_time);
        
        // Проверка правильности параллельной гибридной сортировки
        if (isSorted(result, n)) {
            printf("Параллельная гибридная сортировка выполнена корректно\n");
        } else {
            ;
        }
        
        // Освобождаем ресурсы
        free(data);
        free(result);
    }
    
    free(local_data);
    MPI_Finalize();
    return 0;
}