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
    int *L = (int *)malloc(n1 * sizeof(int));
    int *R = (int *)malloc(n2 * sizeof(int));
    
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
    
    // Копируем оставшиеся элементы L[], если они есть
    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }
    
    // Копируем оставшиеся элементы R[], если они есть
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

// Функция для заполнения массива случайными числами
void fillArrayRandom(int *arr, int size) {
    for (int i = 0; i < size; i++) {
        arr[i] = rand() % 10000;
    }
}

// Функция для заполнения массива отсортированными числами (лучший случай)
void fillArraySorted(int *arr, int size) {
    for (int i = 0; i < size; i++) {
        arr[i] = i;
    }
}

// Функция для заполнения массива числами в обратном порядке (худший случай)
void fillArrayReversed(int *arr, int size) {
    for (int i = 0; i < size; i++) {
        arr[i] = size - i;
    }
}

// Проверка правильности сортировки
int isSorted(int* arr, int size) {
    for (int i = 1; i < size; i++) {
        if (arr[i] < arr[i-1]) {
            return 0;
        }
    }
    return 1;
}

// Функция для сохранения результатов в CSV файл
void saveResultToCSV(const char* filename, int array_size, int num_processes, int case_type, 
                     double seq_time, double par_time, double speedup) {
    FILE *fp;
    int fileExists = 0;
    
    // Проверяем, существует ли файл
    fp = fopen(filename, "r");
    if (fp) {
        fileExists = 1;
        fclose(fp);
    }
    
    // Открываем файл для добавления результатов
    fp = fopen(filename, "a");
    if (!fp) {
        printf("Ошибка при открытии файла %s\n", filename);
        return;
    }
    
    // Если файл новый, запишем заголовки столбцов
    if (!fileExists) {
        fprintf(fp, "Размер массива,Число процессов,Тип случая,Время последовательной сортировки (с),"
                    "Время параллельной сортировки (с),Ускорение\n");
    }
    
    // Запишем данные
    const char* case_name = case_type == 0 ? "Типовой" : case_type == 1 ? "Лучший" : "Худший";
    fprintf(fp, "%d,%d,%s,%f,%f,%f\n", 
            array_size, num_processes, case_name, seq_time, par_time, speedup);
    
    fclose(fp);
    printf("Результаты сохранены в файл %s\n", filename);
}

// Функция для тестирования одной комбинации параметров
void runSingleTest(int rank, int world_size, int array_size, int case_type) {
    int* data = NULL;
    int* local_data = NULL;
    int local_n;
    double start_time, end_time, seq_time = 0, par_time = 0;
    
    if (rank == 0) {
        printf("\n===== Тестирование =====\n");
        printf("Размер массива: %d\n", array_size);
        printf("Число процессов: %d\n", world_size);
        printf("Тип случая: %s\n", case_type == 0 ? "типовой" : case_type == 1 ? "лучший" : "худший");
        
        // Выделяем память для массива
        data = (int*)malloc(array_size * sizeof(int));
        
        // Заполняем массив в зависимости от типа случая
        srand(time(NULL));
        if (case_type == 0) {
            fillArrayRandom(data, array_size);
        } else if (case_type == 1) {
            fillArraySorted(data, array_size);
        } else {
            fillArrayReversed(data, array_size);
        }
        
        // Создаем копию для последовательной сортировки
        int* seq_data = (int*)malloc(array_size * sizeof(int));
        memcpy(seq_data, data, array_size * sizeof(int));
        
        // Замеряем время последовательной сортировки
        start_time = MPI_Wtime();
        mergeSort(seq_data, 0, array_size - 1);
        end_time = MPI_Wtime();
        seq_time = end_time - start_time;
        
        printf("Время последовательной сортировки: %f секунд\n", seq_time);
        
        // Проверяем правильность последовательной сортировки
        if (isSorted(seq_data, array_size)) {
            printf("Последовательная сортировка выполнена правильно\n");
        } else {
            printf("Ошибка в последовательной сортировке\n");
        }
        
        free(seq_data);
    }
    
    // Сообщаем всем процессам размер массива
    MPI_Bcast(&array_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Рассчитываем размер локального массива для каждого процесса
    local_n = array_size / world_size;
    local_data = (int*)malloc(local_n * sizeof(int));
    
    // Замеряем время параллельной сортировки
    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();
    
    // Распределяем данные по процессам
    MPI_Scatter(data, local_n, MPI_INT, local_data, local_n, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Сортируем локальный массив
    mergeSort(local_data, 0, local_n - 1);
    
    // Массив для сбора отсортированных данных
    int* sorted_data = NULL;
    if (rank == 0) {
        sorted_data = (int*)malloc(array_size * sizeof(int));
    }
    
    // Собираем отсортированные части обратно на процесс 0
    MPI_Gather(local_data, local_n, MPI_INT, sorted_data, local_n, MPI_INT, 0, MPI_COMM_WORLD);
    
    // На корневом процессе выполняем финальное слияние
    if (rank == 0) {
        // Слияние частей массива
        for (int i = 0; i < world_size; i += 2) {
            int start = i * local_n;
            int mid = (i + 1) * local_n - 1;
            int end = (i + 2) * local_n - 1;
            
            if (i + 1 < world_size) {
                if (end >= array_size) end = array_size - 1;
                merge(sorted_data, start, mid, end);
            }
        }
        
        // Продолжаем слияние, пока не получим полностью отсортированный массив
        int step = 2 * local_n;
        while (step < array_size) {
            for (int i = 0; i < array_size; i += 2 * step) {
                int start = i;
                int mid = i + step - 1;
                int end = i + 2 * step - 1;
                
                if (mid < array_size) {
                    if (end >= array_size) end = array_size - 1;
                    merge(sorted_data, start, mid, end);
                }
            }
            step *= 2;
        }
        
        end_time = MPI_Wtime();
        par_time = end_time - start_time;
        
        printf("Время параллельной сортировки: %f секунд\n", par_time);
        double speedup = seq_time / par_time;
        printf("Ускорение: %f\n", speedup);
        
        // Проверяем правильность параллельной сортировки
        if (isSorted(sorted_data, array_size)) {
            printf("Параллельная сортировка выполнена правильно\n");
        } else {
            printf("Ошибка в параллельной сортировке\n");
        }
        
        // Сохраняем результаты в CSV файл
        saveResultToCSV("sorting_results.csv", array_size, world_size, case_type, seq_time, par_time, speedup);
        
        free(sorted_data);
        free(data);
    }
    
    free(local_data);
}

int main(int argc, char** argv) {
    int rank, size;
    
    // Инициализация MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Определяем режим работы: обычный (один тест) или полное тестирование
    int test_mode = 0;
    int single_array_size = 1000000;
    int single_case_type = 0;
    
    if (argc > 1) {
        if (strcmp(argv[1], "test") == 0) {
            test_mode = 1;
        } else {
            single_array_size = atoi(argv[1]);
            if (argc > 2) {
                single_case_type = atoi(argv[2]);
            }
        }
    }
    
    if (test_mode) {
        // Массивы с параметрами для тестирования
        int array_sizes[] = {100000, 500000, 1000000, 5000000};
        int num_sizes = sizeof(array_sizes) / sizeof(array_sizes[0]);
        
        // Выполняем тесты для всех комбинаций параметров
        for (int size_idx = 0; size_idx < num_sizes; size_idx++) {
            for (int case_type = 0; case_type < 3; case_type++) {
                runSingleTest(rank, size, array_sizes[size_idx], case_type);
            }
        }
    } else {
        // Выполняем один тест с указанными параметрами
        runSingleTest(rank, size, single_array_size, single_case_type);
    }
    
    MPI_Finalize();
    return 0;
}