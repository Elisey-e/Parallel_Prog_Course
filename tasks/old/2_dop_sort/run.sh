mpicc -o test_runner parallel_sort_runner.c

mpirun -np 4 ./test_runner test