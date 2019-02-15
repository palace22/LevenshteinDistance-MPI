#include "Worker.h"

Worker::Worker(int r, int s, int t, char *&w_str_A, char *&w_str_B, int s_str_A, int s_str_B, int pr_y) : rank(r), size(s), worker_string_A(w_str_A), worker_string_B(w_str_B), process_y(pr_y)
{
    tile = t;
    tile_A = tile;
    tile_B = tile;
    worker_string_A[tile_A - 1] = '\0';
    worker_string_B[tile_B - 1] = '\0';

    if (rank % process_y != process_y - 1 && rank < size - process_y)
    {
        worker_matrix = new int[tile_A * tile_B]; // here +1 is for col and row received
        for (int i = 0; i < tile_A * tile_B; i++)
            worker_matrix[i] = 0;
    }

    //set send flag
    if (rank % process_y == process_y - 1)
    {
        /*
        oooox
        oooox
        oooox
        */
        send_flag.col = false;
        send_flag.single = false;
        tile_B = (s_str_B % (tile - 1) == 0 ? tile : (s_str_B % (tile - 1)) + 1);
        worker_matrix = new int[tile_A * tile_B]; // here +1 is for col and row received
        for (int i = 0; i < tile_A * tile_B; i++)
            worker_matrix[i] = 0;
    }
    if (rank >= size - process_y)
    {
        /*
        ooooo
        ooooo
        xxxxx
        */
        send_flag.row = false;
        send_flag.single = false;
        tile_A = (s_str_A % (tile - 1) == 0 ? tile : (s_str_A % (tile - 1)) + 1);
        worker_matrix = new int[tile_A * tile_B]; // here +1 is for col and row received
        for (int i = 0; i < tile_A * tile_B; i++)
            worker_matrix[i] = 0;
    }

    //set receive flag
    if (rank % process_y == 0)
    {
        /*
        xoooo
        xoooo
        xoooo
        */
        receive_flag.col = false;
        receive_flag.single = false;

        for (int j = 0; j < tile_A; j++)
            worker_matrix[j * (tile)] = j - 1 + rank * tile;
    }
    if (rank < process_y)
    {
        /*
        xxxxx
        ooooo
        ooooo
        */
        receive_flag.row = false;
        receive_flag.single = false;
        for (int i = 0; i < tile_B; i++)
            worker_matrix[i] = i - 1 + rank * tile;
        //cout << "RANK: " << rank << " OF SIZE " << size<< " Setted 4 "  << " WITH PR Y " << process_y<< endl;
    }
    // tile_A = tile_A - 1; //cause final symbol '\0' is setted
    // tile_B = tile_B - 1; //cause final symbol '\0' is setted

    if (rank == 0)
    {
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

void Worker::receive()
{

    if (receive_flag.row)
    {
        MPI_Recv(&worker_matrix[0], 1, SUB_VECTOR_ROW, rank - process_y, ROW_TAG, MPI_COMM_WORLD, &status);
        //cout << "RANK: " << rank << " RECEIVED ROW BY: " << status.MPI_SOURCE << endl;
    }
    if (receive_flag.col)
    {
        MPI_Recv(&worker_matrix[0], 1, SUB_VECTOR_COL, rank - 1, COL_TAG, MPI_COMM_WORLD, &status);
        //cout << "RANK: " << rank << " RECEIVED COL BY: " << status.MPI_SOURCE << endl;
    }
    if (receive_flag.single)
    {
        MPI_Recv(&worker_matrix[0], 1, MPI_INT, rank - process_y - 1, SINGLE_TAG, MPI_COMM_WORLD, &status);
        //cout << "RANK: " << rank << " RECEIVED COL BY: " << status.MPI_SOURCE << endl;
    }
}

void Worker::send()
{
    if (send_flag.row)
    {
        MPI_Send(&worker_matrix[(tile_B - 1) * tile_A], 1, SUB_VECTOR_ROW, rank + process_y, ROW_TAG, MPI_COMM_WORLD);
        //cout << "RANK: " << rank << " SEND ROW TO: " <<rank + process_y << endl;
    }
    if (send_flag.col)
    {
        MPI_Send(&worker_matrix[(tile_B - 1)], 1, SUB_VECTOR_COL, rank + 1, COL_TAG, MPI_COMM_WORLD);
        //cout << "RANK: " << rank << " SEND COL TO: " <<rank + 1 << endl;
    }
    if (send_flag.single)
    {
        MPI_Send(&worker_matrix[tile_A * tile_B - 1], 1, MPI_INT, rank + process_y + 1, SINGLE_TAG, MPI_COMM_WORLD);
        //cout << "RANK: " << rank << " SEND SINGLE TO: " <<rank + process_y +1 << endl;
    }

    delete[] worker_string_A;
    delete[] worker_string_B;
    delete[] worker_matrix;
}

void Worker::work()
{
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
    //std::cout << "RANK: " << rank << " STR L A: " << (worker_string_A) << " STR L B: " << (worker_string_B) << std::endl;
    //std::cout << "RANK: " << rank << " STR L A: " << tile_B << " STR L B: " << tile_B << std::endl;
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