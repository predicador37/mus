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
#define N_CARTAS_PALO 10
#define N_PALOS 4
#define N_JUGADORES 4

int main(int argc, char **argv) {
    int rank, size, namelen, version, subversion, psize, corte, postre, sizeMazo, sizeDescartadas, ok;
    int buffer[3], rbuf[50];
    MPI_Comm parent;
    Carta mazo[N_CARTAS_MAZO];
    Carta mano[N_CARTAS_MANO];
    char *palos[] = {"Oros", "Copas", "Espadas", "Bastos"};
    char *caras[] = {"As", "Dos", "Tres", "Cuatro", "Cinco",
                     "Seis", "Siete", "Sota", "Caballo", "Rey"};
    int cuentaCartas[N_CARTAS_PALO];
    char *paloCorte = (char *) malloc(7 * sizeof(char));
    char processor_name[MPI_MAX_PROCESSOR_NAME];

    MPI_Status stat;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Get_processor_name(processor_name, &namelen);
    MPI_Get_version(&version, &subversion);
    printf("[jugador %d] Jugador con identificador %d  de un total de  %d  en  %s  ejecutando  MPI  %d.%d\n", rank,
           rank, size,
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
        recibirMazo(mazo, 0, parent, stat);
        cortarMazo(mazo, &paloCorte);
        printf("[jugador %d] El palo de corte es: %s\n", rank, paloCorte);
        int j = 0;
        for (j = 0; j < N_PALOS; j++) {
            if (strcmp(paloCorte, palos[j]) == 0) {
                    /* corresponde repartir primero al primer jugador de su derecha si sale oros;
                     * al segundo si sale copas; al tercero si sale espadas y al mismo que cortó si sale bastos*/

                    postre = add_mod(corte, j+1, 4);

                /* Envío del id del postre al proceso maestro */
                MPI_Send(&postre, 1, MPI_INT, 0, 0, parent);
            }
        }
    }
    MPI_Bcast(&postre, 1, MPI_INT, 0, parent);
    int manoId = add_mod(postre, 1, 4);
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == postre && sizeMazo == 40) {

        printf("[postre %d] Proceso postre recibiendo mazo\n", rank);
        recibirMazo(mazo, 0, parent, stat);

        /* A continuación tiene lugar el reparto de cartas secuencial*/
        /* Por este requisito no se ha utilizado scatter*/
        /* La e/s se redirige al proceso maestro */

        int i = 0;
        int j = 0;
        int k = 0;

        int siguienteJugador = postre;
        for (i = 0; i < N_CARTAS_MANO; i++) {
            for (j = 0; j < N_JUGADORES; j++) { /* En total se reparten 16 cartas */

                siguienteJugador = add_mod(siguienteJugador, 1, 4);
                buffer[0] = i;
                buffer[1] = siguienteJugador;

                if (siguienteJugador != postre) {

                    enviarCarta(mazo[k], siguienteJugador, MPI_COMM_WORLD);
                    MPI_Send(&buffer, 2, MPI_INT, 0, 0, parent);

                }
                else {
                    /* Para repartirse a sí mismo no tiene sentido utilizar MPI */
                    MPI_Send(&buffer, 2, MPI_INT, 0, 0, parent);
                    mano[i] = mazo[k];
                    buffer[0] = rank;
                    buffer[1] = i;
                    buffer[2] = mano[i].valor;
                    MPI_Send(&buffer, 3, MPI_INT, 0, 0, parent);
                }
                sizeMazo--; /* es necesario almacenar el tamaño del mazo después de repartir */
                sizeDescartadas++;
                k++; /* un contador auxiliar para recuperar cartas del mazo */
            }
        }

        /* Todos los procesos deben conocer cuál es el tamaño actual del mazo */
        MPI_Send(&sizeMazo, 1, MPI_INT, 0, 0, parent);
        MPI_Bcast(&sizeMazo, 1, MPI_INT, postre, MPI_COMM_WORLD);


    }
    else { /* el proceso no es corte ni postre; es decir, un jugador estándar */
        int i = 0;
        for (i = 0; i < N_CARTAS_MANO; i++) {

            mano[i] = recibirCarta(postre, MPI_COMM_WORLD, stat);
            buffer[0] = rank;
            buffer[1] = i;
            buffer[2] = mano[i].valor;
            MPI_Send(&buffer, 3, MPI_INT, 0, 0, parent);
        }
        MPI_Bcast(&sizeMazo, 1, MPI_INT, postre, MPI_COMM_WORLD);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    /* una vez repartidas las cartas, se evalúan para cada uno de los lances */
    int cuenta = 0;
    int i =0;
    for (i=N_CARTAS_PALO-1;i>=0;i--) {
        cuenta = cuentaCartasMano(mano, caras[i]);
        cuentaCartas[N_CARTAS_PALO - i - 1] = cuenta;
        //printf("CUENTA %d para carta %s: %d\n", i, caras[i], cuenta);
    }
    //for (i=0;i<10;i++){
    //    printf("[jugador %d] CUENTA %d: %d\n", rank, i, cuentaCartas[i]);
   // }
    //int counts[5]={10, 10, 10, 10, 0};
    //int displs[5]={0, 10, 20, 30, 40};
    //MPI_Gatherv(cuentaCartas, 10, MPI_INT, rbuf, counts, displs, MPI_INT, 0, parent);
    MPI_Gather(cuentaCartas, 10, MPI_INT, rbuf, 10, MPI_INT, 0, parent);

    printf("MANO DEL JUGADOR %d\n", rank);
    printMazo(mano, N_CARTAS_MANO);
    MPI_Comm_disconnect(&parent);
    MPI_Finalize();
    return 0;
}