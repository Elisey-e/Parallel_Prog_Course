mpicc -o parallel_merge_sort parallel_merge_sort.c

mpirun -np 4 ./parallel_merge_sort 1000000 1 50