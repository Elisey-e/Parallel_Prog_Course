mpicc mpi_transport.c -o mpi_transport -lm
mpirun -np 4 ./mpi_transport