#!/bin/bash

# Компиляция программы

mpicc parr_test_full.c -o parr_test -lm

# Создаем заголовок файла результатов
echo "K,M,processes,execution_time" > benchmark_results.csv

# Различные размеры задач
K_VALUES=(50000 100000 150000)
M_VALUES=(5000 10000 15000)

# Различные числа процессов
PROCESSES=(2 4 8 16)

# Запуск тестов
for k in "${K_VALUES[@]}"; do
    for m in "${M_VALUES[@]}"; do
        for p in "${PROCESSES[@]}"; do
            echo "Running K=$k, M=$m with $p processes..."
            mpirun -np $p ./parr_test $k $m
            sleep 1  # Пауза между запусками
        done
    done
done

echo "Benchmarking completed. Results saved to benchmark_results.csv"