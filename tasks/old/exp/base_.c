#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <gmp.h>

void factorial(mpz_t result, int n) {
    mpz_set_ui(result, 1);
    for (int i = 2; i <= n; i++) {
        mpz_mul_ui(result, result, i);
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <precision>\n", argv[0]);
        return 1;
    }

    double start_time = MPI_Wtime();
    
    int precision = atoi(argv[1]); // Число знаков после запятой
    int rank, size;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    mpf_set_default_prec(precision * 3.5);
    
    mpf_t local_sum, global_sum, term;
    mpf_init(local_sum);
    mpf_init(global_sum);
    mpf_init(term);
    
    mpz_t fact;
    mpz_init(fact);


    for (int i = rank; i < precision * 2; i += size) {
        factorial(fact, i);
        mpf_set_z(term, fact);
        mpf_ui_div(term, 1, term);
        mpf_add(local_sum, local_sum, term);
    }
    
    mpf_t recv_buffer;
    mpf_init(recv_buffer);
    
    MPI_Reduce(local_sum->_mp_d, recv_buffer->_mp_d, local_sum->_mp_size, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    
    if (rank == 0) {
        mpf_set(global_sum, local_sum);
        mpf_t recv_sum;
        mpf_init(recv_sum);

        for (int i = 1; i < size; i++) {
            MPI_Recv(recv_sum->_mp_d, local_sum->_mp_size, MPI_LONG, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            mpf_add(global_sum, global_sum, recv_sum);
        }

        gmp_printf("e ≈ %.*Ff\n-----------------------------\n", precision, global_sum);
        
        mpf_clear(recv_sum);
    } else {
        MPI_Send(local_sum->_mp_d, local_sum->_mp_size, MPI_LONG, 0, 0, MPI_COMM_WORLD);
    }

    mpf_clear(recv_buffer);
    mpz_clear(fact);
    mpf_clear(local_sum);
    mpf_clear(global_sum);
    mpf_clear(term);

    double end_time = MPI_Wtime();
    printf("Время выполнения: %f секунд\n", end_time - start_time);

    
    MPI_Finalize();
    return 0;
}
