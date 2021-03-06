//
// Created by predicador on 15/01/16.
//
/* C Example */



#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include "mus_feb.h"

#define N_CARTAS_MAZO 40
#define N_CARTAS_MANO 4
#define N_JUGADORES 4
#define N_PAREJAS 2
#define N_LANCES 4


int main(int argc, char **argv) {
    int rank, size, version, subversion, namelen, universe_size, jugadorMano, repartidor, sizeMazo, sizeDescartadas;
    char processor_name[MPI_MAX_PROCESSOR_NAME], worker_program[100];
    MPI_Comm juego_comm;
    Carta mazo[N_CARTAS_MAZO];
    Carta mano0[N_CARTAS_MANO];
    Carta mano1[N_CARTAS_MANO];
    Carta mano2[N_CARTAS_MANO];
    Carta mano3[N_CARTAS_MANO];
    Carta manoJugadorHumano[N_CARTAS_MANO];

    char *caras[] = {"As", "Dos", "Tres", "Cuatro", "Cinco",
                     "Seis", "Siete", "Sota", "Caballo", "Rey"};


    char *palos[] = {"Oros", "Copas", "Espadas", "Bastos"};
    char *lancesEtiquetas[] = {"Grande", "Chica", "Pares", "Juego", "Al punto"};
    int valores[] = {1, 1, 10, 4, 5, 6, 7, 10, 10, 10};
    int equivalencias[] = {1, 1, 10, 4, 5, 6, 7, 8, 9, 10};
    int piedras[N_PAREJAS] = {0, 0};
    int apuestas[N_LANCES + 1] = {0, 0, 0, 0, 0};
    int jugadorHumano = 99;
    int pareja1[6]; // miembro 1 de pareja - miembro 2 de pareja - piedras - rondas - juegos - vacas
    int pareja2[6];
    int ronda = 0;
    // inicialización de contadores
    pareja1[0] = 0;
    pareja1[1] = 2;
    pareja2[0] = 1;
    pareja2[1] = 3;
    int l = 0;
    for (l = 2; l < 6; l++) {
        pareja1[l] = 0;
        pareja2[l] = 0;
    }
    int N_PUNTOS_JUEGO = 40;
    int N_JUEGOS_VACA = 3;
    int N_VACAS_PARTIDA = 3;
    int N_PARTIDAS = 1;
    srand(time(NULL)); /* randomize */

    sizeMazo = crearMazo(mazo, caras, palos, valores, equivalencias); /* llena el mazo de cartas */
    sizeDescartadas = 0;
    int ordago = 0;
    char modo = 'Z'; //1 automático, 0 manual

    printf("Introduzca el número de partidas (1): \n");
    fflush(stdout);
    scanf(" %d", &N_PARTIDAS);
    getchar();
    fflush(stdout);
    fflush(stdin);

    printf("Introduzca el número de vacas (3): \n");
    fflush(stdout);
    scanf(" %d", &N_VACAS_PARTIDA);
    getchar();
    fflush(stdout);
    fflush(stdin);

    printf("Introduzca el número de juegos (3): \n");
    fflush(stdout);
    scanf(" %d", &N_VACAS_PARTIDA);
    getchar();
    fflush(stdout);
    fflush(stdin);

    printf("Introduzca el número de puntos por juego (40): \n");
    fflush(stdout);
    scanf(" %d", &N_PUNTOS_JUEGO);
    getchar();
    fflush(stdout);
    fflush(stdin);

    printf("Introduzca el modo de juego (A:automático, I:interactivo): \n");
    while (modo != 'A' || modo != 'I' || modo != 'a' || modo != 'i') {
        modo = getchar();

        if (modo == 'A' || modo == 'I' || modo == 'a' || modo == 'i') {
            break;
        }
        else {
            getchar();
            printf("Introduzca una A o una I\n");
        }


    }

    printf("Comenzando partida en modo %c\n", modo);
    if (modo == 'I' || modo == 'i') {
        jugadorHumano = rand() % (N_JUGADORES + 1 - 0) + 0;
        //jugadorHumano = 3;
        printf("El identificador para el jugador humano es: %d\n", jugadorHumano);

    }
    printf("[maestro] Tamaño del mazo"
                   " %d\n", sizeMazo);
    //printMazo(mazo); /*Imprime el mazo*/
    printf("\n");
    barajarMazo(mazo); /*Baraja el mazo*/
    printf("\n");
    MPI_Init(&argc, &argv);    /* starts MPI */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);    /* get current process id */
    MPI_Comm_size(MPI_COMM_WORLD, &size);    /* get number of processes */
    MPI_Get_processor_name(processor_name, &namelen);
    MPI_Get_version(&version, &subversion);
    printf("[maestro] Iniciado proceso maestro %d de %d en %s ejecutando MPI %d.%d\n", rank, size, processor_name,
           version,
           subversion);
    if (size != 1)
        printf("[maestro] Error: sólo debería estar ejecutándose el proceso maestro, pero hay %d procesos ejecutándose\n",
               size);

/* Fijar el total de procesos a ejecutar incluyendo el maestro */
    universe_size = 5;
    strcpy(worker_program, "./jugador");
    printf("[maestro] Generando %d procesos ejecutando %s\n", universe_size - 1, worker_program);
    MPI_Comm_spawn(worker_program, MPI_ARGV_NULL, universe_size - 1, MPI_INFO_NULL, 0, MPI_COMM_SELF, &juego_comm,
                   MPI_ERRCODES_IGNORE);
    printf("[maestro] Ejecutado proceso maestro con identificador %d de un total de %d\n", rank, size);

    /* PRIMER INTERCAMBIO DE INFORMACIÓN: maestro a jugadores */
    MPI_Bcast(&sizeMazo, 1, MPI_INT, MPI_ROOT, juego_comm);/*Envío del tamaño del mazo */
    MPI_Bcast(&sizeDescartadas, 1, MPI_INT, MPI_ROOT, juego_comm);/*Envío del tamaño del mazo de descartadas*/
    int corte; /* jugador que realizará el corte */
    int N = 0, M = N_JUGADORES - 1; /* valores del intervalo */
    corte = M + rand() / (RAND_MAX / (N - M + 1) + 1); /* proceso aleatorio de entre los existentes */
    MPI_Bcast(&corte, 1, MPI_INT, MPI_ROOT, juego_comm); /* envío del id de proceso que realizará el corte a todos*/
    MPI_Bcast(&modo, 1, MPI_CHAR, MPI_ROOT, juego_comm);
    MPI_Bcast(&jugadorHumano, 1, MPI_INT, MPI_ROOT,
              juego_comm); /* envío del id de jugador humano en caso de modo manual*/


    enviarMazo(mazo, corte, juego_comm, N_CARTAS_MAZO); /* envío del mazo al jugador que va a cortar la baraja*/
    MPI_Recv(&repartidor, 1, MPI_INT, corte, 0, juego_comm, MPI_STATUS_IGNORE);

    /**************************************************************/
    /* Comienzan rondas
    /**************************************************************/
    while ((pareja1[5] != 1 && pareja2[5] != 1)) { //mientras no haya una partida ganada...
        printf("INICIANDO RONDA %d\n", ronda);
        if (ronda != 0) {
            Carta mazo[N_CARTAS_MAZO];
            sizeMazo = crearMazo(mazo, caras, palos, valores, equivalencias); /* llena el mazo de cartas */
            printf("ATENCION Tamaño mazo: %d\n", sizeMazo);
            sizeDescartadas = 0;
            int ordago = 0;
            barajarMazo(mazo); /*Baraja el mazo*/
        }
        printf("[maestro] El jugador repartidor es: %d\n", repartidor);


        //int mano = add_mod(postre, 1, 4);
        //printf("[maestro] El jugador mano es: %d\n", mano);
        MPI_Bcast(&repartidor, 1, MPI_INT, MPI_ROOT, juego_comm); //envío del repartidor a todos los procesos
        MPI_Bcast(&sizeMazo, 1, MPI_INT, MPI_ROOT, juego_comm);
        /* envío del mazo al jugador que va a repartir */
        enviarMazo(mazo, repartidor, juego_comm, N_CARTAS_MAZO);

        /* e/s auxiliar reparto de cartas */
        int i = 0;
        for (i = 0; i <= (N_CARTAS_MANO * N_JUGADORES - 1); i++) {
            int buffer[3];
            MPI_Recv(&buffer, 3, MPI_INT, repartidor, 0, juego_comm, MPI_STATUS_IGNORE);
            printf("[repartidor %d] Repartida carta %d al jugador %d\n", repartidor, buffer[0], buffer[1]);
            int siguiente = buffer[1];
            MPI_Recv(&buffer, 3, MPI_INT, siguiente, 0, juego_comm, MPI_STATUS_IGNORE);
            printf("[jugador %d] Jugador %d recibe carta %d \n", buffer[0], buffer[0], buffer[1]);
        }

        MPI_Recv(&sizeMazo, 1, MPI_INT, repartidor, 0, juego_comm, MPI_STATUS_IGNORE);
        recibirMazo(mazo, repartidor, juego_comm, N_CARTAS_MAZO, MPI_STATUS_IGNORE);
        printf("[maestro] tamaño del mazo: %d\n", sizeMazo);


        MPI_Bcast(&sizeMazo, 1, MPI_INT, MPI_ROOT, juego_comm); //envío del tamaño del mazo a resto de procesos

        int siguienteJugador = add_mod(repartidor, 1, 4);


        jugadorMano = 99;
        int turno = 0;
        int turnoDescartes = 1;
        int bufferSnd[3] = {99, siguienteJugador, turno};
        int bufferRcv[3] = {99, siguienteJugador, turno};
        int descarte = 99;
        int contador = 0;

        // si jugamos con humano, hay que recibir su mano

        while (jugadorMano == 99) {


            if ((turno % 4) == 0 && (turno != 0) && (turnoDescartes == 1)) { //turno de descartes


                // recibe identificador de carta a descartar
                int descarteHumano = 99;
                for (i = 0; i < N_CARTAS_MANO; i++) {

                    if ((modo == 'I' || modo == 'i') && (siguienteJugador == jugadorHumano)) {
                        printf("¿Desea descartar %s de %s? (S/N)\n", manoJugadorHumano[i].cara,
                               manoJugadorHumano[i].palo);
                        char c;

                        scanf(" %c", &c);

                        if (c == 'S' || c == 's') {
                            descarteHumano = 1;
                        }
                        else {
                            descarteHumano = 0;
                        }

                        MPI_Send(&descarteHumano, 1, MPI_INT, jugadorHumano, 0, juego_comm);

                    }
                    MPI_Recv(&descarte, 1, MPI_INT, siguienteJugador, 0, juego_comm, MPI_STATUS_IGNORE);


                    if (descarte != 99 && descarte != 98) {
                        marcarDescarte(mazo, N_CARTAS_MAZO, descarte);
                        repartirCarta(mazo[N_CARTAS_MAZO - sizeMazo], siguienteJugador, juego_comm);
                        mazo[N_CARTAS_MAZO - sizeMazo].estado = 1;
                        sizeMazo--;
                    }

                }

                MPI_Recv(&bufferRcv[1], 1, MPI_INT, siguienteJugador, 0, juego_comm, MPI_STATUS_IGNORE);
                siguienteJugador = bufferRcv[1];
                contador++;
                if (contador == N_JUGADORES) {
                    turnoDescartes = 0;
                    contador = 0;
                }
                MPI_Bcast(&turnoDescartes, 1, MPI_INT, MPI_ROOT, juego_comm);

            } else { // mus corrido
                turnoDescartes = 1;
                MPI_Bcast(&turnoDescartes, 1, MPI_INT, MPI_ROOT, juego_comm);
                int mus = 99;


                if ((modo == 'I' || modo == 'i') && siguienteJugador == jugadorHumano) {
                    char c = 'Z';
                    printf("[maestro] Mano actual del jugador %d\n", jugadorHumano);
                    recibirMazo(manoJugadorHumano, jugadorHumano, juego_comm, N_CARTAS_MANO, MPI_STATUS_IGNORE);
                    printMazo(manoJugadorHumano, N_CARTAS_MANO);

                    printf("¿Hay mus? (S:mus, N:no mus):\n");
                    fflush(stdout);
                    scanf(" %c", &c);
                    getchar();
                    if (c == 'S' || c == 's') {
                        mus = 0;
                    }
                    else {
                        mus = 1;
                    }
                    fflush(stdout);
                    fflush(stdin);
                    MPI_Send(&mus, 1, MPI_INT, jugadorHumano, 0, juego_comm);
                };

                MPI_Recv(&bufferRcv[0], 1, MPI_INT, siguienteJugador, 0, juego_comm, MPI_STATUS_IGNORE);
                MPI_Recv(&bufferRcv[1], 1, MPI_INT, siguienteJugador, 0, juego_comm, MPI_STATUS_IGNORE);
                MPI_Recv(&bufferRcv[2], 1, MPI_INT, siguienteJugador, 0, juego_comm, MPI_STATUS_IGNORE);
                jugadorMano = bufferRcv[0];
                if (jugadorMano != 99) {
                    printf("[maestro] Mus cortado por jugador: %d\n", jugadorMano);
                }


                turno++;
                if (jugadorMano == 99 || jugadorMano == 0 || jugadorMano == 1 || jugadorMano == 2 || jugadorMano == 3) {
                    bufferSnd[0] = jugadorMano;
                }
                else {
                    jugadorMano = 99;
                }
                if (bufferRcv[1] == 0 || bufferRcv[1] == 1 || bufferRcv[1] == 2 || bufferRcv[1] == 3) {
                    siguienteJugador = bufferRcv[1];
                    bufferSnd[1] = siguienteJugador;
                }
                else {
                    siguienteJugador = add_mod(siguienteJugador, 1, 4);
                    bufferSnd[1] = siguienteJugador;
                }
                if (jugadorMano != 99) {
                    siguienteJugador = jugadorMano;
                    bufferSnd[1] = jugadorMano;
                }

                bufferSnd[2] = turno;

                MPI_Bcast(&bufferSnd, 3, MPI_INT, MPI_ROOT, juego_comm);
            }

            MPI_Bcast(&siguienteJugador, 1, MPI_INT, MPI_ROOT, juego_comm);
        }
        printf("[maestro] La mano es: %d\n", jugadorMano);

        int conteos[10];
        int paresBuf[25];
        int juegoBuf[5];
        for (i = 0; i < 10; i++) {
            conteos[i] = 0;
        }
        int rbuf[50];
        int rbufInv[50];
        int envite[2];
        int enviteAnterior[2];
        envite[0] = 0;
        envite[1] = 0;
        enviteAnterior[0] = 0;
        enviteAnterior[1] = 0;
        int envites[10];

        int enviteContraria[2];
        enviteContraria[0] = 99;
        enviteContraria[1] = 0;

        int lances[N_LANCES];
        int tienenPares[N_JUGADORES + 1];
        int tienenJuego[N_JUGADORES + 1];
        int hayPares = 0;
        int hayJuego = 0;
        //int pareja; //1 es pareja mano, 0 es pareja postre
        int j = 0;

        for (j = 0; j < N_LANCES + 1; j++) {


            /* envites */

            //automático

            if (j == 2) { // ver si hay pares
                // recupera pares de los jugadores
                MPI_Gather(conteos, 1, MPI_INT, tienenPares, 1, MPI_INT, MPI_ROOT, juego_comm);
                // comprueba que alguno es distinto de cero
                for (i = 0; i < N_JUGADORES; i++) {
                    if (tienenPares[i] != 0) {
                        hayPares = 1;
                    }
                }
                printf("HAY PARES: %d\n", hayPares);
            }

            if (j == 3) { // ver si hay juego
                // recupera pares de los jugadores
                MPI_Gather(conteos, 1, MPI_INT, tienenJuego, 1, MPI_INT, MPI_ROOT, juego_comm);
                // comprueba que alguno es distinto de cero
                for (i = 0; i < N_JUGADORES; i++) {
                    if (tienenJuego[i] != 0) {
                        hayJuego = 1;
                    }
                }
                printf("HAY JUEGO: %d\n", hayJuego);
                MPI_Bcast(&hayJuego, 1, MPI_INT, MPI_ROOT, juego_comm);
            }

            // recibir envite de la mano

            if ((j == 0) || (j == 1) || ((j == 2) && (hayPares == 1)) || ((j == 3) && (hayJuego == 1)) ||
                ((j == 4) && (hayJuego == 0))) {
                printf("[maestro] Se juega este lance\n");
                if ((modo == 'I' || modo == 'i') && (siguienteJugador == jugadorHumano) &&
                    (jugadorHumano == jugadorMano)) {

                    int e = 0;

                    printf("[maestro] Mano actual del jugador %d\n", jugadorHumano);
                    recibirMazo(manoJugadorHumano, jugadorHumano, juego_comm, N_CARTAS_MANO, MPI_STATUS_IGNORE);
                    printMazo(manoJugadorHumano, N_CARTAS_MANO);

                    printf("Introduzca envite a %s: (0:no, 2: sí, >2: más)\n", lancesEtiquetas[j]);
                    fflush(stdout);
                    scanf(" %d", &e);
                    getchar();
                    fflush(stdout);
                    fflush(stdin);
                    MPI_Send(&e, 1, MPI_INT, jugadorHumano, 0, juego_comm);

                }
            }

            MPI_Recv(&envite, 2, MPI_INT, jugadorMano, 0, juego_comm, MPI_STATUS_IGNORE);
            printf("[maestro]: Lance %d\n", j);
            enviteAnterior[0] = envite[0];
            enviteAnterior[1] = envite[1];
            printf("[maestro]: Envite de la mano: %d\n", envite[0]);

            // enviar envite a todos los jugadores
            MPI_Bcast(&envite, 2, MPI_INT, MPI_ROOT, juego_comm);

            if ((modo == 'I' || modo == 'i') && jugadorHumano != jugadorMano) {

                int e = 0;
                int humanoTienePares = 0;
                MPI_Recv(&humanoTienePares, 1, MPI_INT, jugadorHumano, 0, juego_comm, MPI_STATUS_IGNORE);
                if (humanoTienePares != 0) {
                    printf("[maestro] Mano actual del jugador %d\n", jugadorHumano);
                    recibirMazo(manoJugadorHumano, jugadorHumano, juego_comm, N_CARTAS_MANO, MPI_STATUS_IGNORE);
                    printMazo(manoJugadorHumano, N_CARTAS_MANO);

                    printf("Introduzca envite a %s: (0:no, 2: sí, >2: más)\n", lancesEtiquetas[j]);
                    fflush(stdout);
                    scanf(" %d", &e);
                    getchar();
                    fflush(stdout);
                    fflush(stdin);
                    MPI_Send(&e, 1, MPI_INT, jugadorHumano, 0, juego_comm);
                }
            }
            //rondas de envites hasta que se igualen o no se acepten
            // se garantiza que no va a haber repeticiones porque se van a rajar
            // recibir respuesta de todos los jugadores: de la pareja contraria, prima la más alta
            MPI_Gather(conteos, 10, MPI_INT, envites, 2, MPI_INT, MPI_ROOT, juego_comm);
            printf("[maestro] recibe envites\n");

            if (ocurrenciasArray(envites, 8, 99) == 1) {
                ordago = 1;
                printf("¡¡¡ÓRDAGO!!!\n");
                MPI_Bcast(&ordago, 1, MPI_INT, MPI_ROOT, juego_comm);
                break;
            }//ordago
            MPI_Bcast(&ordago, 1, MPI_INT, MPI_ROOT, juego_comm);
            apuestas[j] = calcularEnvite(envites, enviteAnterior, jugadorMano, piedras);
            printf("PIEDRAS MANO: %d\n", piedras[1]);
            printf("PIEDRAS POSTRE: %d\n", piedras[0]);

            // enviar respuesta de pareja contraria a todos los jugadores
            MPI_Bcast(&enviteContraria, 2, MPI_INT, MPI_ROOT, juego_comm);

            if (j == 3 && hayJuego == 1) {
                break; //no se juega al punto
            }
        }

        // }
        // almacenar envites

        /* Recepción de datos para evaluar las manos de los jugadores */
        MPI_Gather(conteos, 10, MPI_INT, rbuf, 10, MPI_INT, MPI_ROOT, juego_comm);
        MPI_Gather(conteos, 10, MPI_INT, rbufInv, 10, MPI_INT, MPI_ROOT, juego_comm);
        MPI_Gather(conteos, 5, MPI_INT, paresBuf, 5, MPI_INT, MPI_ROOT, juego_comm);
        MPI_Gather(conteos, 1, MPI_INT, juegoBuf, 1, MPI_INT, MPI_ROOT, juego_comm);


        /*cálculo de manos*/
        lances[0] = calculaGrande(rbuf, jugadorMano);
        lances[1] = calculaChica(rbufInv);
        lances[2] = calcularPares(paresBuf, jugadorMano);
        lances[3] = calcularJuego(juegoBuf, jugadorMano);
        printf("Mejor mano a grande: jugador %d\n", lances[0]);
        printf("Mejor mano a chica: jugador %d\n", lances[1]);
        printf("Mejor mano a pares: jugador %d\n", lances[2]);
        printf("Mejor mano a juego: jugador %d\n", lances[3]);

        if (ordago == 1) {
            printf("Ganador del lance %s y juego: jugador %d\n ", lancesEtiquetas[j], lances[j]);
        }
/*
        for (i = 0; i < N_JUGADORES; i++) {

            if (paresBuf[5 * i] != 99) {
                printf("[jugador %d] Duples de la misma carta: %s\n", i, caras[paresBuf[5 * i]]);
            }
            else if (paresBuf[5 * i + 2] == 2) {
                printf("[jugador %d] DUPLES PAREJAS DE %s Y %s\n", i, caras[paresBuf[5 * i + 3]],
                       caras[paresBuf[5 * i + 4]]);
            }
            else if (paresBuf[5 * i + 1] != 99) {
                printf("[jugador %d] MEDIAS DE: %s\n", i, caras[paresBuf[5 * i + 1]]);
            }
            else if (paresBuf[5 * i + 2] == 1) {
                printf("[jugador %d] PAREJA DE %s\n", i, caras[paresBuf[5 * i + 3]]);
            }
        }
*/

        for (i = 0; i < N_JUGADORES; i++) {
            printf("JUEGO DE JUGADOR %d: %d\n", i, juegoBuf[i]);
        }

        for (i = 0; i <= N_LANCES; i++) {
            printf("APUESTA LANCE %d: %d\n", i, apuestas[i]);
            if (apuestas[i] != 0) {
                piedras[queParejaSoy(lances[i], jugadorMano)] += apuestas[i];
            }

        }


        printf("PIEDRAS MANO: %d\n", piedras[1]);
        printf("PIEDRAS POSTRE: %d\n", piedras[0]);
        if (enQueParejaEstoy(jugadorMano) == 1) {
            pareja1[2] = piedras[1];
            pareja2[2] = piedras[0];
        }
        else {
            pareja1[2] = piedras[0];
            pareja2[2] = piedras[1];
        }
        pareja1[3] = pareja1[2] / N_PUNTOS_JUEGO;
        pareja2[3] = pareja2[2] / N_PUNTOS_JUEGO;

        pareja1[4] = pareja1[3] / N_JUEGOS_VACA;
        pareja2[4] = pareja2[3] / N_JUEGOS_VACA;

        pareja1[5] = pareja1[4] / N_VACAS_PARTIDA;
        pareja2[5] = pareja2[4] / N_VACAS_PARTIDA;


        recibirMazo(mano0, 0, juego_comm, N_CARTAS_MANO, MPI_STATUS_IGNORE);
        recibirMazo(mano1, 1, juego_comm, N_CARTAS_MANO, MPI_STATUS_IGNORE);
        recibirMazo(mano2, 2, juego_comm, N_CARTAS_MANO, MPI_STATUS_IGNORE);
        recibirMazo(mano3, 3, juego_comm, N_CARTAS_MANO, MPI_STATUS_IGNORE);

        printf("MANO DEL JUGADOR 0:\n");
        printMazo(mano0, N_CARTAS_MANO);
        printf("MANO DEL JUGADOR 1:\n");
        printMazo(mano1, N_CARTAS_MANO);
        printf("MANO DEL JUGADOR 2:\n");
        printMazo(mano2, N_CARTAS_MANO);
        printf("MANO DEL JUGADOR 3:\n");
        printMazo(mano3, N_CARTAS_MANO);

        printf("PUNTUACIONES:\n");
        printf("PAREJA 1\n");
        printf("RONDA: %d\n", pareja1[2]);
        printf("JUEGO: %d\n", pareja1[3]);
        printf("VACA: %d\n", pareja1[4]);
        printf("PARTIDA: %d\n", pareja1[5]);

        printf("PAREJA 2\n");
        printf("RONDA: %d\n", pareja2[2]);
        printf("JUEGO: %d\n", pareja2[3]);
        printf("VACA: %d\n", pareja2[4]);
        printf("PARTIDA: %d\n", pareja2[5]);

        /**************************************************************/
        /* Terminan rondas
        /**************************************************************/
        repartidor = jugadorMano;
        ronda++;
        if ((pareja1[5] == 1) || pareja2[5] == 1) {
            // fin de partida
            int finPartida = 1;
            MPI_Bcast(&finPartida, 1, MPI_INT, MPI_ROOT, juego_comm);
        }
        else {
            //sigue partida
            int finPartida = 0;
            MPI_Bcast(&finPartida, 1, MPI_INT, MPI_ROOT, juego_comm);
        }
    }
    MPI_Comm_disconnect(&juego_comm);
    MPI_Finalize();
    return 0;
}


