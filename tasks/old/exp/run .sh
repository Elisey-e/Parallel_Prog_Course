mpicc -o base base.c -lgmp
mpirun -np 4 ./base 1000  # 1000 знаков после запятой
