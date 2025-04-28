mpicc parr_test_full.c -o parr_test -lm
mpirun -np 8 ./parr_test
python3 plot_solution_p.py
