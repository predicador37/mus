//
// Created by predicador on 19/02/16.
//

/*
  MPI2
  Worker
  Code
  */

#include<stdio.h>
#include<stdlib.h>
#include<mpi.h>

int main(int argc, char **argv) {


    int rank, size, namelen, version, subversion, psize, parent_rank;

    MPI_Comm parent;


    char processor_name[MPI_MAX_PROCESSOR_NAME];


    MPI_Init(&argc, &argv);


    MPI_Comm_rank(MPI_COMM_WORLD, &rank);


    MPI_Comm_size(MPI_COMM_WORLD, &size);


    MPI_Get_processor_name(processor_name, &namelen);


    MPI_Get_version(&version, &subversion);


    printf("I’m	worker %d of %d on %s running MPI %d.%d\n", rank, size, processor_name, version, subversion);


    MPI_Comm_get_parent(&parent);


    if (parent == MPI_COMM_NULL) {
        printf("Error: no parent process found!\n");
        exit(1);
    }


    MPI_Comm_remote_size(parent, &psize);


    if (psize != 1) {

        printf("Error: number of parents (%d) should be 1.\n", psize);
        exit(2);
    }
/*
  communicate
  with
  parent
  process
  */


    int sendrank = rank;


    printf("Worker %d: Success!\n", rank);


    MPI_Send(&rank, 1, MPI_INT, 0, 0, parent);
    MPI_Bcast(&parent_rank, 1, MPI_INT, 0, parent);

    printf("Rank received from parent is %d\n", parent_rank);

    MPI_Comm_disconnect(&parent);

    MPI_Finalize();

    return 0;
}