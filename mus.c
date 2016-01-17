/* C Example */
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "mus.h"


#define N_CARTAS_MAZO 40
#define N_CARTAS_MANO 4

typedef int bool;
#define true 1
#define false 0

struct jugador {
    Carta *mano[N_CARTAS_MANO];
    char *palo;
    bool postre;
};

typedef struct jugador Jugador;

int cartaActual = 0;

/* FUNCION crearMazo: puebla un array de estructuras Carta con sus valores y palos*/
int crearMazo(Carta * mazo,  char *strCara[],
                char *strPalo[],  int intValor[]) {
    int i; /* contador */
   int sizeMazo = 0;

    /* iterar el mazo */
    for (i = 0; i <= N_CARTAS_MAZO - 1; i++) {
        mazo[i].cara = strCara[i % 10];
        mazo[i].palo = strPalo[i / 10];
        mazo[i].valor = intValor[i % 10];
        mazo[i].id = i;
        sizeMazo++;

    } /* fin for */
    return sizeMazo;

} /* fin funcion crearMazo */

/* FUNCION printMazo: muestra por pantalla un mazo de cartas */
void printMazo( Carta * wMazo, int sizeMazo) {
    int i;
    for (i = 0; i <= sizeMazo - 1; i++) {
        printf("El valor de %-8s\t de \t%-8s es \t%d \tcon id \t%d\n \t", wMazo[i].cara,
               wMazo[i].palo, wMazo[i].valor, wMazo[i].id);
        printf("\n");
    }
    printf("Fin del contenido del mazo.\n");
}

/* FUNCION barajarMazo: baraja las cartas del mazo*/

void barajarMazo(Carta * wMazo) {
    int i;     /* contador */
    int j;     /* variable que almacena un valor aleatorio entre 0 y 39*/
    Carta temp; /* estructura temporal para intercambiar las cartas */

    /* iterar sobre el mazo intercambiando cartas aleatoriamente */

    for (i = 0; i <= N_CARTAS_MAZO - 1; i++) {
        j = rand() % N_CARTAS_MAZO;
        temp = wMazo[i];
        wMazo[i] = wMazo[j];
        wMazo[j] = temp;
    } /* fin for */
    printf("Mazo barajado.\n\n");

} /* fin funcion barajar */

/* FUNCION cortarMazo: corta el mazo, esto es, saca una carta aleatoria del mazo */

void cortarMazo(Carta * wMazo, char ** paloCorte) {

    int r; /* índice aleatorio para el mazo*/
    int N = 0, M = N_CARTAS_MAZO - 1; /* valores del intervalo */
    r = M + rand() / (RAND_MAX / (N - M + 1) + 1);
    //printf("\nCarta visible al cortar el mazo: \n");
   // printf("%-8s\t de \t%-8s es \t%d \tcon id \t%d\n \t", wMazo[r].cara,
    //       wMazo[r].palo, wMazo[r].valor, wMazo[r].id);
    *paloCorte = wMazo[r].palo;

} /* fin funcion cortarMazo */

int add_mod(int a, int b, int m)
{
    if ( 0==b ) return a;

    // return sub_mod(a, m-b, m);
    b = m - b;
    if ( a>=b )
        return a - b;
    else
        return m - b + a;
}

void enviarMazo(Carta * wMazo, int proceso, MPI_Comm wComm) {

    int j = 0;
    for (j = 0; j < N_CARTAS_MAZO; j++) {
        MPI_Send(&wMazo[j].id, 1, MPI_INT, proceso, 0, wComm);
        MPI_Send(&wMazo[j].valor, 1, MPI_INT, proceso, 0, wComm);
        MPI_Send(wMazo[j].palo, 7, MPI_CHAR, proceso, 0, wComm);
        MPI_Send(wMazo[j].cara, 8, MPI_CHAR, proceso, 0, wComm);
    }
}

    void recibirMazo(Carta * wMazo, int proceso, MPI_Comm wComm, MPI_Status stat) {

        int i = 0;
        for (i = 0; i < N_CARTAS_MAZO; i++) {
            wMazo[i].palo = (char *) malloc(5 * sizeof(char));
            wMazo[i].cara = (char *) malloc(8 * sizeof(char));
            MPI_Recv(&wMazo[i].id, 1, MPI_INT, proceso, 0, wComm, &stat);
            MPI_Recv(&wMazo[i].valor, 1, MPI_INT, proceso, 0, wComm, &stat);
            MPI_Recv(wMazo[i].palo, 7, MPI_CHAR, proceso, 0, wComm, &stat);
            MPI_Recv(wMazo[i].cara, 8, MPI_CHAR, proceso, 0, wComm, &stat);
        }
    }

void enviarCarta (Carta wCarta, int proceso, MPI_Comm wComm){

    MPI_Send(&wCarta.id, 1, MPI_INT, proceso, 0,  wComm);
    MPI_Send(&wCarta.valor, 1, MPI_INT, proceso, 0,  wComm);
    MPI_Send(wCarta.palo, 7, MPI_CHAR, proceso, 0,  wComm);
    MPI_Send(wCarta.cara, 8, MPI_CHAR, proceso, 0,  wComm);
    //printf("Enviada carta: %s de %s con valor %d\n", wCarta.cara, wCarta.palo, wCarta.valor);

}

Carta recibirCarta(int proceso, MPI_Comm wComm, MPI_Status stat) {
        Carta wCarta;
        wCarta.palo = (char *) malloc(5 * sizeof(char));
        wCarta.cara = (char *) malloc(8 * sizeof(char));
        MPI_Recv(&wCarta.id, 1, MPI_INT, proceso, 0, wComm, &stat);
        MPI_Recv(&wCarta.valor, 1, MPI_INT, proceso, 0, wComm, &stat);
        MPI_Recv(wCarta.palo, 7, MPI_CHAR, proceso, 0, wComm, &stat);
        MPI_Recv(wCarta.cara, 8, MPI_CHAR, proceso, 0, wComm, &stat);
    //printf("Recibida carta: %s de %s con valor %d\n", wCarta.cara, wCarta.palo, wCarta.valor);
    return wCarta;

}



