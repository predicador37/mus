//
// Created by predicador on 15/06/16.
//
#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include "dbg.h"
#include "mus.h"

#define N_CARTAS_MAZO 40
#define N_CARTAS_MANO 4
#define N_CARTAS_PALO 10
#define N_PALOS 4
#define N_JUGADORES 4
#define N_LANCES 4
#define DEBUG 1

int main(int argc, char **argv) {
    /*
     * DECLARACIÓN DE VARIABLES
     */
    int rank, size, namelen, version, subversion, psize, token, corte, repartidor, postre, mano, size_mazo, size_descartadas, siguiente_jugador, mus;

    const char * const caras[] = {"As", "Dos", "Tres", "Cuatro", "Cinco",
                                  "Seis", "Siete", "Sota", "Caballo", "Rey"};
    const char * const palos[] = {"Oros", "Copas", "Espadas", "Bastos"};
    char processor_name[MPI_MAX_PROCESSOR_NAME], worker_program[100];
    int cuentaCartas[N_CARTAS_PALO];
    Carta mazo[N_CARTAS_MAZO];
    Carta mano_cartas[N_CARTAS_MANO];
    char *palo_corte;
    MPI_Comm juego_comm;
    MPI_Status stat;

    /*
     * INICIALIZACIÓN DE MPI
     */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm parent;
    MPI_Get_processor_name(processor_name, &namelen);
    MPI_Get_version(&version, &subversion);

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
    MPI_Recv(&token, 1, MPI_INT, 0, 0, parent, &stat);
    MPI_Send(&rank, 1, MPI_INT, 0, 0, parent);

    /*
     * DETERMINACIÓN DE PROCESO REPARTIDOR PARA MUS CORRIDO
     */
    MPI_Bcast(&corte, 1, MPI_INT, 0, parent);
    MPI_Bcast(&size_mazo, 1, MPI_INT, 0, parent);
    if (rank == corte) {
        determinar_repartidor(corte, repartidor, palo_corte, mazo, parent, palos, stat);
    }
    MPI_Bcast(&repartidor, 1, MPI_INT, 0, parent); //recepción del repartidor desde el proceso maestro

    /*
     * REPARTO DE CARTAS PARA MUS CORRIDO
     */
    if (rank == repartidor) {
        repartidor_reparte(rank, repartidor, size_mazo, size_descartadas, mazo, mano_cartas, parent, stat);
        /* El proceso maestro debe contar con el mazo actualizado */
        MPI_Send(&size_mazo, 1, MPI_INT, 0, 0, parent);
        enviar_mazo(mazo, 0, parent, N_CARTAS_MAZO); // se devuelve el mazo al maestro
    }
    else { /* el proceso no es repartidor; es decir, un jugador estándar */
       jugador_recibe_cartas(rank, repartidor, mano_cartas, parent, stat);


    }//se termina el reparto

    MPI_Bcast(&size_mazo, 1, MPI_INT, 0, parent); //recepción del tamaño después de repartir



    mus = 0;
    token=0;
    while (token != 2) {
        MPI_Recv(&token, 1, MPI_INT, 0, 0, parent, &stat);

        /*
         * Evaluación de la mano del jugador
         */

        int i = 0;
        int juego = 0;
        int invertido[N_CARTAS_PALO];
        int pares[5];
        int equivalencias[N_CARTAS_MANO];


        for (i = 0; i < N_CARTAS_MANO; i++) {
            equivalencias[i] = mano_cartas[i].equivalencia;
        }

        int valores[N_CARTAS_MANO];
        for (i = 0; i < N_CARTAS_MANO; i++) {
            valores[i] = mano_cartas[i].valor;
        }

        /* Grande */
        int cuenta = 0;

        for (i = N_CARTAS_PALO - 1; i >= 0; i--) {
            cuenta = cuentaCartasMano(mano_cartas, caras[i]);

            cuentaCartas[N_CARTAS_PALO - i - 1] = cuenta;

        }

        /* chica */

        invertirArray(cuentaCartas, invertido, N_CARTAS_PALO);

        /* pares */

        preparaPares(equivalencias, pares);

        /* juego */

        juego = sumaArray(valores, N_CARTAS_MANO);


        if (token == 1) { //me toca el turno

            enviar_mazo(mano_cartas, 0, parent, N_CARTAS_MANO); // se envía la mano al maestro para E/S
            // Si recibe del maestro es porque le toca
            recibir_mazo(mazo, 0, parent, N_CARTAS_MAZO, &stat);
            // hay que decidir si mus o no mus


            mus = cortarMus(valores, equivalencias, pares);
            MPI_Send(&mus, 1, MPI_INT, 0, 0, parent);
            if (mus == 0) {
                enviar_mazo(mazo, 0, parent, N_CARTAS_MAZO); // se devuelve el mazo al maestro
            }
            else {
                token=2;
            }
        }
    }
    MPI_Comm_disconnect(&parent);
    MPI_Finalize();
    return 0;
}