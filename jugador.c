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
#define DEBUG 0
#define CHAR_BUFFER 8

extern const char *caras[];
extern const char *palos[];
extern const char *lancesEtiquetas[];
extern int valores[];
extern int equivalencias[];

int main(int argc, char **argv) {
    /*
     * DECLARACIÓN DE VARIABLES
     */
    int rank, size, namelen, version, subversion, psize, modo_juego, ronda, token, corte, palo_corte, repartidor, postre, mano, jugador_humano, size_mazo, size_mano, size_descartadas, siguiente_jugador, mus, descarte, n_cartas_a_descartar, apuesta_en_vigor, jugador_apuesta_en_vigor, envite, cuenta, indicador_pares, juego_al_punto;

    char processor_name[MPI_MAX_PROCESSOR_NAME], worker_program[100];
    int cuenta_cartas[N_CARTAS_PALO], equivalencias_jugador[N_CARTAS_MANO];
    //Array de 4 posiciones para los envites, una para cada jugador
    //0: no ha hablado
    //1: paso
    //2: envido (2 piedras, apuesta mínima)
    //3-99: envido N piedras
    int envites_jugadores[N_JUGADORES] = {0, 0, 0, 0};
    int envites[2] = {0, 0};
    int puntos_juego[2] = {0, 0};
    int n_puntos_juego = 40;
    int fin_partida = 0;
    int indicador_ordago = 0;
    int cartas_a_descartar[N_CARTAS_MANO] = {99, 99, 99, 99};
    int rbuf[50]; //buffer de recepcion para evaluar jugadas

    Carta mazo[N_CARTAS_MAZO];
    Carta mano_cartas[N_CARTAS_MANO];
    //char * palo_corte = NULL;
    MPI_Comm juego_comm;
    MPI_Status stat;
    MPI_Comm parent;

    srand(time(NULL)); /* randomize */

    /******************************************************************************************************************
     * INICIALIZACIÓN DE MPI
     ******************************************************************************************************************/

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Get_processor_name(processor_name, &namelen);
    MPI_Get_version(&version, &subversion);

    MPI_Comm_get_parent(&parent);
    int i = 0;
    int juego = 0;
    int invertido[N_CARTAS_PALO];
    int pares[5];
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

    /******************************************************************************************************************
     * COMIENZO DE PARTIDA  *****************************************************************************************************************/

    MPI_Bcast(&modo_juego, 1, MPI_INT, 0, parent);
    MPI_Bcast(&jugador_humano, 1, MPI_INT, 0, parent);
    MPI_Bcast(&n_puntos_juego, 1, MPI_INT, 0, parent);

    int iteracion = 0;
    while (fin_partida == 0) {
        MPI_Bcast(&ronda, 1, MPI_INT, 0, parent);

        if (ronda == 0) { //primera ronda: corte, reparto inicial y mus corrido

            /*
             * DETERMINACIÓN DE PROCESO REPARTIDOR PARA MUS CORRIDO
             */

            MPI_Bcast(&corte, 1, MPI_INT, 0, parent);
            MPI_Bcast(&size_mazo, 1, MPI_INT, 0, parent);


            if (rank == corte) {

                recibir_mazo(mazo, 0, parent, N_CARTAS_MAZO, &stat);
                debug("Mazo recibido");
                int r; // índice aleatorio para el mazo
                r = rand_lim(N_CARTAS_MAZO - 1);
                palo_corte = mazo[r].palo;
                debug("Palo de la carta cortada: %s con id: %d", palos[palo_corte], palo_corte);

                // corresponde repartir primero al primer jugador de su derecha si sale oros;
                // al segundo si sale copas; al tercero si sale espadas y al mismo que cortó si sale bastos*//*

                repartidor = add_mod(corte, palo_corte + 1, 4);
                debug("Repartidor %d calculado", repartidor);

                // Envío del id del repartidor al proceso maestro

                MPI_Send(&repartidor, 1, MPI_INT, 0, 0, parent);
                debug("Repartidor %d enviado", repartidor);


                enviar_mazo(mazo, 0, parent, N_CARTAS_MAZO); //se devuelve el mazo al maestro
            }
            MPI_Bcast(&repartidor, 1, MPI_INT, 0, parent); //recepción del repartidor desde el proceso maestro

            /*
             * REPARTO DE CARTAS PARA MUS CORRIDO
             */
            if (rank == repartidor) {
                size_mazo = repartidor_reparte(rank, repartidor, size_mazo, size_descartadas, mazo, mano_cartas, parent,
                                               stat);
                // El proceso maestro debe contar con el mazo actualizado
                MPI_Send(&size_mazo, 1, MPI_INT, 0, 0, parent);
                enviar_mazo(mazo, 0, parent, N_CARTAS_MAZO); // se devuelve el mazo al maestro
            }
            else { // el proceso no es repartidor; es decir, un jugador estándar
                jugador_recibe_cartas(rank, repartidor, mano_cartas, parent, &stat);


            }//se termina el reparto

            MPI_Bcast(&size_mazo, 1, MPI_INT, 0, parent); //recepción del tamaño después de repartir


            MPI_Recv(&token, 1, MPI_INT, 0, 0, parent, &stat); //sincronización con maestro
            enviar_mazo(mano_cartas, 0, parent, N_CARTAS_MANO); // envío de manos para E/S
            token = 0; //reseteo de variable para el futuro

            /* EVALUACION PREVIA DE CARTAS */

            i = 0;
            juego = 0;

            int valores_jugador[N_CARTAS_MANO];
            for (i = 0; i < N_CARTAS_MANO; i++) {
                valores_jugador[i] = valores[mano_cartas[i].cara];
            }

            //Grande

            cuenta = 0;

            for (i = N_CARTAS_PALO - 1; i >= 0; i--) {
                cuenta = cuenta_cartas_mano(mano_cartas, i);

                cuenta_cartas[N_CARTAS_PALO - i - 1] = cuenta;

            }

            // chica

            invertirArray(cuenta_cartas, invertido, N_CARTAS_PALO);

            // pares

            preparaPares(equivalencias_jugador, pares);

            // juego

            juego = sumaArray(valores_jugador, N_CARTAS_MANO);


            /**********************************************************************************************************
             * MUS CORRIDO
             **********************************************************************************************************/

            mus = 0;
            token = 0;
//token = 1 : evaluar + decidir mus
//token = 2 : no mus
//token = 3 : descartar
//token = 4 : repartir
            while (mus == 0) {


                debug("jugador %d esperando token inicio mus corrido...", rank);
                MPI_Recv(&token, 1, MPI_INT, 0, 0, parent, &stat);
                debug("Token=%d recibido por jugador %d", token, rank);
                switch (token) {

                    case 1: //evaluar mano y decidir si mus o no mus
                        /*
                         * Evaluación de la mano del jugador
                        */

                        enviar_mazo(mano_cartas, 0, parent, N_CARTAS_MANO); // se envía la mano al maestro para E/S
                        // Si recibe del maestro es porque le toca
                        MPI_Recv(&size_mazo, 1, MPI_INT, 0, 0, parent, &stat);
                        recibir_mazo(mazo, 0, parent, N_CARTAS_MAZO, &stat);
                        // hay que decidir si mus o no mus


                        debug("[jugador %d] modo juego = %d, rank = %d", rank, modo_juego, rank);
                        if ((modo_juego == 1) && (rank == jugador_humano)) {//jugador humano
                            MPI_Recv(&mus, 1, MPI_INT, 0, 0, parent, &stat);
                            debug("[jugador %d] JUGADOR HUMANO RECIBE MUS INTRODUCIDO: %d\n", rank, mus);

                        }
                        else { //modo automático
                            mus = cortarMus(valores_jugador, equivalencias_jugador, pares);
                            MPI_Send(&mus, 1, MPI_INT, 0, 0, parent);

                        }
                        if (mus == 0) {
                            MPI_Send(&size_mazo, 1, MPI_INT, 0, 0, parent);
                            enviar_mazo(mazo, 0, parent, N_CARTAS_MAZO); // se devuelve el mazo al maestro
                        }
                        break;

                    case 2: //otro jugador corta mus
                        mus = 1;
                        debug("jugador %d RECIBE FIN MUS...", rank);
                        break;
                    case 3: //jugador envía descartes a repartidor a través de maestro

                        if ((modo_juego == 1) && (jugador_humano == rank)) {//jugador humano
                            enviar_mazo(mano_cartas, 0, parent, N_CARTAS_MANO); // se envía la mano al maestro para E/S
                            MPI_Recv(&n_cartas_a_descartar, 1, MPI_INT, 0, 0, parent, &stat);
                            MPI_Recv(cartas_a_descartar, n_cartas_a_descartar, MPI_INT, 0, 0, parent, &stat);
                        }
                        else { //modo automático
                            // identificar cuántas cartas se van a descartar
                            // identificar qué cartas se van a descartar
                            n_cartas_a_descartar = 0;
                            for (i = 0; i < N_CARTAS_MANO; i++) {
                                cartas_a_descartar[i] = 99;
                                if (equivalencias[mano_cartas[i].cara] != 10) {
                                    debug("jugador %d hace descarte", rank);
                                    cartas_a_descartar[n_cartas_a_descartar] = mano_cartas[i].id;
                                    n_cartas_a_descartar++;

                                }

                            }


                            // enviar número de cartas que se van a descartar
                            MPI_Send(&n_cartas_a_descartar, 1, MPI_INT, 0, 0, parent); //cuantas cartas quiero

                            //llegados a este punto siempre va a haber descartes
                            // enviar ids de cartas a descartar en un array
                            MPI_Send(cartas_a_descartar, n_cartas_a_descartar, MPI_INT, 0, 0,
                                     parent); //envío de descartes

                        }
                        int k;
                        //recibir cartas nuevas

                        for (i = 0; i < N_CARTAS_MANO; i++) {

                            for (k = 0; k < n_cartas_a_descartar; k++) {
                                if (mano_cartas[i].id == cartas_a_descartar[k]) {
                                    debug("[jugador %d] Descartando carta con id %d y k = %d\n", rank,
                                          cartas_a_descartar[k], k);
                                    mano_cartas[i] = recibir_carta(0, parent, &stat);
                                    debug("[jugador %d] Recibida carta con id %d y cara = %d\n", rank,
                                          mano_cartas[i].id, mano_cartas[i].cara);
                                    valores_jugador[i] = valores[mano_cartas[i].cara];
                                    equivalencias_jugador[i] = equivalencias[mano_cartas[i].cara];
                                    break;
                                }
                            }
                        }

                        break;
                        /*********************************************************************************************
                         * FASE DE DESCARTES MUS CORRIDO
                         *********************************************************************************************/
                    case 4: //jugador reparte descartes
                        debug("Jugador: %d reparte descartes", rank);
                        MPI_Recv(&size_mazo, 1, MPI_INT, 0, 0, parent, &stat);
                        recibir_mazo(mazo, 0, parent, N_CARTAS_MAZO, &stat);

                        i = 0;
                        int j;
                        while (i < 4) {

                            if (i == 3) { //soy el repartidor
                                if ((modo_juego == 1) && (jugador_humano == rank)) {//jugador humano
                                    enviar_mazo(mano_cartas, 0, parent,
                                                N_CARTAS_MANO); // se envía la mano al maestro para E/S
                                    MPI_Recv(&n_cartas_a_descartar, 1, MPI_INT, 0, 0, parent, &stat);
                                    MPI_Recv(cartas_a_descartar, n_cartas_a_descartar, MPI_INT, 0, 0, parent, &stat);
                                } else {
                                    debug("[jugador %d] Me toca repartir.\n", rank);
                                    n_cartas_a_descartar = 0;
                                    for (j = 0; j < N_CARTAS_MANO; j++) {
                                        cartas_a_descartar[i] = 99;
                                        if (equivalencias[mano_cartas[j].cara] !=
                                            10) { //Descartamos carta que no sea un rey
                                            debug("jugador %d hace descarte", rank);
                                            cartas_a_descartar[n_cartas_a_descartar] = mano_cartas[j].id;
                                            n_cartas_a_descartar++;

                                        }

                                    }
                                    debug("REPARTIDOR DESCARTA %d CARTAS \n", n_cartas_a_descartar);
                                    MPI_Send(&n_cartas_a_descartar, 1, MPI_INT, 0, 0, parent); //cuantas cartas quiero

                                    //llegados a este punto siempre va a haber descartes
                                    // enviar ids de cartas a descartar en un array
                                    MPI_Send(cartas_a_descartar, n_cartas_a_descartar, MPI_INT, 0, 0,
                                             parent); //envío de descartes
                                }
                            }

                            // repartidor pregunta a siguiente judador cuantas cartas a través de maestro y las reparte
                            debug(" [repartidor %d] Reparto cartas, iteración %d", rank, i);

                            // repartidor recibe de maestro cuántas cartas se va a descartar el jugador
                            MPI_Recv(&n_cartas_a_descartar, 1, MPI_INT, 0, 0, parent, &stat);
                            // repartidor recibe de maestro qué cartas se va a descartar el jugador
                            MPI_Recv(cartas_a_descartar, n_cartas_a_descartar, MPI_INT, 0, 0, parent, &stat);

                            MPI_Send(&size_mazo, 1, MPI_INT, 0, 0, parent); //envío de tamaño del mazo


                            debug("CARTAS EN MANOS: %d\n", contar_cartas_en_estado(mazo, 1));

                            for (j = 0; j < n_cartas_a_descartar; j++) {
                                marcar_descarte(mazo, N_CARTAS_MAZO,
                                                cartas_a_descartar[j]); //primero se tiran las cartas

                            }


                            for (j = 0; j < n_cartas_a_descartar; j++) { //luego se reparten N

                                //y si el mazo se queda sin cartas....

                                while (mazo[N_CARTAS_MAZO - size_mazo].estado != 0) {
                                    debug("CARTA YA REPARTIDA; PASANDO con size_mazo %d e indice de array %d...\n",
                                          size_mazo, N_CARTAS_MAZO - size_mazo);
                                    size_mazo--; //ojo al repartir con el mazo virtual; las que no están en estado 0 no están en el mazo

                                    if (size_mazo == 0) {
                                        debug("ATENCIÓN: MAZO SIN CARTAS. VOLVER A MEZCLAR!!!\n");
                                        int vuelta_al_mazo = poner_descartadas_en_mazo(mazo);
                                        debug("[jugador %d] DEVUELTAS AL MAZO: %d\n", rank, vuelta_al_mazo);
                                        barajar_mazo(mazo); //Baraja el mazo
                                        print_mazo(mazo, N_CARTAS_MAZO);
                                        size_mazo = N_CARTAS_MAZO; // Reestablece el contador para recorrer cartas (representa carta arriba)
                                        //enviar_mazo(mazo, 0, parent, N_CARTAS_MAZO); // se devuelve el mazo al maestro
                                    }
                                }
                                if (mazo[N_CARTAS_MAZO - size_mazo].estado == 0) {

                                    mazo[N_CARTAS_MAZO - size_mazo].estado = 1;
                                    repartir_carta(mazo[N_CARTAS_MAZO - size_mazo], 0, parent);
                                    debug("[jugador %d] Repartiendo carta con size_mazo %d\n", rank, size_mazo);
                                    size_mazo--;
                                    debug("Cursor size_mazo: %d\n", size_mazo);

                                    debug("[jugador %d] CARTAS EN MANOS: %d\n", rank,
                                          contar_cartas_en_estado(mazo, 1));
                                    debug("[jugador %d] CARTAS EN MAZO: %d\n", rank, contar_cartas_en_estado(mazo, 0));
                                    debug("[jugador %d] CARTAS DESCARTADAS: %d\n", rank,
                                          contar_cartas_en_estado(mazo, 2));

                                    if (size_mazo == 0) {
                                        debug("ATENCIÓN: MAZO SIN CARTAS. VOLVER A MEZCLAR!!!\n");
                                        debug("ATENCIÓN: MAZO SIN CARTAS. VOLVER A MEZCLAR!!!\n");
                                        int vuelta_al_mazo = poner_descartadas_en_mazo(mazo);
                                        debug("[jugador %d] DEVUELTAS AL MAZO: %d\n", rank, vuelta_al_mazo);
                                        barajar_mazo(mazo); //Baraja el mazo
                                        print_mazo(mazo, N_CARTAS_MAZO);
                                        size_mazo = N_CARTAS_MAZO; // Reestablece el contador para recorrer cartas (representa carta arriba)
                                        //enviar_mazo(mazo, 0, parent, N_CARTAS_MAZO); // se devuelve el mazo al maestro
                                    }
                                } else {
                                    debug("[ERROR] Error grave.\n");
                                }

                                if (i == 3) { // repartidor se reparte a sí mismo
                                    debug("[jugador %d] REPARTIDOR SE REPARTE A SI MISMO\n", rank);
                                    int k;
                                    debug("[jugador %d] Equivalencias antes de descartes: ", rank);
                                    for (k = 0; k < N_CARTAS_MANO; k++) {
                                        debug("%d ", equivalencias_jugador[k]);
                                    }
                                    debug("\n");
                                    for (k = 0; k < N_CARTAS_MANO; k++) {
                                        if (mano_cartas[k].id == cartas_a_descartar[j]) {
                                            debug("Carta a reemplazar con id %d\n", cartas_a_descartar[j]);
                                            debug("[jugador %d] Descartando carta con id %d y j = %d\n", rank,
                                                  cartas_a_descartar[j], j);
                                            mano_cartas[k] = recibir_carta(0, parent, &stat);
                                            valores_jugador[k] = valores[mano_cartas[k].cara];
                                            equivalencias_jugador[k] = equivalencias[mano_cartas[k].cara];
                                            break;
                                        }
                                    }

                                }


                            }
                            //se han actualizado las equivalencias, luego hay que actualizar los pares

                            i++; //incrementar jugador
                        }
                        debug("[jugador %d] Final de caso repartidor\n", rank);
                        MPI_Send(&size_mazo, 1, MPI_INT, 0, 0, parent); //envío de tamaño del mazo
                        enviar_mazo(mazo, 0, parent, N_CARTAS_MAZO); // se devuelve el mazo al maestro
                        break;

                }
                //  if (mus == 1) {
                //     break;
                //}
            } //fin while mus==0
            //sincronización del jugador mano
            MPI_Bcast(&mano, 1, MPI_INT, 0, parent);
        } //end if ronda inicial

            /*************************************************************************************************************
         * MUS NORMAL
         *************************************************************************************************************/

        else { //rondas normales sin mus corrido
            MPI_Bcast(&size_mazo, 1, MPI_INT, 0, parent);
            MPI_Bcast(&repartidor, 1, MPI_INT, 0, parent); //recepción del repartidor desde el proceso maestr
            MPI_Bcast(&mano, 1, MPI_INT, 0, parent);
            postre = repartidor;
            /*
             * REPARTO DE CARTAS PARA MUS
             */
            if (rank == repartidor) {
                size_mazo = repartidor_reparte(rank, repartidor, size_mazo, size_descartadas, mazo, mano_cartas, parent,
                                               stat);
                // El proceso maestro debe contar con el mazo actualizado
                MPI_Send(&size_mazo, 1, MPI_INT, 0, 0, parent);
                enviar_mazo(mazo, 0, parent, N_CARTAS_MAZO); // se devuelve el mazo al maestro
            }
            else { // el proceso no es repartidor; es decir, un jugador estándar
                jugador_recibe_cartas(rank, repartidor, mano_cartas, parent, &stat);


            }//se termina el reparto

            MPI_Bcast(&size_mazo, 1, MPI_INT, 0, parent); //recepción del tamaño después de repartir

            /* EVALUACION PREVIA DE CARTAS */

            i = 0;
            juego = 0;

            debug("[jugador %d] Equivalencias mano original: ", rank);
            for (i = 0; i < N_CARTAS_MANO; i++) {
                equivalencias_jugador[i] = equivalencias[mano_cartas[i].cara];
                debug("%d ", equivalencias_jugador[i]);
            }
            debug("\n");

            int valores_jugador[N_CARTAS_MANO];
            for (i = 0; i < N_CARTAS_MANO; i++) {
                valores_jugador[i] = valores[mano_cartas[i].cara];
            }

            //Grande

            cuenta = 0;

            for (i = N_CARTAS_PALO - 1; i >= 0; i--) {
                cuenta = cuenta_cartas_mano(mano_cartas, i);

                cuenta_cartas[N_CARTAS_PALO - i - 1] = cuenta;

            }

            // chica

            invertirArray(cuenta_cartas, invertido, N_CARTAS_PALO);

            // pares


            preparaPares(equivalencias_jugador, pares);

            // juego

            juego = sumaArray(valores_jugador, N_CARTAS_MANO);

            /* RONDA DE MUS: QUIERES MUS? */

            mus = 0;
            token = 0;
//token = 1 : evaluar + decidir mus
//token = 2 : no mus
//token = 3 : descartar
//token = 4 : repartir
            enviar_mazo(mano_cartas, 0, parent, N_CARTAS_MANO); // se envía la mano al maestro para E/S
            while (mus == 0) {


                debug("jugador %d esperando token inicio mus...", rank);
                MPI_Recv(&token, 1, MPI_INT, 0, 0, parent, &stat);
                debug("Token=%d recibido por jugador %d", token, rank);
                switch (token) {

                    case 1: //evaluar mano y decidir si mus o no mus
                        /*
                         * Evaluación de la mano del jugador
                        */

                        enviar_mazo(mano_cartas, 0, parent, N_CARTAS_MANO); // se envía la mano al maestro para E/S

                        // hay que decidir si mus o no mus
                        debug("[jugador %d] modo juego = %d, rank = %d", rank, modo_juego, rank);
                        if ((modo_juego == 1) && (rank == jugador_humano)) {//jugador humano
                            MPI_Recv(&mus, 1, MPI_INT, 0, 0, parent, &stat);
                            debug("[jugador %d] JUGADOR HUMANO RECIBE MUS INTRODUCIDO: %d\n", rank, mus);

                        }
                        else { //modo automático
                            mus = cortarMus(valores_jugador, equivalencias_jugador, pares);
                            MPI_Send(&mus, 1, MPI_INT, 0, 0, parent);

                        }

                        break;

                    case 2: //otro jugador corta mus
                        mus = 1;
                        debug("jugador %d RECIBE FIN MUS...", rank);
                        break;
                    case 3: //jugador envía descartes a repartidor a través de maestro

                        if ((modo_juego == 1) && (jugador_humano == rank)) {//jugador humano
                            enviar_mazo(mano_cartas, 0, parent, N_CARTAS_MANO); // se envía la mano al maestro para E/S
                            MPI_Recv(&n_cartas_a_descartar, 1, MPI_INT, 0, 0, parent, &stat);
                            MPI_Recv(cartas_a_descartar, n_cartas_a_descartar, MPI_INT, 0, 0, parent, &stat);
                        }
                        else { //modo automático
                            // identificar cuántas cartas se van a descartar
                            // identificar qué cartas se van a descartar
                            n_cartas_a_descartar = 0;
                            for (i = 0; i < N_CARTAS_MANO; i++) {
                                cartas_a_descartar[i] = 99;
                                if (equivalencias[mano_cartas[i].cara] != 10) {
                                    debug("jugador %d hace descarte", rank);
                                    cartas_a_descartar[n_cartas_a_descartar] = mano_cartas[i].id;
                                    n_cartas_a_descartar++;

                                }

                            }


                            // enviar número de cartas que se van a descartar
                            MPI_Send(&n_cartas_a_descartar, 1, MPI_INT, 0, 0, parent); //cuantas cartas quiero

                            //llegados a este punto siempre va a haber descartes
                            // enviar ids de cartas a descartar en un array
                            MPI_Send(cartas_a_descartar, n_cartas_a_descartar, MPI_INT, 0, 0,
                                     parent); //envío de descartes

                        }
                        int k;

                        //recibir cartas nuevas
                        //int k;
                        for (i = 0; i < N_CARTAS_MANO; i++) {

                            for (k = 0; k < n_cartas_a_descartar; k++) {
                                if (mano_cartas[i].id == cartas_a_descartar[k]) {
                                    debug("[jugador %d] Descartando carta con id %d y k = %d\n", rank,
                                          cartas_a_descartar[k], k);
                                    mano_cartas[i] = recibir_carta(0, parent, &stat);
                                    valores_jugador[i] = valores[mano_cartas[i].cara];
                                    equivalencias_jugador[i] = equivalencias[mano_cartas[i].cara];
                                    break;
                                }
                            }
                        }
                        //se han actualizado las equivalencias, hay que actualizar los pares
                        debug("[jugador %d] Equivalencias después de recibir cartas: \n", rank);
                        for (k = 0; k < N_CARTAS_MANO; k++) {
                            debug("%d ", equivalencias_jugador[k]);
                        }
                        debug("\n");

                        break;
                        /**************************************************************************************************
                     * FASE DE DESCARTES MUS NORMAL
                     **************************************************************************************************/
                    case 4: //jugador reparte descartes
                        debug("Jugador: %d reparte descartes", rank);
                        MPI_Recv(&size_mazo, 1, MPI_INT, 0, 0, parent, &stat);
                        recibir_mazo(mazo, 0, parent, N_CARTAS_MAZO, &stat);

                        i = 0;
                        int j;
                        while (i < 4) {

                            if (i == 3) { //soy el repartidor
                                if ((modo_juego == 1) && (jugador_humano == rank)) {//jugador humano
                                    enviar_mazo(mano_cartas, 0, parent,
                                                N_CARTAS_MANO); // se envía la mano al maestro para E/S
                                    MPI_Recv(&n_cartas_a_descartar, 1, MPI_INT, 0, 0, parent, &stat);
                                    MPI_Recv(cartas_a_descartar, n_cartas_a_descartar, MPI_INT, 0, 0, parent, &stat);
                                } else {
                                    debug("[jugador %d] SOY EL REPARTIDOR\n", rank);
                                    n_cartas_a_descartar = 0;
                                    for (j = 0; j < N_CARTAS_MANO; j++) {
                                        cartas_a_descartar[i] = 99;
                                        if (equivalencias[mano_cartas[j].cara] !=
                                            10) { //Descartamos carta que no sea un rey
                                            debug("jugador %d hace descarte", rank);
                                            cartas_a_descartar[n_cartas_a_descartar] = mano_cartas[j].id;
                                            n_cartas_a_descartar++;

                                        }

                                    }
                                    debug("REPARTIDOR DESCARTA %d CARTAS \n", n_cartas_a_descartar);
                                    MPI_Send(&n_cartas_a_descartar, 1, MPI_INT, 0, 0, parent); //cuantas cartas quiero

                                    //llegados a este punto siempre va a haber descartes
                                    // enviar ids de cartas a descartar en un array
                                    MPI_Send(cartas_a_descartar, n_cartas_a_descartar, MPI_INT, 0, 0,
                                             parent); //envío de descartes
                                }
                            }

                            // repartidor pregunta a siguiente judador cuantas cartas a través de maestro y las reparte
                            debug(" [repartidor %d] Reparto cartas, iteración %d", rank, i);

                            // repartidor recibe de maestro cuántas cartas se va a descartar el jugador
                            MPI_Recv(&n_cartas_a_descartar, 1, MPI_INT, 0, 0, parent, &stat);
                            // repartidor recibe de maestro qué cartas se va a descartar el jugador
                            MPI_Recv(cartas_a_descartar, n_cartas_a_descartar, MPI_INT, 0, 0, parent, &stat);

                            MPI_Send(&size_mazo, 1, MPI_INT, 0, 0, parent); //envío de tamaño del mazo


                            debug("CARTAS EN MANOS: %d\n", contar_cartas_en_estado(mazo, 1));

                            for (j = 0; j < n_cartas_a_descartar; j++) {
                                marcar_descarte(mazo, N_CARTAS_MAZO,
                                                cartas_a_descartar[j]); //primero se tiran las cartas

                            }

                            for (j = 0; j < n_cartas_a_descartar; j++) { //luego se reparten N

                                //y si el mazo se queda sin cartas....

                                while (mazo[N_CARTAS_MAZO - size_mazo].estado != 0) {
                                    debug("CARTA YA REPARTIDA; PASANDO con size_mazo %d e indice de array %d...\n",
                                          size_mazo, N_CARTAS_MAZO - size_mazo);
                                    size_mazo--; //ojo al repartir con el mazo virtual; las que no están en estado 0 no están en el mazo

                                    if (size_mazo == 0) {
                                        debug("ATENCIÓN: MAZO SIN CARTAS. VOLVER A MEZCLAR!!!\n");
                                        int vuelta_al_mazo = poner_descartadas_en_mazo(mazo);
                                        debug("[jugador %d] DEVUELTAS AL MAZO: %d\n", rank, vuelta_al_mazo);
                                        barajar_mazo(mazo); //Baraja el mazo
                                        print_mazo(mazo, N_CARTAS_MAZO);
                                        size_mazo = N_CARTAS_MAZO; // Reestablece el contador para recorrer cartas (representa carta arriba)
                                        //enviar_mazo(mazo, 0, parent, N_CARTAS_MAZO); // se devuelve el mazo al maestro
                                    }
                                }
                                if (mazo[N_CARTAS_MAZO - size_mazo].estado == 0) {

                                    mazo[N_CARTAS_MAZO - size_mazo].estado = 1;
                                    repartir_carta(mazo[N_CARTAS_MAZO - size_mazo], 0, parent);
                                    debug("[jugador %d] Repartiendo carta con size_mazo %d\n", rank, size_mazo);
                                    size_mazo--;
                                    debug("Cursor size_mazo: %d\n", size_mazo);

                                    debug("[jugador %d] CARTAS EN MANOS: %d\n", rank,
                                          contar_cartas_en_estado(mazo, 1));
                                    debug("[jugador %d] CARTAS EN MAZO: %d\n", rank, contar_cartas_en_estado(mazo, 0));
                                    debug("[jugador %d] CARTAS DESCARTADAS: %d\n", rank,
                                          contar_cartas_en_estado(mazo, 2));

                                    if (size_mazo == 0) {
                                        debug("ATENCIÓN: MAZO SIN CARTAS. VOLVER A MEZCLAR!!!\n");
                                        int vuelta_al_mazo = poner_descartadas_en_mazo(mazo);
                                        debug("[jugador %d] DEVUELTAS AL MAZO: %d\n", rank, vuelta_al_mazo);
                                        barajar_mazo(mazo); //Baraja el mazo
                                        print_mazo(mazo, N_CARTAS_MAZO);
                                        size_mazo = N_CARTAS_MAZO; // Reestablece el contador para recorrer cartas (representa carta arriba)
                                        //enviar_mazo(mazo, 0, parent, N_CARTAS_MAZO); // se devuelve el mazo al maestro
                                    }
                                } else {
                                    debug("[ERROR] Error grave.\n");
                                }

                                if (i == 3) { // repartidor se reparte a sí mismo
                                    debug("[jugador %d] REPARTIDOR SE REPARTE A SI MISMO\n", rank);
                                    int k;
                                    for (k = 0; k < N_CARTAS_MANO; k++) {
                                        if (mano_cartas[k].id == cartas_a_descartar[j]) {
                                            debug("Carta a reemplazar con id %d\n", cartas_a_descartar[j]);
                                            debug("[jugador %d] Descartando carta con id %d y j = %d\n", rank,
                                                  cartas_a_descartar[j], j);
                                            mano_cartas[k] = recibir_carta(0, parent, &stat);
                                            valores_jugador[k] = valores[mano_cartas[k].cara];
                                            equivalencias_jugador[k] = equivalencias[mano_cartas[k].cara];
                                            break;
                                        }
                                    }

                                }


                            }


                            i++; //incrementar jugador
                        }
                        debug("[jugador %d] Final de caso repartidor\n", rank);
                        MPI_Send(&size_mazo, 1, MPI_INT, 0, 0, parent); //envío de tamaño del mazo
                        enviar_mazo(mazo, 0, parent, N_CARTAS_MAZO); // se devuelve el mazo al maestro
                        break;

                }
                //  if (mus == 1) {
                //     break;
                //}
                // enviar_mazo(mano_cartas, 0, parent, N_CARTAS_MANO); // se envía la mano al maestro para E/S
            } //fin while mus==0




        }


        /* EVALUACION PREVIA DE CARTAS */

        i = 0;
        juego = 0;

        debug("[jugador %d] Equivalencias mano original: ", rank);
        for (i = 0; i < N_CARTAS_MANO; i++) {
            equivalencias_jugador[i] = equivalencias[mano_cartas[i].cara];
            debug("%d ", equivalencias_jugador[i]);
        }
        debug("\n");

        int valores_jugador[N_CARTAS_MANO];
        for (i = 0; i < N_CARTAS_MANO; i++) {
            valores_jugador[i] = valores[mano_cartas[i].cara];
        }

        //Grande

        cuenta = 0;

        for (i = N_CARTAS_PALO - 1; i >= 0; i--) {
            cuenta = cuenta_cartas_mano(mano_cartas, i);

            cuenta_cartas[N_CARTAS_PALO - i - 1] = cuenta;

        }

        // chica

        invertirArray(cuenta_cartas, invertido, N_CARTAS_PALO);

        // pares


        preparaPares(equivalencias_jugador, pares);

        // juego

        juego = sumaArray(valores_jugador, N_CARTAS_MANO);

        //Se asegura que el jugador mano es correcto
        MPI_Bcast(&mano, 1, MPI_INT, 0, parent);
        MPI_Bcast(&postre, 1, MPI_INT, 0, parent);
        if (rank == postre) { //recibir mazo
            recibir_mazo(mazo, 0, parent, N_CARTAS_MAZO, &stat);
        }
        enviar_mazo(mano_cartas, 0, parent, N_CARTAS_MANO); // se envía la mano al maestro para E/S
        /*************************************************************************************************************
           * FASE DE LANCES
           *************************************************************************************************************/


        //Array de 4 posiciones para los envites, una para cada jugador
        //0: no ha hablado
        //1: paso
        //2: envido (2 piedras, apuesta mínima)
        //3-99: envido N piedras

        //apuesta termina si:
        // 4 jugadores están en paso
        // 3 jugadores están en paso y 1 en 2-99
        // mayor apuesta de pareja 1 y mayor apuesta de pareja 2 son iguales (apuesta igualada)
        int l;
        for (l = 0; l < N_LANCES; l++) { // iterar N_LANCES...

            if (l == 2) { //lance de pares
                preparaPares(equivalencias_jugador, pares);
                indicador_pares = 0;
                int tengo_pares = tengoPares(pares);
                //enviar si se tienen pares o no
                MPI_Gather(&tengo_pares, 1, MPI_INT, rbuf, 1, MPI_INT, 0, parent);
                //recibir indicador de pares
                MPI_Recv(&indicador_pares, 1, MPI_INT, 0, 0, parent, &stat);
            }
            else if (l == 3) {
                juego = sumaArray(valores_jugador, N_CARTAS_MANO);
                int tengo_juego = tengoJuego(juego);
                //enviar si se tienen pares o no
                MPI_Gather(&tengo_juego, 1, MPI_INT, rbuf, 1, MPI_INT, 0, parent);
                MPI_Bcast(&juego_al_punto, 1, MPI_INT, 0, parent);

            }
            if ((l == 0) || (l == 1) || ((l == 2) && (indicador_pares ==
                                                        2)) || ((l == 3) && (juego_al_punto !=
                                                                             0))) { //los envites sólo tienen lugar si hay pares en ambas parejas y en el resto de los lances
                i = 0;
                while (i < N_JUGADORES) { // 4 turnos
                    debug("jugador %d esperando token...", rank);
                    token = 0;
                    MPI_Recv(&token, 1, MPI_INT, 0, 0, parent, &stat);
                    debug("Token=%d recibido por jugador %d", token, rank);

                    /****************************************************************************************************
                 * FASE DE ENVITES
                 *****************************************************************************************************/

                    switch (token) {
                        case 1: //decidir envite

                             if (((modo_juego == 1) && (rank == jugador_humano) && ((l == 0) || (l==1))) || //grande o chica
                                ((modo_juego == 1) && (rank == jugador_humano) && (l == 2) && (indicador_pares==2) && (tengoPares(pares) == 1)) || //pares y tengo pares
                                ((modo_juego == 1) && (rank == jugador_humano) && (l == 3) && (juego_al_punto == 2) && (tengoJuego(juego) == 1)) ||  //juego y tengo juego
                                ((modo_juego == 1) && (rank == jugador_humano) && (l == 3) && (juego_al_punto == 1))) { //enviar mazo para enseñarselo al jugador humano antes de envidar
                                printf("[jugador %d] Enviando cartas a maestro...\n", rank);
                                enviar_mazo(mano_cartas, 0, parent, N_CARTAS_MANO); // se envía la mano al maestro para E/S
                            }
                             else {
                                 debug("[jugador %d] decidiendo envite...\n", rank);
                                 MPI_Recv(envites_jugadores, 4, MPI_INT, 0, 0, parent, &stat);
                                 apuesta_en_vigor = maximo_array(envites_jugadores, N_JUGADORES);
                                 jugador_apuesta_en_vigor = busca_indice(envites_jugadores, N_JUGADORES,
                                                                         apuesta_en_vigor);
                                 //TODO: no subir envite a misma pareja?? no pasa nada...
                                 //TODO: LO QUE DIGA EL OTRO
                                 if ((l == 2) &&
                                     (tengoPares(pares) == 0)) { //si no tengo pares no envido. envite en paso.
                                     envites[0] = 1;
                                     envites[1] = 0;
                                     MPI_Send(envites, 2, MPI_INT, 0, 0, parent);
                                 } else {
                                     envido(envites, equivalencias_jugador, N_CARTAS_MANO, l, apuesta_en_vigor, rank,
                                            mano, pares, juego_al_punto, puntos_juego, n_puntos_juego);
                                     debug("[jugador %d] Generado envite: %d\n", rank, envites[0]);
                                     debug("[jugador %d] Generado envite_N: %d\n", rank, envites[1]);
                                     MPI_Send(envites, 2, MPI_INT, 0, 0, parent);
                                 }
                             }
                            break;
                        case 2: //esperar a ver qué dicen los otros
                            break;
                        case 3: // soy el jugador humano. Control por proceso maestro.
                            break;
                    }
                    MPI_Bcast(envites_jugadores, 4, MPI_INT, 0, parent);
                    i++;

                }
                /******************************************************************************************************
                 * SUBIDA DE APUESTAS
                 ******************************************************************************************************/
                token = 0;
                while (token != 2) {

                    MPI_Recv(&token, 1, MPI_INT, 0, 0, parent, &stat);

                    switch (token) {
                        case 1:
                            apuesta_en_vigor = maximo_array(envites_jugadores, N_JUGADORES);
                            jugador_apuesta_en_vigor = busca_indice(envites_jugadores, N_JUGADORES, apuesta_en_vigor);


                            if (((modo_juego == 1) && (rank == jugador_humano) && ((l == 0) || (l==1))) || //grande o chica
                                     ((modo_juego == 1) && (rank == jugador_humano) && (l == 2) && (indicador_pares==2) && (tengoPares(pares) == 1)) || //pares y tengo pares
                                     ((modo_juego == 1) && (rank == jugador_humano) && (l == 3) && (juego_al_punto == 2) && (tengoJuego(juego) == 1)) ||  //juego y tengo juego
                                     ((modo_juego == 1) && (rank == jugador_humano) && (l == 3) && (juego_al_punto == 1))) {  //enviar mazo para enseñarselo al jugador humano antes de envidar
                                debug("[jugador %d] Enviando cartas a maestro...\n", rank);
                                enviar_mazo(mano_cartas, 0, parent,
                                            N_CARTAS_MANO); // se envía la mano al maestro para E/S

                            }

                            else {
                                debug("[jugador %d] decidiendo envite...\n", rank);
                                MPI_Recv(envites_jugadores, 4, MPI_INT, 0, 0, parent, &stat);
                                apuesta_en_vigor = maximo_array(envites_jugadores, N_JUGADORES);
                                jugador_apuesta_en_vigor = busca_indice(envites_jugadores, N_JUGADORES,
                                                                        apuesta_en_vigor);

                                if ((l == 2) &&
                                    (tengoPares(pares) == 0)) { //si no tengo pares no envido. envite en paso.
                                    envites[0] = 1;
                                    envites[1] = 0;
                                    MPI_Send(envites, 2, MPI_INT, 0, 0, parent);
                                } else {
                                    envido(envites, equivalencias_jugador, N_CARTAS_MANO, l, apuesta_en_vigor, rank,
                                           mano, pares, juego_al_punto, puntos_juego, n_puntos_juego);
                                    debug("[jugador %d] Generado envite: %d\n", rank, envites[0]);
                                    debug("[jugador %d] Generado envite_N: %d\n", rank, envites[1]);
                                    MPI_Send(envites, 2, MPI_INT, 0, 0, parent);
                                }
                            }

                            break;
                        case 2:
                            break;

                        case 3:
                            MPI_Bcast(envites_jugadores, 4, MPI_INT, 0, parent);
                            break;

                    }


                }
            }

            /* Envío de datos al maestro para que evalúe*/

            /* Se repiten las cuentas para comparar jugadas porque las manos pueden haber cambiado */

            /**********************************************************************************************************
 * EVALUACIÓN DE MANOS
 **********************************************************************************************************/

            switch (l) {
                case 0:
                    // grande
                    for (i = N_CARTAS_PALO - 1; i >= 0; i--) {
                        cuenta = cuenta_cartas_mano(mano_cartas, i);

                        cuenta_cartas[N_CARTAS_PALO - i - 1] = cuenta;

                    }
                    MPI_Gather(cuenta_cartas, 10, MPI_INT, rbuf, 10, MPI_INT, 0, parent);
                    break;
                case 1:
                    // chica
                    for (i = N_CARTAS_PALO - 1; i >= 0; i--) {
                        cuenta = cuenta_cartas_mano(mano_cartas, i);

                        cuenta_cartas[N_CARTAS_PALO - i - 1] = cuenta;

                    }
                    invertirArray(cuenta_cartas, invertido, N_CARTAS_PALO);
                    MPI_Gather(invertido, 10, MPI_INT, rbuf, 10, MPI_INT, 0, parent);
                    break;
                case 2:
                    //pares


                    if (indicador_pares == 2) {
                        MPI_Gather(pares, 5, MPI_INT, rbuf, 5, MPI_INT, 0, parent);
                    }
                    break;
                case 3:
                    if (juego_al_punto != 0) {
                        juego = sumaArray(valores_jugador, N_CARTAS_MANO);
                        MPI_Gather(&juego, 1, MPI_INT, rbuf, 5, MPI_INT, 0, parent);
                    }
            }

            MPI_Bcast(&indicador_ordago, 1, MPI_INT, 0, parent);
            if (indicador_ordago == 1) {
                break; //salir de lances
            }
        }// end for iteración N_LANCES

        debug("[jugador %d] FINALIZADO", rank);
        MPI_Bcast(puntos_juego, 2, MPI_INT, 0, parent);
        MPI_Bcast(&fin_partida, 1, MPI_INT, 0, parent);
        if (indicador_ordago == 1) {
            indicador_ordago = 0;
        }
    }
    int token_end = 1;
    MPI_Send(&token_end, 1, MPI_INT, 0, 0, parent);
    //MPI_Comm_disconnect(&parent);
    MPI_Finalize();
    return 0;
}