//
// Created by predicador on 15/01/16.
//

#include "mus.h"
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N_CARTAS_MAZO 40
#define N_CARTAS_MANO 4
#define N_PALOS 4

int main(int argc, char **argv) {
    int rank, size, namelen, version, subversion, psize, corte, repartidor;
    MPI_Comm parent;
    Carta mazo[N_CARTAS_MAZO];
    char *palos[] = { "Oros", "Copas", "Espadas", "Bastos"};
    char * paloCorte = (char *)malloc(7*sizeof(char));
    char processor_name[MPI_MAX_PROCESSOR_NAME];

    MPI_Status stat;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Get_processor_name(processor_name, &namelen);
    MPI_Get_version(&version, &subversion);
    printf("Jugador con identificador %d  de un total de  %d  en  %s  ejecutando  MPI  %d.%d\n", rank, size,
           processor_name, version, subversion);

    MPI_Comm_get_parent(&parent);

    if (parent == MPI_COMM_NULL) {
        printf("Error: no se ha encontrado proceso padre.\n");
        exit(1);
    }

    MPI_Comm_remote_size(parent, &psize);

    if (psize != 1) {

        printf("Error:  el número de padres debería ser 1 y no %d.\n", psize);
        exit(2);
    }

    printf("Jugador %d: Listo para jugar!\n", rank);
    MPI_Bcast(&corte, 1, MPI_INT, 0, parent);
    //MPI_Barrier(parent);

    if (rank == corte) {
        /* Este proceso debe realizar el corte */
        /* Para ello debe recibir el mazo */
        printf("Proceso corte recibiendo mazo; el proceso corte es: %d\n", corte);
        /*
        int i=0;
        for (i=0; i<N_CARTAS_MAZO;i++) {
            mazo[i].palo = (char *)malloc(5*sizeof(char));
            mazo[i].cara = (char *)malloc(8*sizeof(char));
            MPI_Recv(&mazo[i].id, 1, MPI_INT, 0, 0, parent, &stat);
            MPI_Recv(&mazo[i].valor, 1, MPI_INT, 0, 0, parent, &stat);
            MPI_Recv(mazo[i].palo, 7, MPI_CHAR, 0, 0, parent, &stat);
            MPI_Recv(mazo[i].cara, 8, MPI_CHAR, 0, 0, parent, &stat);
        }
*/
        recibirMazo(mazo, 0, parent, stat);
        //printMazo(mazo);
        cortarMazo(mazo, &paloCorte);
        printf("El palo de corte es: %s\n", paloCorte);
        int j=0;
        for(j=0; j < N_PALOS; j++) {
            if (strcmp(paloCorte, palos[j]) == 0 ) {

                repartidor=j;
                /* Envío del id del repartidor al proceso maestro */

                MPI_Send(&repartidor, 1, MPI_INT, 0, 0, parent);
                //MPI_Bcast(&repartidor, 1, MPI_INT, 0, parent);
            }
        }


    }
    MPI_Bcast(&repartidor, 1, MPI_INT, 0, parent);
    if (rank == repartidor) {

        printf("Proceso repartidor recibiendo mazo\n");
        /*
        int i=0;
        for (i=0; i<N_CARTAS_MAZO;i++) {
            mazo[i].palo = (char *)malloc(5*sizeof(char));
            mazo[i].cara = (char *)malloc(8*sizeof(char));
            MPI_Recv(&mazo[i].id, 1, MPI_INT, 0, 0, parent, &stat);
            MPI_Recv(&mazo[i].valor, 1, MPI_INT, 0, 0, parent, &stat);
            MPI_Recv(mazo[i].palo, 7, MPI_CHAR, 0, 0, parent, &stat);
            MPI_Recv(mazo[i].cara, 8, MPI_CHAR, 0, 0, parent, &stat);
        }*/
        recibirMazo(mazo, 0, parent, stat);
        printMazo(mazo);

        //int recibido;
        //char *palabras = (char *) malloc(2*sizeof(char *));

        //printf("VALOR 1: %s\n" , mazo[0].palo);
       // printMazo(mazo);
          //  MPI_Recv(palabras, 10, MPI_CHAR, 0, 0, parent, &stat);

        //MPI_Bcast(&recibido, 1, MPI_INT, 0, parent);
       // printf("RECIBIDO: %d\n" , recibido);

        //printf("PALABRAS: %s\n" , palabras);

/*
        MPI_Datatype cartaType, tipos[2] = {MPI_CHAR, MPI_INT};
        MPI_Aint offsets[2], extent;
        int blocklen[2] = {1, 8};
        offsets[0] = offsetof(Carta, palo);
        offsets[1] = offsetof(Carta, id);
        MPI_Type_extent(MPI_CHAR, &extent);
        MPI_Type_struct(2, blocklen, offsets, tipos, &cartaType);
        MPI_Type_commit(&cartaType);
        MPI_Recv(mazo, N_CARTAS_MANO, cartaType, 0, 0, MPI_COMM_WORLD, &stat);
        printMazo(mazo);*/
    }

    MPI_Comm_disconnect(&parent);
    MPI_Finalize();
    return 0;
}