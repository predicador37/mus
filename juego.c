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


int main(int argc, char **argv) {
    int rank, size, version, subversion, namelen, universe_size, repartidor;
    char processor_name[MPI_MAX_PROCESSOR_NAME], worker_program[100];
    MPI_Comm juego_comm;
    Carta mazo[N_CARTAS_MAZO];


     char *caras[] = {"As", "Dos", "Tres", "Cuatro", "Cinco",
                           "Seis", "Siete", "Sota", "Caballo", "Rey"};


     char *palos[] = { "Oros", "Copas", "Espadas", "Bastos"};
     int valores[] = {1, 1, 10, 4, 5, 6, 7, 10, 10, 10};

    srand(time(NULL)); /* randomize */

    crearMazo(mazo, caras, palos, valores); /* llena el mazo de cartas */
    printMazo(mazo); /*Imprime el mazo*/
    printf("\n");
    barajarMazo(mazo); /*Baraja el mazo*/
    printMazo(mazo); /*Vuelve a imprimir el mazo con las cartas barajadas*/
    //cortarMazo(mazo);
    printf("\n");
    MPI_Init(&argc, &argv);    /* starts MPI */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);    /* get current process id */
    MPI_Comm_size(MPI_COMM_WORLD, &size);    /* get number of processes */
    MPI_Get_processor_name(processor_name, &namelen);
    //MPI_Status stat;
    MPI_Get_version(&version, &subversion);
    printf("Iniciado proceso maestro %d de %d en %s ejecutando MPI %d.%d\n", rank, size, processor_name, version,
           subversion);
    if (size != 1)
        printf("Error: sólo debería estar ejecutándose el proceso maestro, pero hay %d procesos ejecutándose\n", size);


/* Fijar el total de procesos a ejecutar incluyendo el maestro */
    universe_size = 5;
    strcpy(worker_program, "./jugador");
    printf("Generando %d procesos ejecutando %s\n", universe_size - 1, worker_program);
    MPI_Comm_spawn(worker_program, MPI_ARGV_NULL, universe_size - 1, MPI_INFO_NULL, 0, MPI_COMM_SELF, &juego_comm,
                   MPI_ERRCODES_IGNORE);
    printf("Ejecutado proceso maestro con identificador %d de un total de %d\n", rank, size);

    int corte; /* jugador que realizará el corte */
    int N = 0, M = N_JUGADORES - 1; /* valores del intervalo */
    corte = M + rand() / (RAND_MAX / (N - M + 1) + 1); /* proceso aleatorio de entre los existentes */
    MPI_Bcast(&corte, 1, MPI_INT, MPI_ROOT, juego_comm); /* envío del id de proceso que realizará el corte a todos*/
    //MPI_Barrier(juego_comm);
    /*
    int j=0;
    for (j=0;j<N_CARTAS_MAZO;j++) {
        MPI_Send(&mazo[j].id, 1, MPI_INT, corte, 0, juego_comm);
        MPI_Send(&mazo[j].valor, 1, MPI_INT, corte, 0, juego_comm);
        MPI_Send(mazo[j].palo, 7, MPI_CHAR, corte, 0, juego_comm);
        MPI_Send(mazo[j].cara, 8, MPI_CHAR, corte, 0, juego_comm);
    }*/
    enviarMazo(mazo, corte, juego_comm);

    MPI_Recv(&repartidor, 1, MPI_INT, corte, 0, juego_comm, MPI_STATUS_IGNORE);
    int mano = add_mod(repartidor,1,4);
    //printf("%d + 1 mod 4 = %d\n", repartidor, mano );
    //MPI_Bcast(&repartidor, 1, MPI_INT, MPI_ROOT, juego_comm);
    //MPI_Barrier(juego_comm);
    printf("El proceso repartidor es: %d\n", repartidor);
    printf("El proceso mano es: %d\n", mano);
    //MPI_Send(palabras, 10, MPI_CHAR, 0, 0, juego_comm);
    MPI_Bcast(&repartidor, 1, MPI_INT, MPI_ROOT, juego_comm);
    /*
    int j=0;
    for (j=0;j<N_CARTAS_MAZO;j++) {
        MPI_Send(&mazo[j].id, 1, MPI_INT, repartidor, 0, juego_comm);
        MPI_Send(&mazo[j].valor, 1, MPI_INT, repartidor, 0, juego_comm);
        MPI_Send(mazo[j].palo, 7, MPI_CHAR, repartidor, 0, juego_comm);
        MPI_Send(mazo[j].cara, 8, MPI_CHAR, repartidor, 0, juego_comm);
    }*/
    enviarMazo(mazo, repartidor, juego_comm);

    //MPI_Bcast(&valor, 1, MPI_INT, MPI_ROOT, &juego_comm);
    MPI_Comm_disconnect(&juego_comm);
    MPI_Finalize();
    return 0;
}


