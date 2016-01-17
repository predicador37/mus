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
    int rank, size, version, subversion, namelen, universe_size, postre, sizeMazo, sizeDescartadas;
    char processor_name[MPI_MAX_PROCESSOR_NAME], worker_program[100];
    MPI_Comm juego_comm;
    Carta mazo[N_CARTAS_MAZO];


    char *caras[] = {"As", "Dos", "Tres", "Cuatro", "Cinco",
                     "Seis", "Siete", "Sota", "Caballo", "Rey"};


    char *palos[] = {"Oros", "Copas", "Espadas", "Bastos"};
    int valores[] = {1, 1, 10, 4, 5, 6, 7, 10, 10, 10};

    srand(time(NULL)); /* randomize */

    sizeMazo = crearMazo(mazo, caras, palos, valores); /* llena el mazo de cartas */
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
    MPI_Recv(&postre, 1, MPI_INT, corte, 0, juego_comm, MPI_STATUS_IGNORE);
    int mano = add_mod(postre, 1, 4);
    printf("[maestro] El jugador postre es: %d\n", postre);
    printf("[maestro] El jugador mano es: %d\n", mano);
    MPI_Bcast(&postre, 1, MPI_INT, MPI_ROOT, juego_comm);
    /* envío del mazo al jugador postre, que va a repartir */
    enviarMazo(mazo, postre, juego_comm);

    /* e/s auxiliar reparto de cartas */
    int i = 0;
    for (i = 0; i <= (N_CARTAS_MANO * N_JUGADORES - 1); i++) {
        int buffer[3];
        MPI_Recv(&buffer, 3, MPI_INT, postre, 0, juego_comm, MPI_STATUS_IGNORE);
        printf("[postre %d] Se reparte carta %d al jugador %d\n", postre, buffer[0], buffer[1]);
        int siguiente = buffer[1];
        MPI_Recv(&buffer, 3, MPI_INT, siguiente, 0, juego_comm, MPI_STATUS_IGNORE);
        printf("[jugador %d] Jugador %d recibe carta %d con valor %d\n", buffer[0], buffer[0], buffer[1], buffer[2]);
    }

    MPI_Recv(&sizeMazo, 1, MPI_INT, postre, 0, juego_comm, MPI_STATUS_IGNORE);
    printf("[maestro] tamaño del mazo: %d\n", sizeMazo);
    int counts[5]={10, 10, 10, 10,0};
    int displs[5]={0, 10, 20, 30,40};
    int conteos[10];
    for (i=0;i<10;i++) {
        conteos[i]=0;
    }
    int rbuf[50];
    int ganador;
    //MPI_Gatherv(conteos, 0, MPI_INT, rbuf, counts, displs, MPI_INT, MPI_ROOT, juego_comm);
    MPI_Gather(conteos, 10, MPI_INT, rbuf, 10, MPI_INT, MPI_ROOT, juego_comm);

    int k=0;
    int empates[4];
    for (i=0;i<4;i++){
        empates[i]=0;
    }
    for (k=0; k<40; k++){
        printf("Conteo carta %d: %d\n", k, rbuf[k]);
    }

    /*cálculo de puntos a grande*/
    /* TODO: empaquetar para siempre*/
    /* parametros: rbuf */
    /* mover array de empates adentro */
    /* devuelve entero con proceso ganador */

    for (k=0; k<10; k++) {
        if (k==0) { /* se buscan reyes y treses*/
            printf("Contando reyes\n");
            int suma[4];
            suma[0]=rbuf[0]+rbuf[7];
            printf("Reyes para proceso 0: %d\n", suma[0]);
            suma[1]=rbuf[10]+rbuf[17];
            printf("Reyes para proceso 1: %d\n", suma[1]);
            suma[2]=rbuf[20]+rbuf[27];
            printf("Reyes para proceso 2: %d\n", suma[2]);
            suma[3]=rbuf[30]+rbuf[37];
            printf("Reyes para proceso 3: %d\n", suma[3]);
            int maximo = maximoArray(suma, 4);
            printf("Conteo máximo: %d\n", maximo);
            int ocurrencias = ocurrenciasArray(suma, 4, maximo);
            if (ocurrencias == 1) { //el jugador gana porque tiene más reyes
                ganador = buscaIndice(suma, 4, maximo);
                break;
            }
            else {
                for (i=0; i<4; i++){
                    if (suma[i] == maximo){
                        empates[i] =1;
                    }
                }
            }
        }

        else if (k>0 && k<7) {
            int suma[4];
            suma[0] = rbuf[k] * empates[0];
            suma[1] = rbuf[k+10] * empates[1];
            suma[2] = rbuf[k+20] * empates[2];
            suma[3] = rbuf[k+30] * empates[3];
            if (k==1){
                printf("Caballos para proceso 0: %d\n", suma[0]);

                printf("Caballos para proceso 1: %d\n", suma[1]);

                printf("Caballos para proceso 2: %d\n", suma[2]);

                printf("Caballos para proceso 3: %d\n", suma[3]);
            }
            int maximo = maximoArray(suma, 4);
            if (maximo!=0) {
                int ocurrencias = ocurrenciasArray(suma, 4, maximo);
                if (ocurrencias == 1 && empates[buscaIndice(suma, 4, maximo)] == 1) {
                    ganador = buscaIndice(suma, 4, maximo);
                    break;
                }
                else {
                    for (i = 0; i < 4; i++) {
                        if (suma[i] == maximo && empates[i] == 1) {
                            empates[i] = 1;
                        }
                        else {
                            empates[i] = 0;
                        }
                    }
                    if (ocurrenciasArray(empates, 4, 1) == 1) {
                        ganador = buscaIndice(suma, 4, 1);
                        break;
                    }
                }
            }
        }
        else if (k==8){  /*se buscan doses y ases*/
            int suma[4];
            suma[0]=rbuf[8]+rbuf[9];
            suma[1]=rbuf[18]+rbuf[19];
            suma[2]=rbuf[28]+rbuf[29];
            suma[3]=rbuf[38]+rbuf[39];
            int maximo = maximoArray(suma, 4);
            int ocurrencias = ocurrenciasArray(suma, 4, maximo);
            if (ocurrencias == 1) { //el jugador gana porque tiene más reyes
                ganador = buscaIndice(suma, 4, maximo);
                break;
            }
        }
    }
    printf("Mejor mano a grande: jugador %d\n", ganador);
    MPI_Comm_disconnect(&juego_comm);
    MPI_Finalize();
    return 0;
}


