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
/*
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
    }*/

    /*TEST PARA CALCULO DE GANADOR A GRANDE*/

    // La siguiente jugada empata entre dos jugadores, uno de los cuales es mano
    int rbuf_1[40] = {2, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1};
    int rbuf_2[40] = {3, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 2, 0, 0};
    int invertido_1[40]={0, 1, 0, 0, 0, 0, 0, 0, 2, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1};
    int invertido_2[40]={1, 0, 1, 0, 0, 0, 0, 0, 0, 2, 0, 0, 3, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1};
    int pares_1[20]={99, 99, 1, 1, 99, 99, 99, 0, 99, 99, 99, 99, 1, 10, 99,99, 99, 1, 10, 99};
    int pares_2[20]={99,99,0,99,99, 99,99,1,8,99, 99,1,1,1,99, 99,10,1,10,99};
    int pares_3[20]= {99, 10, 1, 10, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 10, 1, 10, 99};
    int juego_1[4]={31, 34, 22, 31};

    int mano_1 = 0;
    int mano_2 = 0;
    int mano_3 = 3;
    int mano_4 = 1;
    int mano_5 = 2;
    int mano_6 = 2;
    int ganador = 88;

   /* ganador = calculaGrande(rbuf_1, mano_1);
    if (ganador == 0) {
        printf("TEST 1 OK: %d\n", ganador);
    }
    else {
        printf("TEST 1 ERROR: %d\n", ganador);
    }
*/
    ganador = calcula_grande(rbuf_2, mano_2);

    printf("TEST 2 : %d\n", ganador);

    /*if (ganador == 0) {
        printf("TEST 2 OK: %d\n", ganador);
    }
    else {
        printf("TEST 2 ERROR: %d\n", ganador);
    }*/

        int n_juegos_vaca = 3;
    printf("AL MEJOR DE %d PARTIDAS\n", (n_juegos_vaca / 2) + 1);

    ganador = calcula_chica(invertido_1, mano_3);
    printf("TEST 3 A CHICA: %d\n", ganador);

    ganador = calcula_chica(invertido_2, mano_4);
    printf("TEST 4 A CHICA: %d\n", ganador);

    ganador=calcular_pares(pares_1, mano_5);
    printf("TEST 5 A PARES: %d\n", ganador);

    ganador=calcularJuego(juego_1, mano_6);
    printf("TEST 6 A JUEGO: %d\n", ganador);

    int piedras = 13;
    int amarracos= 13/5;
    int resto = 13%5;

    printf("Tengo %d amarracos y %d piedras\n", amarracos, resto);

    int envites[4]={5,0,0,0};
    int envite=calcular_envite(envites, 3, 99, 5);
    printf("TEST 6 ENVITE: %d\n", envite);

    int envites_2[4] = {3,1,1,99};
    int envites_3[2]={0,0};
    int equivalencias[4]={10,10,10,8};
    int pares[5]={99,10,1,10,99};
    int puntos_juego[4]={31,31,31,31};
    int envites_jugadores[4]={1,1,1,2};
    int apuesta = apuesta_terminada(envites_jugadores, 4);

    //envido(envites_3, equivalencias, 4, 2, 99, 2, 0,pares, 0, puntos_juego);
    print_envite(envites_3[0], 0, 1, envites_3[1]);
    int envite_2=calcular_envite(envites_3, 2, 0, 99);
    printf("TEST 7 ENVITE: %d\n", envite_2);

    printf("TEST 8: APUESTA: %d\n", apuesta);

    int r, i;
    int N = 3,
            M = 6;
    //RAND_MAX = 10;
    /* initialize random seed: */
    srand (time(NULL));
    printf("Números aleatorios entre %d y %d\n", N, M);
    /* generate secret number between 1 and 10: */
    for(i=0; i < 10 ; i++){
        r = rand() % (M - N + 1) + N;
        printf("%d ", r);
    }

    ganador=calcular_pares(pares_2, 2);
    printf("TEST 9 A PARES: %d\n", ganador);

    int jugadores[] = {99,1,10,99};
    int empate = deshacerEmpateComplementario(jugadores, 2, 99);

    printf("TEST 10: EMPATE: %d\n", empate);

    int maximo = maximo_array_excluyendo(jugadores, 4, 99);
    printf("TEST 11: MAXIMO: %d\n", maximo);

    int pareja_soy = que_pareja_inicial_soy(0);
    int pareja_no_soy = add_mod(pareja_soy, 1, 2);
    int puntos_juego_2[]={29,37};
    int n_puntos_juego = 40;
    printf("TEST 12: PAREJA SOY: %d\n", pareja_soy);
    printf("TEST 12: PAREJA NO SOY: %d\n", pareja_no_soy);



    int ordago_1 = ordago(0, 3, puntos_juego_2, n_puntos_juego);
    printf ("TEST 13: ORDAGO: %d", ordago_1);

    ganador=calcular_pares(pares_3, 1);
    printf("TEST 13 A PARES: %d\n", ganador);

    return 0;
}