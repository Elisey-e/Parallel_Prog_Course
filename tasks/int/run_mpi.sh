mpicc -o integration integration.c -lm
mpirun -np 4 ./integration 0 10 0.00001
