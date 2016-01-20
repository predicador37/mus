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
#include "mus.h"

#define N_CARTAS_MAZO 40
#define N_CARTAS_MANO 4
#define N_JUGADORES 4
#define N_LANCES 4


int main(int argc, char **argv) {
    int rank, size, version, subversion, namelen, universe_size, postre, jugadorMano, repartidor, sizeMazo, sizeDescartadas;
    char processor_name[MPI_MAX_PROCESSOR_NAME], worker_program[100];
    MPI_Comm juego_comm;
    Carta mazo[N_CARTAS_MAZO];


    char *caras[] = {"As", "Dos", "Tres", "Cuatro", "Cinco",
                     "Seis", "Siete", "Sota", "Caballo", "Rey"};


    char *palos[] = {"Oros", "Copas", "Espadas", "Bastos"};
    int valores[] = {1, 1, 10, 4, 5, 6, 7, 10, 10, 10};
    int equivalencias[] = {1, 1, 10, 4, 5, 6, 7, 8, 9, 10};

    srand(time(NULL)); /* randomize */

    sizeMazo = crearMazo(mazo, caras, palos, valores, equivalencias); /* llena el mazo de cartas */
    sizeDescartadas = 0;
    printf("[maestro] Tamaño del mazo: %d\n", sizeMazo);
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
    MPI_Bcast(&sizeMazo, 1, MPI_INT, MPI_ROOT, juego_comm);/*Envío del tamaño del mazo */
    MPI_Bcast(&sizeDescartadas, 1, MPI_INT, MPI_ROOT, juego_comm);/*Envío del tamaño del mazo de descartadas*/
    int corte; /* jugador que realizará el corte */
    int N = 0, M = N_JUGADORES - 1; /* valores del intervalo */
    corte = M + rand() / (RAND_MAX / (N - M + 1) + 1); /* proceso aleatorio de entre los existentes */
    MPI_Bcast(&corte, 1, MPI_INT, MPI_ROOT, juego_comm); /* envío del id de proceso que realizará el corte a todos*/

    /* envío del mazo al jugador que va a cortar la baraja*/

    enviarMazo(mazo, corte, juego_comm);
    MPI_Recv(&repartidor, 1, MPI_INT, corte, 0, juego_comm, MPI_STATUS_IGNORE);
    printf("[maestro] El jugador repartidor es: %d\n", repartidor);


    //int mano = add_mod(postre, 1, 4);
    //printf("[maestro] El jugador mano es: %d\n", mano);
    MPI_Bcast(&repartidor, 1, MPI_INT, MPI_ROOT, juego_comm);

    /* envío del mazo al jugador que va a repartir */
    enviarMazo(mazo, repartidor, juego_comm);

    /* e/s auxiliar reparto de cartas */
    int i = 0;
    for (i = 0; i <= (N_CARTAS_MANO * N_JUGADORES - 1); i++) {
        int buffer[3];
        MPI_Recv(&buffer, 3, MPI_INT, repartidor, 0, juego_comm, MPI_STATUS_IGNORE);
        printf("[repartidor %d] Repartida carta %d al jugador %d\n", repartidor, buffer[0], buffer[1]);
        int siguiente = buffer[1];
        MPI_Recv(&buffer, 3, MPI_INT, siguiente, 0, juego_comm, MPI_STATUS_IGNORE);
        printf("[jugador %d] Jugador %d recibe carta %d con valor %d\n", buffer[0], buffer[0], buffer[1], buffer[2]);
    }

    MPI_Recv(&sizeMazo, 1, MPI_INT, repartidor, 0, juego_comm, MPI_STATUS_IGNORE);
    recibirMazo(mazo, repartidor, juego_comm, MPI_STATUS_IGNORE);
    printf("[maestro] tamaño del mazo: %d\n", sizeMazo);
    MPI_Recv(&jugadorMano, 1, MPI_INT, corte, 0, juego_comm, MPI_STATUS_IGNORE);



    int counts[5] = {10, 10, 10, 10, 0};
    int displs[5] = {0, 10, 20, 30, 40};
    int conteos[10];
    int paresBuf[25];
    int juegoBuf[5];
    for (i = 0; i < 10; i++) {
        conteos[i] = 0;
    }
    int rbuf[50];
    int rbufInv[50];
    int lances[N_LANCES];

    /* Recepción de datos para evaluar las manos de los jugadores */
    MPI_Gather(conteos, 10, MPI_INT, rbuf, 10, MPI_INT, MPI_ROOT, juego_comm);
    MPI_Gather(conteos, 10, MPI_INT, rbufInv, 10, MPI_INT, MPI_ROOT, juego_comm);
    MPI_Gather(conteos, 5, MPI_INT, paresBuf, 5, MPI_INT, MPI_ROOT, juego_comm);
    MPI_Gather(conteos, 1, MPI_INT, juegoBuf, 1, MPI_INT, MPI_ROOT, juego_comm);


    int k = 0;


    /*cálculo de manos*/
    lances[0] = calculaGrande(rbuf);
    lances[1] = calculaChica(rbufInv);
    lances[2] = calcularPares(paresBuf);
    lances[3] = calcularJuego(juegoBuf);
    printf("Mejor mano a grande: jugador %d\n", lances[0]);
    printf("Mejor mano a chica: jugador %d\n", lances[1]);
    printf("Mejor mano a pares: jugador %d\n", lances[2]);
    printf("Mejor mano a juego: jugador %d\n", lances[3]);

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


    for (i = 0; i < N_JUGADORES; i++) {
        printf("JUEGO DE JUGADOR %d: %d\n", i, juegoBuf[i]);
    }
    printMazo(mazo, N_CARTAS_MAZO);

    MPI_Comm_disconnect(&juego_comm);
    MPI_Finalize();
    return 0;
}


