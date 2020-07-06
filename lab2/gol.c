/*
ParaCom Lab2, Game of Life
Use MPI with Cartestian Topology
Lintin@PKU
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <mpi.h>

#define L1 2000
#define L2 2000
#define LT 100
#define row_size 4
#define col_size 2
#define xn 500
#define yn 1000
#define SEED 0
bool world[xn+2][yn+4];
bool result[xn+2][yn+4];

double get_walltime()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return (double)(tp.tv_sec + tp.tv_usec * 1e-6);
}

int main(int argc, char*argv[]){
    int population;

    
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // define a new cart topology.
    MPI_Comm old_comm, new_comm;
    old_comm = MPI_COMM_WORLD;
    int dim_size[2], ndims = 2, periods[2], reorder;
    dim_size[0] = row_size, dim_size[1] = col_size;
    periods[0] = 1, periods[1] = 1;
    reorder = 1;

    MPI_Cart_create(old_comm, ndims, dim_size, periods, reorder, &new_comm);
    MPI_Comm_rank(new_comm, &rank);
    int coords[2];
    MPI_Cart_coords(new_comm, rank, ndims, coords);


    // get ranks of all its neighbors
    int up;
    int down;
    int left;
    int right;


    int up_coords[2];
    int down_coords[2];
    int left_coords[2];
    int right_coords[2];

    up_coords[0] = coords[0] - 1;
    down_coords[0] = coords[0] + 1;
    left_coords[0] = coords[0];
    right_coords[0] = coords[0];

    up_coords[1] = coords[1];
    down_coords[1] = coords[1];
    left_coords[1] = coords[1] - 1;
    right_coords[1] = coords[1] + 1;

    int up_left;
    int up_right;
    int down_left;
    int down_right;

    int up_left_coords[2];
    int up_right_coords[2];
    int down_left_coords[2];
    int down_right_coords[2];

    up_left_coords[0] = coords[0] - 1;
    up_right_coords[0] = coords[0] - 1;
    down_left_coords[0] = coords[0] + 1;
    down_right_coords[0] = coords[0] + 1;

    up_left_coords[1] = coords[1] - 1;
    up_right_coords[1] = coords[1] + 1;
    down_left_coords[1] = coords[1] - 1;
    down_right_coords[1] = coords[1] + 1;

    // Get the rank of each direction
    MPI_Cart_rank(new_comm, up_coords, &up);
    MPI_Cart_rank(new_comm, down_coords, &down);
    MPI_Cart_rank(new_comm, left_coords, &left);
    MPI_Cart_rank(new_comm, right_coords, &right);

    MPI_Cart_rank(new_comm, up_left_coords, &up_left);
    MPI_Cart_rank(new_comm, up_right_coords, &up_right);
    MPI_Cart_rank(new_comm, down_left_coords, &down_left);
    MPI_Cart_rank(new_comm, down_right_coords, &down_right);

    // define a MPI_Datatype for exchanging the column data.
    MPI_Datatype vertical_type;
    MPI_Type_vector(xn, 1, yn+4, MPI_CHAR, &vertical_type);
    MPI_Type_commit(&vertical_type);
    
    //INITILAZATION
    for(int i = 0; i < L1*L2; i++){
        int tmp = rand()%2;
        int x = i%L1;
        int y = i/L1;
        if(x/xn==coords[0]&&y/yn==coords[1]){
            world[x%xn+1][y%yn+1] = tmp;
        }
    }

    MPI_Barrier(new_comm);
    
    double time = MPI_Wtime();
    for(int t = 0; t<LT; t+=2){
        // ping! world -> result

        // exchange up and down row
        MPI_Sendrecv(&world[1][1], yn, MPI_CHAR, up, 0, &world[xn+1][1], yn, MPI_CHAR, down, 0, new_comm, MPI_STATUS_IGNORE);
        MPI_Sendrecv(&world[xn][1], yn, MPI_CHAR, down, 0, &world[0][1], yn, MPI_CHAR, up, 0, new_comm, MPI_STATUS_IGNORE);
        // exchange left and right column
       
        MPI_Sendrecv(&world[1][1], 1, vertical_type, left, 0, &world[1][yn+1], 1, vertical_type, right, 0, new_comm, MPI_STATUS_IGNORE);
        MPI_Sendrecv(&world[1][yn], 1, vertical_type, right, 0, &world[1][0], 1, vertical_type, left, 0, new_comm, MPI_STATUS_IGNORE);
        
        // exchange four corner

        MPI_Sendrecv(&world[1][1], 1, MPI_CHAR, up_left, 0, &world[xn+1][yn+1], 1, MPI_CHAR, down_right, 0, new_comm, MPI_STATUS_IGNORE);
        MPI_Sendrecv(&world[1][yn], 1, MPI_CHAR, up_right, 0, &world[xn+1][0], 1, MPI_CHAR, down_left, 0, new_comm, MPI_STATUS_IGNORE);
        MPI_Sendrecv(&world[xn][1], 1, MPI_CHAR, down_left, 0, &world[0][yn+1], 1, MPI_CHAR, up_right, 0, new_comm, MPI_STATUS_IGNORE);
        MPI_Sendrecv(&world[xn][yn], 1, MPI_CHAR, down_right, 0, &world[0][0], 1, MPI_CHAR, up_left, 0, new_comm, MPI_STATUS_IGNORE);

        // evolving
        for(int x = 1; x<=xn; x++){
            for(int y = 1; y<=yn;y++){
                int nn = world[x-1][y-1] + world[x-1][y] + world[x-1][y+1] + world[x][y-1] + world[x][y+1] + world[x+1][y-1] + world[x+1][y] + world[x+1][y+1];
                result[x][y] = !(((nn & 7) | world[x][y]) ^ 3);
            }
        }

        // pong! result -> world
        // exchange up and down row
        MPI_Sendrecv(&result[1][1], yn, MPI_CHAR, up, 0, &result[xn+1][1], yn, MPI_CHAR, down, 0, new_comm, MPI_STATUS_IGNORE);
        MPI_Sendrecv(&result[xn][1], yn, MPI_CHAR, down, 0, &result[0][1], yn, MPI_CHAR, up, 0, new_comm, MPI_STATUS_IGNORE);
       
        // exchange left and right column
       
        MPI_Sendrecv(&result[1][1], 1, vertical_type, left, 0, &result[1][yn+1], 1, vertical_type, right, 0, new_comm, MPI_STATUS_IGNORE);
        MPI_Sendrecv(&result[1][yn], 1, vertical_type, right, 0, &result[1][0], 1, vertical_type, left, 0, new_comm, MPI_STATUS_IGNORE);
        
        // exchange four corner

        MPI_Sendrecv(&result[1][1], 1, MPI_CHAR, up_left, 0, &result[xn+1][yn+1], 1, MPI_CHAR, down_right, 0, new_comm, MPI_STATUS_IGNORE);
        MPI_Sendrecv(&result[1][yn], 1, MPI_CHAR, up_right, 0, &result[xn+1][0], 1, MPI_CHAR, down_left, 0, new_comm, MPI_STATUS_IGNORE);
        MPI_Sendrecv(&result[xn][1], 1, MPI_CHAR, down_left, 0, &result[0][yn+1], 1, MPI_CHAR, up_right, 0, new_comm, MPI_STATUS_IGNORE);
        MPI_Sendrecv(&result[xn][yn], 1, MPI_CHAR, down_right, 0, &result[0][0], 1, MPI_CHAR, up_left, 0, new_comm, MPI_STATUS_IGNORE);
        //evolving
        for(int x = 1; x<=xn; x++){
            for(int y = 1; y<=yn;y++){
                int nn = result[x-1][y-1] + result[x-1][y] + result[x-1][y+1] + result[x][y-1] + result[x][y+1] + result[x+1][y-1] + result[x+1][y] + result[x+1][y+1];
                world[x][y] = !(((nn & 7) | result[x][y]) ^ 3);
            }
        }

    }



    MPI_Barrier(new_comm);
    time = MPI_Wtime() - time;
    //COUNTING
    population = 0;
    for(int x = 1; x<=xn; x++){
        for(int y = 1; y<=yn; y++)
        population += world[x][y];
    }
    int total_pop;
    MPI_Reduce(&population, &total_pop, 1, MPI_INT, MPI_SUM, 0, new_comm);

    if(rank==0){ // report the result in root.
                       printf("World size: %d x %d, total generations: %d\n", L1, L2, LT);
        printf("Population is changed to %d\n", total_pop);
        
        printf("Wall time: %lf\n", time);
    }
    MPI_Finalize();
}
