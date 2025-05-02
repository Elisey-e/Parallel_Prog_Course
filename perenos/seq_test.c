#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

// Parameters for the transport equation
#define A 1.0         // coefficient 'a' in the equation
#define T 1.0         // max time
#define X 1.0         // max x coordinate
#define K 1000        // number of time steps
#define M 100         // number of space steps
#define NUM_SNAPSHOTS 9  // Number of time snapshots to save
#define NUM_SNAP_SAVE 3

// Initial and boundary conditions
double phi(double x) {
    // Example: initial condition u(0,x) = gaussian pulse
    return exp(-(x - X / 2) * (x - X / 2) * 100);
}

double psi(double t) {
    // Example: boundary condition u(t,0) = 0
    return 0.0;
}

double f(double t, double x) {
    // Example: source term f(t,x) = 0
    return 0.0;
}

int main() {
    clock_t start_time, end_time;
    double cpu_time_used;
    
    start_time = clock(); // Засекаем время начала выполнения
    
    double tau = T / K;       // time step
    double h = X / M;         // space step
    
    // Check stability condition for cross scheme
    // For the cross scheme, the CFL condition is |a|*tau/h <= 1
    if (fabs(A) * tau / h > 1.0) {
        printf("Warning: Stability condition not satisfied (|A|*tau/h = %f)\n", fabs(A) * tau / h);
        printf("Solution may be unstable. Consider reducing tau or increasing h.\n");
    }
    
    // Allocate memory for all time layers
    double **u = (double **)malloc((K + 1) * sizeof(double *));
    for (int k = 0; k <= K; k++) {
        u[k] = (double *)malloc((M + 1) * sizeof(double));
    }
    
    // Set initial condition: u(0,x) = phi(x)
    for (int m = 0; m <= M; m++) {
        u[0][m] = phi(m * h);
    }
    
    // Set boundary condition: u(t,0) = psi(t)
    for (int k = 0; k <= K; k++) {
        u[k][0] = psi(k * tau);
    }
    
    // We need to compute the first time layer (k=1) using another method
    // Here we'll use a first-order forward time, central space scheme
    for (int m = 1; m < M; m++) {
        u[1][m] = u[0][m] - A * tau / (2 * h) * (u[0][m+1] - u[0][m-1]) + tau * f(0, m * h);
    }
    
    // Set boundary condition at right end (x=X) for all time steps
    for (int k = 0; k <= K; k++) {
        // You can use different boundary conditions at x=X if needed
        // For now, we'll use a simple extrapolation (zero gradient)
        u[k][M] = u[k][M-1];
    }
    
    // Solve using Cross scheme (central differences in time and space)
    // (u^(k+1)_m - u^(k-1)_m)/(2*τ) + a*(u^k_(m+1) - u^k_(m-1))/(2*h) = f^k_m
    for (int k = 1; k < K; k++) {
        for (int m = 1; m < M; m++) {
            u[k+1][m] = u[k-1][m] - A * tau / h * (u[k][m+1] - u[k][m-1]) + 2 * tau * f(k * tau, m * h);
        }
        // Update boundary at right end after each time step
        u[k+1][M] = u[k+1][M-1];
    }
    
    // Calculate time steps for snapshots
    int snapshot_steps[NUM_SNAPSHOTS + 1];
    for (int i = 0; i <= NUM_SNAPSHOTS; i++) {
        snapshot_steps[i] = i * K / NUM_SNAPSHOTS;
    }
    
    // Output solution at multiple time steps to file
    FILE *fp = fopen("transport_solution_multiple.csv", "w");
    if (fp == NULL) {
        printf("Error opening file for writing\n");
        return 1;
    }
    
    // Write header with time snapshots
    fprintf(fp, "x");
    for (int i = 0; i <= NUM_SNAPSHOTS; i++) {
        fprintf(fp, ",t_%d", i);
    }
    fprintf(fp, "\n");
    
    // Write data for each spatial point
    for (int m = 0; m <= M; m++) {
        fprintf(fp, "%f", m * h);  // x coordinate
        
        // Write solution values at different time steps
        for (int i = 0; i <= NUM_SNAP_SAVE; i++) {
            int k = snapshot_steps[i];
            fprintf(fp, ",%f", u[k][m]);
        }
        fprintf(fp, "\n");
    }
    
    fclose(fp);
    printf("Solution data saved to transport_solution_multiple.csv\n");
    
    // Free allocated memory
    for (int k = 0; k <= K; k++) {
        free(u[k]);
    }
    free(u);
    
    // Вычисляем и выводим время выполнения
    end_time = clock();
    cpu_time_used = ((double) (end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Program execution time: %.4f seconds\n", cpu_time_used);
    
    return 0;
}