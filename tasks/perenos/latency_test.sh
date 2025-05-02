#!/bin/sh
#SBATCH -n 2
#SBATCH -o Hello-%j.out # STDOUT
#SBATCH -e Hello-%j.err # STDERR


mpirun -np 2 ./latency_test
