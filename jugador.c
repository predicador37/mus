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
#define CHAR_BUFFER 8

extern const char * caras[];
extern const char * palos[];
extern const char * lancesEtiquetas[];
extern int valores[];
extern int equivalencias[];

int main(int argc, char **argv) {
    /*
     * DECLARACIÓN DE VARIABLES
     */
    int rank, size, namelen, version, subversion, psize, modo_juego, token, corte, palo_corte, repartidor, postre, mano, jugador_humano, size_mazo, size_mano, size_descartadas, siguiente_jugador, mus, descarte, n_cartas_a_descartar, apuesta_en_vigor, jugador_apuesta_en_vigor, envite;

    char processor_name[MPI_MAX_PROCESSOR_NAME], worker_program[100];
    int cuenta_cartas[N_CARTAS_PALO], equivalencias_jugador[N_CARTAS_MANO];
    //Array de 4 posiciones para los envites, una para cada jugador
    //0: no ha hablado
    //1: paso
    //2: envido (2 piedras, apuesta mínima)
    //3-99: envido N piedras
    int envites_jugadores[N_JUGADORES] = {0,0,0,0};
    int envites[2]={0,0};
    int cartas_a_descartar[N_CARTAS_MANO] = {99,99,99,99};
    int  rbuf[50]; //buffer de recepcion para evaluar jugadas
    Carta mazo[N_CARTAS_MAZO];
    Carta mano_cartas[N_CARTAS_MANO];
    //char * palo_corte = NULL;
    MPI_Comm juego_comm;
    MPI_Status stat;
    MPI_Comm parent;

    srand(time(NULL)); /* randomize */

    /*
     * INICIALIZACIÓN DE MPI
     */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
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
    //inicializar_mazo(mazo, N_CARTAS_MAZO);
    //inicializar_mazo(mano_cartas, N_CARTAS_MANO);
    MPI_Recv(&token, 1, MPI_INT, 0, 0, parent, &stat);
    MPI_Send(&rank, 1, MPI_INT, 0, 0, parent);
    //MPI_Barrier(MPI_COMM_WORLD);

    /*
     * DETERMINACIÓN DE PROCESO REPARTIDOR PARA MUS CORRIDO
     */

  MPI_Bcast(&corte, 1, MPI_INT, 0, parent);
  MPI_Bcast(&size_mazo, 1, MPI_INT, 0, parent);
    MPI_Bcast(&modo_juego, 1, MPI_INT, 0, parent);
  MPI_Bcast(&jugador_humano, 1, MPI_INT, 0, parent);

    if (rank == corte) {

        recibir_mazo(mazo, 0, parent, N_CARTAS_MAZO, &stat);
        debug("Mazo recibido");
        int r; // índice aleatorio para el mazo
        //r = rand() % (N_CARTAS_MAZO + 1 - 0) + 0;
        r = rand_lim(N_CARTAS_MAZO-1);
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
   // MPI_Barrier(MPI_COMM_WORLD);
    /*
     * REPARTO DE CARTAS PARA MUS CORRIDO
     */
    if (rank == repartidor) {
        size_mazo = repartidor_reparte(rank, repartidor, size_mazo, size_descartadas, mazo, mano_cartas, parent, stat);
        // El proceso maestro debe contar con el mazo actualizado
        MPI_Send(&size_mazo, 1, MPI_INT, 0, 0, parent);
        enviar_mazo(mazo, 0, parent, N_CARTAS_MAZO); // se devuelve el mazo al maestro
    }
    else { // el proceso no es repartidor; es decir, un jugador estándar
        jugador_recibe_cartas(rank, repartidor, mano_cartas, parent, &stat);


    }//se termina el reparto
    //MPI_Barrier(MPI_COMM_WORLD);
    MPI_Bcast(&size_mazo, 1, MPI_INT, 0, parent); //recepción del tamaño después de repartir


    /* EVALUACION PREVIA DE CARTAS */

    int i = 0;
    int juego = 0;
    int invertido[N_CARTAS_PALO];
    int pares[5];



    for (i = 0; i < N_CARTAS_MANO; i++) {
        equivalencias_jugador[i] = equivalencias[mano_cartas[i].cara];
    }

    int valores_jugador[N_CARTAS_MANO];
    for (i = 0; i < N_CARTAS_MANO; i++) {
        valores_jugador[i] = valores[mano_cartas[i].cara];
    }

    //Grande
    int cuenta = 0;

    for (i = N_CARTAS_PALO - 1; i >= 0; i--) {
        cuenta = cuenta_cartas_mano(mano_cartas,i);

        cuenta_cartas[N_CARTAS_PALO - i - 1] = cuenta;

    }

    // chica

    invertirArray(cuenta_cartas, invertido, N_CARTAS_PALO);

    // pares

    preparaPares(equivalencias_jugador, pares);

    // juego

    juego = sumaArray(valores_jugador, N_CARTAS_MANO);


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

                if ((modo_juego==1) && (jugador_humano==rank)){//jugador humano
                    enviar_mazo(mano_cartas, 0, parent, N_CARTAS_MANO); // se envía la mano al maestro para E/S
                    MPI_Recv(&n_cartas_a_descartar, 1, MPI_INT, 0, 0, parent, &stat);
                    MPI_Recv(cartas_a_descartar, n_cartas_a_descartar, MPI_INT, 0, 0, parent, &stat);
                }
                 else { //modo automático
                    // identificar cuántas cartas se van a descartar
                    // identificar qué cartas se van a descartar
                    n_cartas_a_descartar = 0;
                    for (i = 0; i < N_CARTAS_MANO; i++) {
                        cartas_a_descartar[i]=99;
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
                    MPI_Send(cartas_a_descartar, n_cartas_a_descartar, MPI_INT, 0, 0, parent); //envío de descartes

                }

                /**************** INICIO BLOQUE BLOQUEO***************/

                    //recibir cartas nuevas
                int k;
                for ( i = 0; i < N_CARTAS_MANO; i++) {

                    for (k = 0; k < n_cartas_a_descartar; k++) {
                        if (mano_cartas[i].id == cartas_a_descartar[k]) {
                            printf("[jugador %d] Descartando carta con id %d y k = %d\n", rank, cartas_a_descartar[k], k);
                            mano_cartas[i] = recibir_carta(0, parent, &stat);
                            valores_jugador[i] = valores[mano_cartas[i].cara];
                            equivalencias_jugador[i] = equivalencias[mano_cartas[i].cara];
                            break;
                        }
                    }
                }

                /**************** FIN BLOQUE BLOQUEO***************/

                printf("[jugador %d] IMPORTANTE: LLEGO AQUÏ\n", rank);
                break;
            case 4: //jugador reparte descartes
                debug("Jugador: %d reparte descartes", rank);
                MPI_Recv(&size_mazo, 1, MPI_INT, 0, 0, parent, &stat);
                recibir_mazo(mazo, 0, parent, N_CARTAS_MAZO, &stat);

                i=0;
                int j;
                while(i<4) {

                    if (i==3) { //soy el repartidor
                        if ((modo_juego == 1) && (jugador_humano == rank)) {//jugador humano
                            enviar_mazo(mano_cartas, 0, parent, N_CARTAS_MANO); // se envía la mano al maestro para E/S
                            MPI_Recv(&n_cartas_a_descartar, 1, MPI_INT, 0, 0, parent, &stat);
                            MPI_Recv(cartas_a_descartar, n_cartas_a_descartar, MPI_INT, 0, 0, parent, &stat);
                        } else {
                            printf("[jugador %d] SOY EL REPARTIDOR\n", rank);
                            n_cartas_a_descartar = 0;
                            for (j = 0; j < N_CARTAS_MANO; j++) {
                                cartas_a_descartar[i]=99;
                                if (equivalencias[mano_cartas[j].cara] != 10) { //Descartamos todo lo que no sea un rey
                                    debug("jugador %d hace descarte", rank);
                                    cartas_a_descartar[n_cartas_a_descartar] = mano_cartas[j].id;
                                    n_cartas_a_descartar++;

                                }

                            }
                            printf("REPARTIDOR DESCARTA %d CARTAS \n", n_cartas_a_descartar);
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


                    printf("CARTAS EN MANOS: %d\n", contar_cartas_en_estado(mazo,1));

                for (j = 0; j < n_cartas_a_descartar; j++) {
                    marcar_descarte(mazo, N_CARTAS_MAZO, cartas_a_descartar[j]); //primero se tiran las cartas

                }
                   // printf("[jugador %d] CARTAS EN MANOS: %d\n", rank, contar_cartas_en_estado(mazo, 1));
                   // printf("[jugador %d] CARTAS EN MAZO: %d\n", rank, contar_cartas_en_estado(mazo, 0));
                   // printf("[jugador %d] CARTAS DESCARTADAS: %d\n", rank, contar_cartas_en_estado(mazo, 2));
                    /**************** INICIO BLOQUE BLOQUEO***************/

                    for (j = 0; j < n_cartas_a_descartar; j++) { //luego se reparten N

                //y si el mazo se queda sin cartas....

                        while (mazo[N_CARTAS_MAZO - size_mazo].estado != 0) {
                            printf("CARTA YA REPARTIDA; PASANDO con size_mazo %d e indice de array %d...\n", size_mazo, N_CARTAS_MAZO - size_mazo);
                            size_mazo--; //ojo al repartir con el mazo virtual; las que no están en estado 0 no están en el mazo

                            if (size_mazo == 0){
                                printf("[285] ATENCIÓN: MAZO SIN CARTAS. VOLVER A MEZCLAR!!!\n");
                                int vuelta_al_mazo = poner_descartadas_en_mazo(mazo);
                                printf("[jugador %d] DEVUELTAS AL MAZO: %d\n", rank, vuelta_al_mazo);
                                //barajar_mazo(mazo); //Baraja el mazo
                                print_mazo(mazo, N_CARTAS_MAZO);
                                size_mazo = N_CARTAS_MAZO; // Reestablece el contador para recorrer cartas (representa carta arriba)
                                //enviar_mazo(mazo, 0, parent, N_CARTAS_MAZO); // se devuelve el mazo al maestro
                            }
                        }
                    if (mazo[N_CARTAS_MAZO - size_mazo].estado == 0) {

                        mazo[N_CARTAS_MAZO - size_mazo].estado = 1; //TODO: BUCLE INFINITO; TODAS LAS CARTAS SE PONEN A ESTADO 1
                        repartir_carta(mazo[N_CARTAS_MAZO - size_mazo], 0, parent);
                        printf("[jugador %d] Repartiendo carta con size_mazo %d\n", rank, size_mazo);
                        size_mazo--;
                        printf("Cursor size_mazo: %d\n", size_mazo);
                     
                        printf("[jugador %d] CARTAS EN MANOS: %d\n", rank, contar_cartas_en_estado(mazo, 1));
                        printf("[jugador %d] CARTAS EN MAZO: %d\n", rank, contar_cartas_en_estado(mazo, 0));
                        printf("[jugador %d] CARTAS DESCARTADAS: %d\n", rank, contar_cartas_en_estado(mazo, 2));

                        if (size_mazo == 0){
                            printf("[301] ATENCIÓN: MAZO SIN CARTAS. VOLVER A MEZCLAR!!!\n");
                            debug("[302] ATENCIÓN: MAZO SIN CARTAS. VOLVER A MEZCLAR!!!\n");
                            int vuelta_al_mazo =  poner_descartadas_en_mazo(mazo);
                            printf("[jugador %d] DEVUELTAS AL MAZO: %d\n", rank, vuelta_al_mazo);
                            //barajar_mazo(mazo); //Baraja el mazo
                            print_mazo(mazo, N_CARTAS_MAZO);
                            size_mazo = N_CARTAS_MAZO; // Reestablece el contador para recorrer cartas (representa carta arriba)
                            //enviar_mazo(mazo, 0, parent, N_CARTAS_MAZO); // se devuelve el mazo al maestro
                        }
                    }else {
                        printf("IMPOSSIBLE SITUATION\n");
                    }

                        if (i==3) { // repartidor se reparte a sí mismo
                            printf("[jugador %d] REPARTIDOR SE REPARTE A SI MISMO\n");
                             int k;
                               for (k = 0; k < N_CARTAS_MANO; k++) {
                                    if (mano_cartas[k].id == cartas_a_descartar[j]) {
                                        printf("Carta a reemplazar con id %d\n", cartas_a_descartar[j]);
                                        printf("[jugador %d] Descartando carta con id %d y j = %d\n", rank, cartas_a_descartar[j], j);
                                        mano_cartas[k] = recibir_carta(0, parent, &stat);
                                        valores_jugador[k] = valores[mano_cartas[j].cara];
                                        equivalencias_jugador[k] = equivalencias[mano_cartas[j].cara];
                                        break;
                                    }
                                }
                            }



                }

                    /**************** FIN BLOQUE BLOQUEO***************/
                    i++;
                }
                printf("[jugador %d] Final de caso repartidor\n", rank);
                MPI_Send(&size_mazo, 1, MPI_INT, 0, 0, parent); //envío de tamaño del mazo
                enviar_mazo(mazo, 0, parent, N_CARTAS_MAZO); // se devuelve el mazo al maestro
                break;

        }
      //  if (mus == 1) {
       //     break;
        //}
        } //fin while mus==0

//Se recibe del maestro quien es el jugador mano
    MPI_Bcast(&mano, 1, MPI_INT, 0, parent);
    enviar_mazo(mano_cartas, 0, parent, N_CARTAS_MANO); // se envía la mano al maestro para E/S
    /*
   * FASE DE LANCES
   */

    //Array de 4 posiciones para los envites, una para cada jugador
    //0: no ha hablado
    //1: paso
    //2: envido (2 piedras, apuesta mínima)
    //3-99: envido N piedras

    //lance termina si:
        // 4 jugadores están en paso
        // 3 jugadores están en paso y 1 en 2-99
        // mayor apuesta de pareja 1 y mayor apuesta de pareja 2 son iguales (apuesta igualada)
    i = 0;
    while(i<N_JUGADORES) { // 4 turnos
        debug("jugador %d esperando token...", rank);
        token = 0;
        MPI_Recv(&token, 1, MPI_INT, 0, 0, parent, &stat);
        debug("Token=%d recibido por jugador %d", token, rank);

        switch (token) {
            case 1: //decidir envite
                if ((modo_juego==1) && (rank!=jugador_humano) || (modo_juego == 0)) {
                    debug("[jugador %d] decidiendo envite...\n", rank);
                    MPI_Recv(envites_jugadores, 4, MPI_INT, 0, 0, parent, &stat);
                    apuesta_en_vigor = maximo_array(envites_jugadores, N_JUGADORES);
                    jugador_apuesta_en_vigor = busca_indice(envites_jugadores, N_JUGADORES, apuesta_en_vigor);
                    //TODO: no subir envite a misma pareja
                    envido(envites, equivalencias_jugador, N_CARTAS_MANO, 0, apuesta_en_vigor, rank, mano);
                    printf("[jugador %d] Generado envite: %d\n", envites[0]);
                    printf("[jugador %d] Generado envite_N: %d\n", envites[1]);
                    MPI_Send(envites, 2, MPI_INT, 0, 0, parent);
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

    //MPI_Barrier(parent);
    token = 0;
    while (token != 2) {

        MPI_Recv(&token, 1, MPI_INT, 0, 0, parent, &stat);

        switch (token) {
            case 1:
                apuesta_en_vigor = maximo_array(envites_jugadores, N_JUGADORES);
                jugador_apuesta_en_vigor = busca_indice(envites_jugadores, N_JUGADORES, apuesta_en_vigor);
                envido(envites, equivalencias_jugador, N_CARTAS_MANO, 0, apuesta_en_vigor, rank, mano);
                MPI_Send(envites, 2, MPI_INT, 0, 0, parent);
                break;
            case 2:
                break;

            case 3:
                MPI_Bcast(envites_jugadores, 4, MPI_INT, 0, parent);
                break;

        }


    }

    /* Envío de datos al maestro para que evalúe*/

    MPI_Gather(cuenta_cartas, 10, MPI_INT, rbuf, 10, MPI_INT, 0, parent);

    //debug("[jugador %d] FINALIZADO", rank);


    //MPI_Comm_disconnect(&parent);

    //MPI_Barrier(parent);

    MPI_Finalize();
    return 0;
}