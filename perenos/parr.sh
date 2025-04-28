mpicc parr_test.c -o parr_test -lm
mpirun -np 1 ./parr_test
python3 plot_solution_p.py
