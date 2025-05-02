#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Parameters for the transport equation
#define A 1.0         // coefficient 'a' in the equation
#define T 1.0         // max time
#define X 1.0         // max x coordinate
#define K 1000        // number of time steps
#define M 100         // number of space steps

// Initial and boundary conditions
double phi(double x) {
    // Example: initial condition u(0,x) = sin(2*pi*x)
    return sin(2.0 * M_PI * x);
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
    double tau = T / K;       // time step
    double h = X / M;         // space step
    
    // Check stability condition for explicit scheme
    if (A * tau / h > 1.0) {
        printf("Warning: Stability condition not satisfied (A*tau/h = %f)\n", A * tau / h);
        printf("Solution may be unstable. Consider reducing tau or increasing h.\n");
    }
    
    // Allocate memory for two time layers
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
    
    // Solve using explicit left-corner scheme
    for (int k = 0; k < K; k++) {
        for (int m = 1; m <= M; m++) {
            u[k+1][m] = u[k][m] - A * tau / h * (u[k][m] - u[k][m-1]) + tau * f(k * tau, m * h);
        }
    }
    
    // Output initial state (t=0) and final state (t=T) to file
    FILE *fp = fopen("transport_solution.csv", "w");
    if (fp == NULL) {
        printf("Error opening file for writing\n");
        return 1;
    }
    
    // Write header
    fprintf(fp, "x,u_initial,u_final\n");
    
    // Write data
    for (int m = 0; m <= M; m++) {
        fprintf(fp, "%f,%f,%f\n", m * h, u[0][m], u[K][m]);
    }
    
    fclose(fp);
    printf("Solution data saved to transport_solution.csv\n");
    
    // Free allocated memory
    for (int k = 0; k <= K; k++) {
        free(u[k]);
    }
    free(u);
    
    return 0;
}