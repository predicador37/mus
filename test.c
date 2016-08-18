//
// Created by predicador on 15/06/16.
//

#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include "mus.h"
#include "dbg.h"

#define N_CARTAS_MAZO 40
#define N_CARTAS_MANO 4
#define N_CARTAS_PALO 10
#define N_JUGADORES 4
#define N_PAREJAS 2
#define N_LANCES 4
#define DEBUG 1
#define MODO_JUEGO 1


extern const char * caras[];
extern const char * palos[];
extern const char * lances_etiquetas[];
extern int valores[];
extern int equivalencias[];



int main(int argc, char **argv) {

    /* Test para envido() */

    Carta mano_jugador[N_CARTAS_MANO];
    int equivalencias_jugador[N_CARTAS_MANO], cuentaCartas[N_CARTAS_PALO];
    int i, envite;

    mano_jugador[0].cara = 7;
    mano_jugador[0].palo = 0;
    mano_jugador[0].id = 1;
    mano_jugador[0].orden = 1;
    mano_jugador[0].estado = 0;

    mano_jugador[1].cara = 9;
    mano_jugador[1].palo = 1;
    mano_jugador[1].id = 2;
    mano_jugador[1].orden = 2;
    mano_jugador[1].estado = 0;

    mano_jugador[2].cara = 9;
    mano_jugador[2].palo = 2;
    mano_jugador[2].id = 3;
    mano_jugador[2].orden = 3;
    mano_jugador[2].estado = 0;

    mano_jugador[3].cara = 1;
    mano_jugador[3].palo = 3;
    mano_jugador[3].id = 4;
    mano_jugador[3].orden = 4;
    mano_jugador[3].estado = 0;

    print_mazo(mano_jugador,4);


    for (i = 0; i < N_CARTAS_MANO; i++) {
        equivalencias_jugador[i] = equivalencias[mano_jugador[i].cara];
    }

    int valores_jugador[N_CARTAS_MANO];
    for (i = 0; i < N_CARTAS_MANO; i++) {
        valores_jugador[i] = valores[mano_jugador[i].cara];
    }

    //Grande
    int cuenta = 0;

    for (i = N_CARTAS_PALO - 1; i >= 0; i--) {
        cuenta = cuenta_cartas_mano(mano_jugador,i);

        cuentaCartas[N_CARTAS_PALO - i - 1] = cuenta;

    }

    envite = envido(equivalencias_jugador, N_CARTAS_MANO, 0, 0);

    if (envite == 2) {
        printf("OK\n");
    }
    else {
        printf("FAIL\n");
    }
}