#include <iostream>
#include <string.h>
#include "mpi.h"

using namespace std;

#define ROW_TAG 1111
#define COL_TAG 2222
#define SINGLE_TAG 3333

class Worker
{
public:
  Worker(int r, int s, int tile, char *&w_str_A, char *&w_str_B, int s_str_A, int s_str_B, int pr_y);
  MPI_Datatype SUB_VECTOR_ROW;
  MPI_Datatype SUB_VECTOR_COL;

  void work();

protected:
  void send();
  void receive();
  int calculate_distance();

  MPI_Status status;

  int rank, size, tile, tile_A, tile_B, process_y;
  char *worker_string_A, *worker_string_B;
  int *worker_matrix;

  struct position_flag
  {
    bool row = true;
    bool col = true;
    bool single = true;
  } send_flag, receive_flag;
};