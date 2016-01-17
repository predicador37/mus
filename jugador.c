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
#define N_JUGADORES 4

int main(int argc, char **argv) {
    int rank, size, namelen, version, subversion, psize, corte, postre, sizeMazo, sizeDescartadas, ok;
    int buffer[3];
    MPI_Comm parent;
    Carta mazo[N_CARTAS_MAZO];
    Carta mano[N_CARTAS_MANO];
    char *palos[] = { "Oros", "Copas", "Espadas", "Bastos"};
    char * paloCorte = (char *)malloc(7*sizeof(char));
    char processor_name[MPI_MAX_PROCESSOR_NAME];

    MPI_Status stat;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Get_processor_name(processor_name, &namelen);
    MPI_Get_version(&version, &subversion);
    printf("[jugador %d] Jugador con identificador %d  de un total de  %d  en  %s  ejecutando  MPI  %d.%d\n", rank, rank, size,
           processor_name, version, subversion);

    MPI_Comm_get_parent(&parent);

    if (parent == MPI_COMM_NULL) {
        printf("[jugador %d] Error: no se ha encontrado proceso padre.\n", rank);
        exit(1);
    }

    MPI_Comm_remote_size(parent, &psize);

    if (psize != 1) {

        printf("[jugador %d] Error:  el número de padres debería ser 1 y no %d.\n", rank, psize);
        exit(2);
    }

    printf("[jugador %d] Jugador %d: Listo para jugar!\n", rank, rank);
    MPI_Bcast(&sizeMazo, 1, MPI_INT, 0, parent);
    MPI_Bcast(&sizeDescartadas, 1, MPI_INT, 0, parent);
    MPI_Bcast(&corte, 1, MPI_INT, 0, parent);


    if (rank == corte) {
        /* Este proceso debe realizar el corte */
        /* Para ello debe recibir el mazo */
        printf("[jugador %d] Proceso corte recibiendo mazo\n", rank);
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
        printf("[jugador %d] El palo de corte es: %s\n", rank, paloCorte);
        int j=0;
        for(j=0; j < N_PALOS; j++) {
            if (strcmp(paloCorte, palos[j]) == 0 ) {

                postre=j;
                /* Envío del id del postre al proceso maestro */

                MPI_Send(&postre, 1, MPI_INT, 0, 0, parent);
                //MPI_Bcast(&postre, 1, MPI_INT, 0, parent);
            }
        }


    }
    MPI_Bcast(&postre, 1, MPI_INT, 0, parent);
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == postre && sizeMazo==40) {

        printf("[postre %d] Proceso postre recibiendo mazo\n", rank);
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
        //printMazo(mazo);

        int i=0;
        int j=0;
        int k=0;

        int siguienteJugador = postre;
        for (i=0; i<N_CARTAS_MANO;i++) {
            for (j=0; j<N_JUGADORES; j++) {

                siguienteJugador = add_mod(siguienteJugador, 1, 4);

                buffer[0]=i;
                buffer[1]=siguienteJugador;

                if (siguienteJugador != postre) {

                    enviarCarta(mazo[k], siguienteJugador, MPI_COMM_WORLD);
                    MPI_Send(&buffer, 2, MPI_INT, 0, 0, parent);
                    //MPI_Recv(&ok, 1, MPI_INT, siguienteJugador, 0, MPI_COMM_WORLD, &stat);

                }
                else {
                    //printf("[postre %d] Se reparte carta %d al postre %d\n", rank, i, siguienteJugador);
                    MPI_Send(&buffer, 2, MPI_INT, 0, 0, parent);
                    mano[i] = mazo [k];
                    buffer[0]=rank;
                    buffer[1]=i;
                    buffer[2]=mano[i].valor;
                    MPI_Send(&buffer, 3, MPI_INT, 0, 0, parent);
                    //printf("[postre %d] Jugador %d recibe carta %d con valor %d\n", rank, rank, k, mano[i].valor );
                }
                sizeMazo--;
                sizeDescartadas++;
                k++;
            }
        }
        //MPI_Bcast(&sizeMazo, 1, MPI_INT, postre, parent);
        MPI_Send(&sizeMazo, 1, MPI_INT, 0, 0, parent);
        MPI_Bcast(&sizeMazo, 1, MPI_INT, postre, MPI_COMM_WORLD);
        //printf("[jugador %d] tamaño del mazo: %d\n", rank, sizeMazo);

    }
    else {
        int i = 0;
        for (i = 0; i < N_CARTAS_MANO; i++) {

            mano[i] = recibirCarta(postre, MPI_COMM_WORLD, stat);
            buffer[0]=rank;
            buffer[1]=i;
            buffer[2]=mano[i].valor;
            MPI_Send(&buffer, 3, MPI_INT, 0, 0, parent);
            //printf("[jugador %d] Jugador %d recibe carta %d con valor %d\n", rank, rank, i, mano[i].valor );
            //ok =1;
            //MPI_Send(&ok, 1, MPI_INT, postre, 0, MPI_COMM_WORLD);
            //ok=0;
        }
        MPI_Bcast(&sizeMazo, 1, MPI_INT, postre, MPI_COMM_WORLD);
        //printf("[jugador %d] tamaño del mazo: %d\n", rank, sizeMazo);


    }
    MPI_Barrier(MPI_COMM_WORLD);
    //printf("[jugador %d], MANO PARA EL JUGADOR %d\n", rank, rank);
    //printMazo(mano, N_CARTAS_MANO);
    MPI_Comm_disconnect(&parent);
    MPI_Finalize();
    return 0;
}