#include "Worker.h"

Worker::Worker(int r, int s, int t, char *&w_str_A, char *&w_str_B, int s_str_A, int s_str_B, int pr_y) : rank(r), size(s), worker_string_A(w_str_A), worker_string_B(w_str_B), process_y(pr_y)
{
    tile = t;
    tile_A = tile;
    tile_B = tile;
    worker_string_A[tile_A - 1] = '\0';
    worker_string_B[tile_B - 1] = '\0';

    //set send flag
    if (rank % process_y == process_y - 1)
    {
        send_flag.col = false;
        tile_B = (s_str_B % (tile - 1) == 0 ? tile : (s_str_B % (tile - 1)) + 1);
    }
    if (rank >= size - process_y)
    {
        send_flag.row = false;
        tile_A = (s_str_A % (tile - 1) == 0 ? tile : (s_str_A % (tile - 1)) + 1);
    }

    //set receive flag

    if (rank == 0)
    {
        receive_flag.col = false;
        receive_flag.row = false;
        worker_matrix = new int[tile_A * tile_B]; // here +1 is for col and row received
        for (int i = 0; i < tile_B; i++)
            worker_matrix[i] = i + rank * tile;
        for (int j = 1; j < tile_A; j++)
            worker_matrix[j * (tile)] = j + rank * tile;
    }
    
    MPI_Type_vector(1, tile_B, 1, MPI_INT, &SUB_VECTOR_ROW);
    MPI_Type_commit(&SUB_VECTOR_ROW);

    MPI_Type_vector(tile_A, 1, tile_B, MPI_INT, &SUB_VECTOR_COL);
    MPI_Type_commit(&SUB_VECTOR_COL);

}

void Worker::instantiate_matrix(){

    worker_matrix = new int[tile_A * tile_B]; // here +1 is for col and row received

    if (rank % process_y == 0 )
    {
        receive_flag.col = false;
        for (int j = 0; j < tile_A; j++)
            worker_matrix[j * (tile)] = j - 1 + rank * tile;
    }
    if (rank < process_y )
    {
        receive_flag.row = false;
        for (int i = 0; i < tile_B; i++)
            worker_matrix[i] = i - 1 + rank * tile;
    }
}

void Worker::receive()
{   
    MPI_Probe( MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    instantiate_matrix();

    if (receive_flag.row){
        MPI_Irecv(&worker_matrix[0], 1, SUB_VECTOR_ROW, rank - process_y, ROW_TAG, MPI_COMM_WORLD, &row_request);
        MPI_Wait(&row_request, &status);
    }
    if (receive_flag.col){
        MPI_Irecv(&worker_matrix[0], 1, SUB_VECTOR_COL, rank - 1, COL_TAG, MPI_COMM_WORLD, &col_request);
        MPI_Wait(&col_request, &status);
    }
}

void Worker::send()
{
    if (send_flag.row){
        MPI_Isend(&worker_matrix[tile_B * (tile_A-1)], 1, SUB_VECTOR_ROW, rank + process_y, ROW_TAG, MPI_COMM_WORLD, &row_request);
        MPI_Request_free(&row_request);
    }

    if (send_flag.col){
        MPI_Isend(&worker_matrix[(tile_B - 1)], 1, SUB_VECTOR_COL, rank + 1, COL_TAG, MPI_COMM_WORLD, &col_request);
        MPI_Request_free(&col_request);
    }

    delete[] worker_string_A;
    delete[] worker_string_B;
    delete[] worker_matrix;
}

void Worker::work()
{
    if( rank != 0)
        this->receive();
    
    this->calculate_distance();
    this->send();
    
}

int minimum_(const int a, const int b, const int c)
{
    return std::min(std::min(a, b), c);
}

int Worker::calculate_distance()
{
    int distance;

    for (int i = 1; i < tile_B; i++)
    {
        for (int j = 1; j < tile_A; j++)
        {
            if (worker_string_B[i - 1] != worker_string_A[j - 1])
            {
                int k = minimum_(worker_matrix[i + tile_B * (j - 1)],        //insertion
                                 worker_matrix[(i - 1) + tile_B * j],        //insertion
                                 worker_matrix[(i - 1) + tile_B * (j - 1)]); //substitution
                worker_matrix[i + tile_B * j] = k + 1;
            }
            else
            {
                worker_matrix[i + tile_B * j] = worker_matrix[(i - 1) + tile_B * (j - 1)];
            }
        }
    }

    distance = worker_matrix[tile_A * tile_B - 1];
    if (rank == size - 1)
        cout << "RANK " << rank << " DISTANCE IS " << distance << endl;

    return distance;
}