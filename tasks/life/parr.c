#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <mpi.h>
#include <pthread.h>
#include <unistd.h>

// Configuration parameters
typedef struct {
    int width;
    int height;
    int steps;
    bool demo_mode;
    int thread_count;
} Config;

// Thread data structure
typedef struct {
    int thread_id;
    int start_row;
    int end_row;
    int width;
    int height;
    bool *current_grid;
    bool *next_grid;
    pthread_barrier_t *barrier;
} ThreadData;

// Function prototypes
void initialize_grid(bool *grid, int width, int height);
int count_neighbors(bool *grid, int x, int y, int width, int height);
void print_grid(bool *grid, int width, int height);
void* thread_compute(void *arg);
void exchange_borders(bool *grid, int width, int local_height, int rank, int size, MPI_Comm comm);
bool is_grid_stable(bool *current, bool *next, int width, int height);

int main(int argc, char *argv[]) {
    int rank, size;
    Config config;
    
    // Initialize MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // Set default configuration
    if (argc > 1 && atoi(argv[1]) == 1) {
        // Demo mode
        config.width = 40;
        config.height = 20;
        config.steps = 100;
        config.demo_mode = true;
    } else {
        // Performance mode
        config.width = 2000;
        config.height = 2000;
        config.steps = 1000;
        config.demo_mode = false;
    }
    
    // Get thread count (default to 4 if not specified)
    config.thread_count = (argc > 2) ? atoi(argv[2]) : 4;
    
    // Calculate local grid dimensions
    int local_height = config.height / size;
    if (rank == size - 1) {
        // Last process may get extra rows
        local_height += config.height % size;
    }
    
    // Allocate memory for grids (including ghost rows for border exchange)
    int local_size = config.width * (local_height + 2);
    bool *current_grid = (bool*)calloc(local_size, sizeof(bool));
    bool *next_grid = (bool*)calloc(local_size, sizeof(bool));
    
    // Adjust pointer to skip the first ghost row
    bool *current = current_grid + config.width;
    bool *next = next_grid + config.width;
    
    // Initialize the grid on rank 0 and distribute
    if (rank == 0) {
        initialize_grid(current, config.width, local_height);
        
        // Send portions to other processes
        for (int i = 1; i < size; i++) {
            int dest_height = config.height / size;
            if (i == size - 1) {
                dest_height += config.height % size;
            }
            
            bool *temp_grid = (bool*)calloc(config.width * dest_height, sizeof(bool));
            initialize_grid(temp_grid, config.width, dest_height);
            MPI_Send(temp_grid, config.width * dest_height, MPI_C_BOOL, i, 0, MPI_COMM_WORLD);
            free(temp_grid);
        }
    } else {
        // Receive portion from rank 0
        MPI_Recv(current, config.width * local_height, MPI_C_BOOL, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    
    // Synchronize before starting
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Create thread data and barrier
    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, config.thread_count);
    
    ThreadData *thread_data = (ThreadData*)malloc(config.thread_count * sizeof(ThreadData));
    pthread_t *threads = (pthread_t*)malloc(config.thread_count * sizeof(pthread_t));
    
    // Calculate rows per thread
    int rows_per_thread = local_height / config.thread_count;
    
    // Start timer
    double start_time = MPI_Wtime();
    
    // Main simulation loop
    for (int step = 0; step < config.steps; step++) {
        // Exchange border rows with neighboring processes
        exchange_borders(current, config.width, local_height, rank, size, MPI_COMM_WORLD);
        
        // Create and launch threads
        for (int i = 0; i < config.thread_count; i++) {
            thread_data[i].thread_id = i;
            thread_data[i].start_row = i * rows_per_thread;
            thread_data[i].end_row = (i == config.thread_count - 1) ? 
                                     local_height : (i + 1) * rows_per_thread;
            thread_data[i].width = config.width;
            thread_data[i].height = local_height;
            thread_data[i].current_grid = current;
            thread_data[i].next_grid = next;
            thread_data[i].barrier = &barrier;
            
            pthread_create(&threads[i], NULL, thread_compute, &thread_data[i]);
        }
        
        // Wait for all threads to complete
        for (int i = 0; i < config.thread_count; i++) {
            pthread_join(threads[i], NULL);
        }
        
        // Check if the grid is stable (optimization for later iterations)
        bool local_stable = is_grid_stable(current, next, config.width, local_height);
        bool global_stable;
        MPI_Allreduce(&local_stable, &global_stable, 1, MPI_C_BOOL, MPI_LAND, MPI_COMM_WORLD);
        
        if (global_stable) {
            if (rank == 0) {
                printf("Grid stabilized at step %d\n", step);
            }
            break;
        }
        
        // Swap grids
        bool *temp = current;
        current = next;
        next = temp;
        
        // Print grid in demo mode
        if (config.demo_mode && rank == 0) {
            printf("Step %d:\n", step);
            print_grid(current, config.width, local_height);
            
            // Collect and print grids from other processes
            for (int i = 1; i < size; i++) {
                int src_height = config.height / size;
                if (i == size - 1) {
                    src_height += config.height % size;
                }
                
                bool *temp_grid = (bool*)calloc(config.width * src_height, sizeof(bool));
                MPI_Recv(temp_grid, config.width * src_height, MPI_C_BOOL, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                print_grid(temp_grid, config.width, src_height);
                free(temp_grid);
            }
            
            // Small delay to see animation
            usleep(100000);
        } else if (config.demo_mode) {
            // Send grid to rank 0 for display
            MPI_Send(current, config.width * local_height, MPI_C_BOOL, 0, 0, MPI_COMM_WORLD);
        }
    }
    
    // End timer
    double end_time = MPI_Wtime();
    double local_elapsed = end_time - start_time;
    double global_elapsed;
    
    // Get the maximum elapsed time across all processes
    MPI_Reduce(&local_elapsed, &global_elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    
    if (rank == 0) {
        printf("Execution time: %.6f seconds\n", global_elapsed);
    }
    
    // Clean up
    pthread_barrier_destroy(&barrier);
    free(threads);
    free(thread_data);
    free(current_grid);
    free(next_grid);
    
    MPI_Finalize();
    return 0;
}

// Function to initialize the grid with random values or specific patterns
void initialize_grid(bool *grid, int width, int height) {
    srand(time(NULL));
    
    // Initialize with random pattern
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            grid[y * width + x] = (rand() % 100 < 25); // 25% chance of life
        }
    }
    
    // Alternatively, add a glider in the middle
    /*
    int mid_x = width / 2;
    int mid_y = height / 2;
    
    grid[mid_y * width + mid_x + 1] = true;
    grid[(mid_y + 1) * width + mid_x + 2] = true;
    grid[(mid_y + 2) * width + mid_x] = true;
    grid[(mid_y + 2) * width + mid_x + 1] = true;
    grid[(mid_y + 2) * width + mid_x + 2] = true;
    */
}

// Count the number of live neighbors around a cell (using toroidal boundary)
int count_neighbors(bool *grid, int x, int y, int width, int height) {
    int count = 0;
    
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            
            // Calculate neighbor coordinates with wrapping (toroidal)
            int nx = (x + dx + width) % width;
            int ny = (y + dy + height) % height;
            
            if (grid[ny * width + nx]) {
                count++;
            }
        }
    }
    
    return count;
}

// Print the grid (for demo mode)
void print_grid(bool *grid, int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            printf("%c", grid[y * width + x] ? '#' : '.');
        }
        printf("\n");
    }
    printf("\n");
}

// Thread function to compute next generation
void* thread_compute(void *arg) {
    ThreadData *data = (ThreadData*)arg;
    int width = data->width;
    bool *current = data->current_grid;
    bool *next = data->next_grid;
    
    // Process assigned rows
    for (int y = data->start_row; y < data->end_row; y++) {
        for (int x = 0; x < width; x++) {
            int neighbors = count_neighbors(current - width, x, y + 1, width, data->height + 2);
            
            if (current[y * width + x]) {
                // Cell is alive
                next[y * width + x] = (neighbors == 2 || neighbors == 3);
            } else {
                // Cell is dead
                next[y * width + x] = (neighbors == 3);
            }
        }
    }
    
    // Wait for all threads to finish processing
    pthread_barrier_wait(data->barrier);
    
    return NULL;
}

// Exchange border rows with neighboring processes
void exchange_borders(bool *grid, int width, int local_height, int rank, int size, MPI_Comm comm) {
    int top = (rank - 1 + size) % size;
    int bottom = (rank + 1) % size;
    
    // Send top row to top process and receive bottom ghost row from top process
    MPI_Sendrecv(grid, width, MPI_C_BOOL, top, 0,
                 grid - width, width, MPI_C_BOOL, top, 0,
                 comm, MPI_STATUS_IGNORE);
    
    // Send bottom row to bottom process and receive top ghost row from bottom process
    MPI_Sendrecv(grid + (local_height - 1) * width, width, MPI_C_BOOL, bottom, 0,
                 grid + local_height * width, width, MPI_C_BOOL, bottom, 0,
                 comm, MPI_STATUS_IGNORE);
}

// Check if the grid is stable (no changes between generations)
bool is_grid_stable(bool *current, bool *next, int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (current[y * width + x] != next[y * width + x]) {
                return false;
            }
        }
    }
    return true;
}