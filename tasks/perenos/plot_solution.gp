set terminal png size 800,600
set output 'transport_equation_solution.png'
set title 'Solution of Transport Equation'
set xlabel 'x'
set ylabel 'u(t,x)'
set grid
plot 'initial_solution.txt' using 1:2 with lines lw 2 title 'Initial t=0', \
     'final_solution.txt' using 1:2 with lines lw 2 title 'Final t=1.000000'
