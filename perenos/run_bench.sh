#!/bin/bash

# Компилируем программу
mpicc parr_test.c -o parr_test -lm

# Создаем заголовок для файла с результатами
echo "processes,execution_time" > execution_times.csv

# Запускаем для разного числа процессов (от 3 до 16)
for procs in {2..3}
do
    echo "Running with $procs processes..."
    mpirun -np $procs ./parr_test >> execution_times.csv
done

echo "Benchmark completed. Results saved to execution_times.csv"