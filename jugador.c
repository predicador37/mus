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
    int rank, size, namelen, version, subversion, psize, token, corte, repartidor, postre, mano, size_mazo, size_descartadas, siguiente_jugador, mus, descarte;

    const char *const caras[] = {"As", "Dos", "Tres", "Cuatro", "Cinco",
                                 "Seis", "Siete", "Sota", "Caballo", "Rey"};
    const char *const palos[] = {"Oros", "Copas", "Espadas", "Bastos"};
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
        jugador_recibe_cartas(rank, repartidor, mano_cartas, parent, &stat);


    }//se termina el reparto

    MPI_Bcast(&size_mazo, 1, MPI_INT, 0, parent); //recepción del tamaño después de repartir



    mus = 0;
    token = 0;
//token = 1 : evaluar + decidir mus
//token = 2 : no mus
//token = 3 : descartar
//token = 4 : repartir
    while (mus == 0) {
        debug("jugador %d esperando token...", rank);
        MPI_Recv(&token, 1, MPI_INT, 0, 0, parent, &stat);
        debug("Token=%d recibido por jugador %d", token, rank);
        switch (token) {

            case 1: //evaluar mano y decidir si mus o no mus
                /*
                 * Evaluación de la mano del jugador
                 */

                enviar_mazo(mano_cartas, 0, parent, N_CARTAS_MANO); // se envía la mano al maestro para E/S
                // Si recibe del maestro es porque le toca
                recibir_mazo(mazo, 0, parent, N_CARTAS_MAZO, &stat);
                // hay que decidir si mus o no mus
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

                mus = cortarMus(valores, equivalencias, pares);
                MPI_Send(&mus, 1, MPI_INT, 0, 0, parent);
                if (mus == 0) {
                    enviar_mazo(mazo, 0, parent, N_CARTAS_MAZO); // se devuelve el mazo al maestro
                }
                break;

            case 2: //otro jugador corta mus
                mus = 1;
                break;
            case 3: //jugador envía descartes a repartidor a través de maestro

                for (i = 0; i < N_CARTAS_MANO; i++) {
                    if (mano_cartas[i].equivalencia != 10) {
                        debug("jugador %d hace descarte", rank);
                        MPI_Send(&mano_cartas[i].id, 1, MPI_INT, 0, 0, parent); //envío de descarte
                        mano_cartas[i] = recibir_carta(0, parent, &stat);
                        valores[i] = mano_cartas[i].valor;
                        equivalencias[i] = mano_cartas[i].equivalencia;
                        // enviar peticion de carta y descarte

                    }
                    else {
                        debug("jugador %d NO descarta", rank);
                        descarte = 99;
                        MPI_Send(&descarte, 1, MPI_INT, 0, 0, parent);

                    }

                }
                break;
            case 4: //jugador reparte descartes
                debug("Jugador: %d reparte descartes", rank);
                recibir_mazo(mazo, 0, parent, N_CARTAS_MAZO, &stat);
                i=0;
                int j;
                while(i<4) {
                // repartidor pregunta a siguiente judador cuantas cartas a través de maestro y las reparte
                    debug(" [repartidor %d] Reparto cartas, iteración %d", rank, i);
                for (j = 0; j < N_CARTAS_MANO; j++) {
                    if (i==3) {
                        debug("jugador %d hace descarte", rank);
                        MPI_Send(&mano_cartas[i].id, 1, MPI_INT, 0, 0, parent); //envío de descarte
                    }
                    MPI_Recv(&descarte, 1, MPI_INT, 0, 0, parent, &stat);
                    debug("Repartidor: %d recibe descarte", rank);
                    if (descarte != 99 && descarte != 98) {
                        marcar_descarte(mazo, N_CARTAS_MAZO, descarte);
                        repartir_carta(mazo[N_CARTAS_MAZO - size_mazo], 0, parent);
                        debug("Repartidor: %d reparte carta", rank);
                        mazo[N_CARTAS_MAZO - size_mazo].estado = 1;
                        size_mazo--;
                        if (i==3) { // repartidor se reparte a sí mismo
                            mano_cartas[i] = recibir_carta(0, parent, &stat);
                            valores[i] = mano_cartas[i].valor;
                            equivalencias[i] = mano_cartas[i].equivalencia;
                        }
                    }
                }

                    i++;
                }
                break;


        }
        if (mus == 1) {
            break;
        }
        }

    debug("[jugador %d] FINALIZADO", rank);
    MPI_Comm_disconnect(&parent);
    MPI_Finalize();
    return 0;
}