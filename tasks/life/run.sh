#!/bin/bash

# Compile the program
mpicc -o game_of_life parr.c -pthread -O3

# Run in demo mode (small grid with visualization)
echo "Running in demo mode..."
mpirun -np 2 ./game_of_life 1 4

# Run in performance mode with different configurations
echo "Running performance tests..."

# 1 process, 4 threads
echo "1 process, 4 threads:"
mpirun -np 1 ./game_of_life 0 4

# 2 processes, 4 threads each
echo "2 processes, 4 threads each:"
mpirun -np 2 ./game_of_life 0 4

# 4 processes, 4 threads each
echo "4 processes, 4 threads each:"
mpirun -np 4 ./game_of_life 0 4

echo "Performance tests completed."