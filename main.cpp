#include <math.h>
#include "mpi.h"
#include "Worker.h"

void generate_string(char *A, int size)
{
    for (int i = 0; i < size - 1; i++)
    {
        if (i % 4 == 0)
            A[i] = 'a';
        if (i % 4 == 1)
            A[i] = 'b';
        if (i % 4 == 2)
            A[i] = 'c';
        if (i % 4 == 3)
            A[i] = 'd';
    }
    A[size - 1] = '\0';
}

int main(int argc, char **argv)
{

    int size, rank;
    int size_string_A = 10000 + 1;
    int size_string_B = 9790 + 1;
    int TILE_STRING = 5000 + 1; // char[] + '/0'

    int process_x = ceil(float(size_string_A - 1) / float(TILE_STRING - 1));
    int process_y = ceil(float(size_string_B - 1) / float(TILE_STRING - 1));

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Status stat;

    int row_rank, row_size; //divide rank by rows to send it string_a
    int row_comm_rule = rank % process_y;
    MPI_Comm row_comm;
    MPI_Comm_split(MPI_COMM_WORLD, row_comm_rule, rank, &row_comm);
    MPI_Comm_rank(row_comm, &row_rank);
    MPI_Comm_size(row_comm, &row_size);

    int col_rank, col_size; //divide rank by cols to send it string_B
    int col_comm_rule = rank / process_y;
    MPI_Comm col_comm;
    MPI_Comm_split(MPI_COMM_WORLD, col_comm_rule, rank, &col_comm);
    MPI_Comm_rank(col_comm, &col_rank);
    MPI_Comm_size(col_comm, &col_size);

    char *string_A = new char[size_string_A];
    char *string_B = new char[size_string_B];
    char *worker_string_A = new char[TILE_STRING];
    char *worker_string_B = new char[TILE_STRING];

    if (row_rank == 0)
        generate_string(string_A, size_string_A);

    if (col_rank == 0)
        generate_string(string_B, size_string_B);

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Scatter(string_A, TILE_STRING - 1, MPI_CHAR, worker_string_A, TILE_STRING - 1, MPI_CHAR, 0, row_comm);
    MPI_Scatter(string_B, TILE_STRING - 1, MPI_CHAR, worker_string_B, TILE_STRING - 1, MPI_CHAR, 0, col_comm);
    MPI_Barrier(MPI_COMM_WORLD);

    delete string_A;
    delete string_B;

    Worker *w = new Worker(rank, size, TILE_STRING, worker_string_A, worker_string_B, size_string_A - 1, size_string_B - 1, process_y);
    w->work();

    MPI_Finalize();
}