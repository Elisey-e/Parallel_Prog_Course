mpicc -o base base.c -lgmp
mpirun -np 1 ./base 1000
