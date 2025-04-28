#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

// Parameters for the transport equation
#define A 1.0         // coefficient 'a' in the equation
#define T 1.0         // max time
#define X 1.0         // max x coordinate
#define NUM_SNAPSHOTS 9  // Number of time snapshots to save
#define NUM_SNAP_SAVE 3

#define DEFAULT_K 100000
#define DEFAULT_M 100000

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

int main(int argc, char *argv[]) {
    int rank, size;
    double start_time, end_time;
    int K = DEFAULT_K, M = DEFAULT_M;
    double tau = T / K;       // time step
    double h = X / M;         // space step

    if (argc == 3) {
        K = atoi(argv[1]);
        M = atoi(argv[2]);
    }

    
    // Initialize MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Start timing (only rank 0 needs to track the total time)
    if (rank == 0) {
        start_time = MPI_Wtime();
    }
    
    // Check stability condition for cross scheme
    if (rank == 0) {
        if (fabs(A) * tau / h > 1.0) {
            printf("Warning: Stability condition not satisfied (|A|*tau/h = %f)\n", fabs(A) * tau / h);
            printf("Solution may be unstable. Consider reducing tau or increasing h.\n");
        }
    }
    
    // Calculate domain decomposition
    // Each process handles a portion of the spatial domain
    int points_per_proc = (M + 1) / size;
    int remainder = (M + 1) % size;
    
    // Calculate local domain boundaries
    int local_start = rank * points_per_proc;
    int local_count = points_per_proc;
    
    // Distribute remainder points among the first 'remainder' processes
    if (rank < remainder) {
        local_start += rank;
        local_count += 1;
    } else {
        local_start += remainder;
    }
    
    // Add ghost cells for communication
    int ghost_cells_left = (rank > 0) ? 1 : 0;
    int ghost_cells_right = (rank < size - 1) ? 1 : 0;
    
    // Total local domain size including ghost cells
    int local_size = local_count + ghost_cells_left + ghost_cells_right;
    
    // Allocate memory for solution
    // We need to store 3 time layers (prev, current, next)
    double *u_prev = (double *)malloc(local_size * sizeof(double));
    double *u_curr = (double *)malloc(local_size * sizeof(double));
    double *u_next = (double *)malloc(local_size * sizeof(double));
    
    // Initialize solution - set initial condition at t=0
    for (int i = 0; i < local_size; i++) {
        // Convert local index to global
        int global_idx = local_start + i - ghost_cells_left;
        
        // Initialize with proper initial condition
        if (i >= ghost_cells_left && i < local_size - ghost_cells_right) {
            u_prev[i] = phi(global_idx * h);
        }
    }
    
    // Set boundary condition at x=0 (only for process 0)
    if (rank == 0) {
        u_prev[ghost_cells_left] = psi(0);
    }
    
    // Communication buffers for ghost cells
    double send_buf, recv_buf;
    MPI_Status status;
    
    // First time step calculation (t=1)
    // Exchange ghost cells for t=0 layer
    if (rank > 0) {
        // Send leftmost point to left neighbor
        send_buf = u_prev[ghost_cells_left];
        MPI_Send(&send_buf, 1, MPI_DOUBLE, rank-1, 0, MPI_COMM_WORLD);
        
        // Receive from left neighbor
        MPI_Recv(&recv_buf, 1, MPI_DOUBLE, rank-1, 0, MPI_COMM_WORLD, &status);
        u_prev[0] = recv_buf;
    }
    
    if (rank < size - 1) {
        // Send rightmost point to right neighbor
        send_buf = u_prev[local_size - ghost_cells_right - 1];
        MPI_Send(&send_buf, 1, MPI_DOUBLE, rank+1, 0, MPI_COMM_WORLD);
        
        // Receive from right neighbor
        MPI_Recv(&recv_buf, 1, MPI_DOUBLE, rank+1, 0, MPI_COMM_WORLD, &status);
        u_prev[local_size - 1] = recv_buf;
    }
    
    // Calculate first time step (t=1) using forward time, central space scheme
    for (int i = ghost_cells_left; i < local_size - ghost_cells_right; i++) {
        if (i == ghost_cells_left && rank == 0) {
            // Left boundary condition
            u_curr[i] = psi(tau);
        } else {
            // Interior points (and right boundary for last process)
            int global_idx = local_start + i - ghost_cells_left;
            double x = global_idx * h;
            
            // Forward time, central space scheme for first time step
            u_curr[i] = u_prev[i] - A * tau / (2 * h) * (u_prev[i+1] - u_prev[i-1]) + tau * f(0, x);
        }
    }
    
    // Allocate memory for snapshots collection
    double *global_solution = NULL;
    int *recvcounts = NULL;
    int *displs = NULL;
    
    if (rank == 0) {
        global_solution = (double *)malloc((M + 1) * sizeof(double));
        recvcounts = (int *)malloc(size * sizeof(int));
        displs = (int *)malloc(size * sizeof(int));
        
        // Calculate recvcounts and displacements for MPI_Gatherv
        for (int i = 0; i < size; i++) {
            recvcounts[i] = (M + 1) / size;
            if (i < remainder) {
                recvcounts[i]++;
            }
            
            displs[i] = (i == 0) ? 0 : displs[i-1] + recvcounts[i-1];
        }
    }
    
    // Collect initial solution (t=0) for output
    MPI_Gatherv(&u_prev[ghost_cells_left], local_count, MPI_DOUBLE,
                global_solution, recvcounts, displs, MPI_DOUBLE,
                0, MPI_COMM_WORLD);
    
    // Store snapshots in memory (only on rank 0)
    double **snapshots = NULL;
    if (rank == 0) {
        snapshots = (double **)malloc((NUM_SNAPSHOTS + 1) * sizeof(double *));
        for (int s = 0; s <= NUM_SNAPSHOTS; s++) {
            snapshots[s] = (double *)malloc((M + 1) * sizeof(double));
        }
        
        // Store initial solution as first snapshot
        for (int i = 0; i <= M; i++) {
            snapshots[0][i] = global_solution[i];
        }
    }
    
    // Main time stepping loop
    for (int k = 1; k < K; k++) {
        // Exchange ghost cells for current time layer
        if (rank > 0) {
            // Send leftmost point to left neighbor
            send_buf = u_curr[ghost_cells_left];
            MPI_Send(&send_buf, 1, MPI_DOUBLE, rank-1, 0, MPI_COMM_WORLD);
            
            // Receive from left neighbor
            MPI_Recv(&recv_buf, 1, MPI_DOUBLE, rank-1, 0, MPI_COMM_WORLD, &status);
            u_curr[0] = recv_buf;
        }
        
        if (rank < size - 1) {
            // Send rightmost point to right neighbor
            send_buf = u_curr[local_size - ghost_cells_right - 1];
            MPI_Send(&send_buf, 1, MPI_DOUBLE, rank+1, 0, MPI_COMM_WORLD);
            
            // Receive from right neighbor
            MPI_Recv(&recv_buf, 1, MPI_DOUBLE, rank+1, 0, MPI_COMM_WORLD, &status);
            u_curr[local_size - 1] = recv_buf;
        }
        
        // Calculate next time step (t=k+1) using cross scheme
        for (int i = ghost_cells_left; i < local_size - ghost_cells_right; i++) {
            if (i == ghost_cells_left && rank == 0) {
                // Left boundary condition
                u_next[i] = psi((k+1) * tau);
            } else {
                // Interior points (and right boundary for last process)
                int global_idx = local_start + i - ghost_cells_left;
                double x = global_idx * h;
                
                // Cross scheme for next time step
                u_next[i] = u_prev[i] - A * tau / h * (u_curr[i+1] - u_curr[i-1]) + 
                           2 * tau * f(k * tau, x);
            }
        }
        
        // Save snapshots at specified intervals
        if (k % (K / NUM_SNAPSHOTS) == 0) {
            int snapshot_idx = k / (K / NUM_SNAPSHOTS);
            
            // Collect solution for this snapshot
            MPI_Gatherv(&u_next[ghost_cells_left], local_count, MPI_DOUBLE,
                        global_solution, recvcounts, displs, MPI_DOUBLE,
                        0, MPI_COMM_WORLD);
            
            // Store snapshot in memory (only on rank 0)
            if (rank == 0) {
                for (int i = 0; i <= M; i++) {
                    snapshots[snapshot_idx][i] = global_solution[i];
                }
            }
        }
        
        // Rotate time layers (prev <- curr <- next)
        double *temp = u_prev;
        u_prev = u_curr;
        u_curr = u_next;
        u_next = temp;
    }
    
    // Write all snapshots to file (only on rank 0)
    if (rank == 0) {
        FILE *outfile = fopen("transport_solution_multiple_mpi.csv", "w");
        if (!outfile) {
            printf("Error opening output file\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        // Write header
        fprintf(outfile, "x");
        for (int s = 0; s <= NUM_SNAPSHOTS; s++) {
            fprintf(outfile, ",t_%d", s);
        }
        fprintf(outfile, "\n");
        
        // Write all snapshots
        for (int i = 0; i <= M; i++) {
            fprintf(outfile, "%f", i * h);
            for (int s = 0; s <= NUM_SNAP_SAVE; s++) {
                fprintf(outfile, ",%f", snapshots[s][i]);
            }
            fprintf(outfile, "\n");
        }
        
        fclose(outfile);
        printf("Solution saved to transport_solution_multiple_mpi.csv\n");
        
        // Free snapshot memory
        for (int s = 0; s <= NUM_SNAPSHOTS; s++) {
            free(snapshots[s]);
        }
        free(snapshots);
        
        // Calculate and print execution time
        end_time = MPI_Wtime();
        printf("Total execution time: %.4f seconds\n", end_time - start_time);
        double exec_time = end_time - start_time;
        
        // Выводим время выполнения в стандартный формат для удобства сбора данных
        printf("%d,%.4f\n", size, exec_time);
        
        // Также сохраняем в файл
        FILE *time_file = fopen("execution_times.csv", "a");
        if (time_file != NULL) {
            fprintf(time_file, "%d,%.4f\n", size, exec_time);
            fclose(time_file);
        }
        
        // Вывод в формате: K,M,processes,time
        printf("%d,%d,%d,%.4f\n", K, M, size, exec_time);
        
        // Запись в файл
        FILE *f = fopen("benchmark_results.csv", "a");
        if (f) {
            fprintf(f, "%d,%d,%d,%.4f\n", K, M, size, exec_time);
            fclose(f);
        }
    }

    
    // Cleanup
    free(u_prev);
    free(u_curr);
    free(u_next);
    
    if (rank == 0) {
        free(global_solution);
        free(recvcounts);
        free(displs);
    }
    
    MPI_Finalize();
    return 0;
}