#!/bin/sh
#SBATCH -n 8
#SBATCH -o Hello-%j.out # STDOUT
#SBATCH -e Hello-%j.err # STDERR

mpirun -np 4 ./client_server
