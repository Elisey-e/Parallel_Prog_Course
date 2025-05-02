mpicc -o parent parent.c
mpicc -o child child.c

# Запуск с 2 родительскими процессами
mpirun -n 2 ./parent


# echo -----------------------
# mpirun -np 4 ./client_server
# echo -----------------------
# mpirun -np 4 ./one_sided
# echo -----------------------
# mpirun -np 4 ./collective_io