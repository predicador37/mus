/* C Example */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
int crearMazo(Carta *mazo, char *strCara[],
              char *strPalo[], int intValor[], int intEquivalencias[]) {
    int i; /* contador */
    int sizeMazo = 0;

    /* iterar el mazo */
    for (i = 0; i <= N_CARTAS_MAZO - 1; i++) {
        mazo[i].cara = strCara[i % 10];
        mazo[i].palo = strPalo[i / 10];
        mazo[i].valor = intValor[i % 10];
        mazo[i].equivalencia = intEquivalencias[i % 10];
        mazo[i].id = i;
        mazo[i].orden = i % 10;
        mazo[i].estado = 0;
        sizeMazo++;

    } /* fin for */
    return sizeMazo;

} /* fin funcion crearMazo */

/* FUNCION printMazo: muestra por pantalla un mazo de cartas */
void printMazo(Carta *wMazo, int sizeMazo) {
    int i;
    for (i = 0; i <= sizeMazo - 1; i++) {
        printf("El valor de %-8s\t de \t%-8s es \t%d \tcon orden \t%d y estado %d\n \t", wMazo[i].cara,
               wMazo[i].palo, wMazo[i].valor, wMazo[i].orden, wMazo[i].estado);
        printf("\n");
    }
    printf("Fin del contenido del mazo.\n");
}

/* FUNCION barajarMazo: baraja las cartas del mazo*/

void barajarMazo(Carta *wMazo) {
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

void cortarMazo(Carta *wMazo, char **paloCorte) {

    int r; /* índice aleatorio para el mazo*/
    int N = 0, M = N_CARTAS_MAZO - 1; /* valores del intervalo */
    r = M + rand() / (RAND_MAX / (N - M + 1) + 1);
    //printf("\nCarta visible al cortar el mazo: \n");
    // printf("%-8s\t de \t%-8s es \t%d \tcon id \t%d\n \t", wMazo[r].cara,
    //       wMazo[r].palo, wMazo[r].valor, wMazo[r].id);
    *paloCorte = wMazo[r].palo;

}

/* fin funcion cortarMazo */

int add_mod(int a, int b, int m) {
    if (0 == b) return a;

    // return sub_mod(a, m-b, m);
    b = m - b;
    if (a >= b)
        return a - b;
    else
        return m - b + a;
}

void enviarMazo(Carta *wMazo, int proceso, MPI_Comm wComm) {

    int j = 0;
    for (j = 0; j < N_CARTAS_MAZO; j++) {
        MPI_Send(&wMazo[j].id, 1, MPI_INT, proceso, 0, wComm);
        MPI_Send(&wMazo[j].valor, 1, MPI_INT, proceso, 0, wComm);
        MPI_Send(&wMazo[j].equivalencia, 1, MPI_INT, proceso, 0, wComm);
        MPI_Send(&wMazo[j].orden, 1, MPI_INT, proceso, 0, wComm);
        MPI_Send(&wMazo[j].estado, 1, MPI_INT, proceso, 0, wComm);
        MPI_Send(wMazo[j].palo, 7, MPI_CHAR, proceso, 0, wComm);
        MPI_Send(wMazo[j].cara, 8, MPI_CHAR, proceso, 0, wComm);
    }
}

void recibirMazo(Carta *wMazo, int proceso, MPI_Comm wComm, MPI_Status *stat) {

    int i = 0;
    for (i = 0; i < N_CARTAS_MAZO; i++) {
        wMazo[i].palo = (char *) malloc(5 * sizeof(char));
        wMazo[i].cara = (char *) malloc(8 * sizeof(char));
        MPI_Recv(&wMazo[i].id, 1, MPI_INT, proceso, 0, wComm, stat);
        MPI_Recv(&wMazo[i].valor, 1, MPI_INT, proceso, 0, wComm, stat);
        MPI_Recv(&wMazo[i].equivalencia, 1, MPI_INT, proceso, 0, wComm, stat);
        MPI_Recv(&wMazo[i].orden, 1, MPI_INT, proceso, 0, wComm, stat);
        MPI_Recv(&wMazo[i].estado, 1, MPI_INT, proceso, 0, wComm, stat);
        MPI_Recv(wMazo[i].palo, 7, MPI_CHAR, proceso, 0, wComm, stat);
        MPI_Recv(wMazo[i].cara, 8, MPI_CHAR, proceso, 0, wComm, stat);
    }
}

void repartirCarta(Carta wCarta, int proceso, MPI_Comm wComm) {
    MPI_Send(&wCarta.id, 1, MPI_INT, proceso, 0, wComm);
    MPI_Send(&wCarta.valor, 1, MPI_INT, proceso, 0, wComm);
    MPI_Send(&wCarta.equivalencia, 1, MPI_INT, proceso, 0, wComm);
    MPI_Send(&wCarta.orden, 1, MPI_INT, proceso, 0, wComm);
    MPI_Send(&wCarta.estado, 1, MPI_INT, proceso, 0, wComm);
    MPI_Send(wCarta.palo, 7, MPI_CHAR, proceso, 0, wComm);
    MPI_Send(wCarta.cara, 8, MPI_CHAR, proceso, 0, wComm);
    //printf("Enviada carta: %s de %s con valor %d\n", wCarta.cara, wCarta.palo, wCarta.valor);

}

Carta recibirCarta(int proceso, MPI_Comm wComm, MPI_Status stat) {
    Carta wCarta;
    wCarta.palo = (char *) malloc(5 * sizeof(char));
    wCarta.cara = (char *) malloc(8 * sizeof(char));
    MPI_Recv(&wCarta.id, 1, MPI_INT, proceso, 0, wComm, &stat);
    MPI_Recv(&wCarta.valor, 1, MPI_INT, proceso, 0, wComm, &stat);
    MPI_Recv(&wCarta.equivalencia, 1, MPI_INT, proceso, 0, wComm, &stat);
    MPI_Recv(&wCarta.orden, 1, MPI_INT, proceso, 0, wComm, &stat);
    MPI_Recv(&wCarta.estado, 1, MPI_INT, proceso, 0, wComm, &stat);
    MPI_Recv(wCarta.palo, 7, MPI_CHAR, proceso, 0, wComm, &stat);
    MPI_Recv(wCarta.cara, 8, MPI_CHAR, proceso, 0, wComm, &stat);
    wCarta.estado = 1;
    //printf("Recibida carta: %s de %s con valor %d\n", wCarta.cara, wCarta.palo, wCarta.valor);
    return wCarta;
}

int cuentaCartasMano(Carta *wMano, char *cara) {
    int i = 0;
    int cuenta = 0;
    for (i = 0; i < N_CARTAS_MANO; i++) {
        if (strcmp(wMano[i].cara, cara) == 0) {
            cuenta++;
        }
    }
    return cuenta;
}

int maximoArray(int array[], int longitud) {
    int i = 0;
    int max = array[0];

    for (i; i < longitud; i++) {
        if (max < array[i]) {
            max = array[i];
        }
    }
    return max;
}

int maximoArrayExcluyendo(int array[], int longitud, int excluido) {
    int i = 0;
    int max = array[0];

    for (i; i < longitud; i++) {
        if (max < array[i] && array[i] != excluido) {
            max = array[i];
        }
    }
    return max;
}

int ocurrenciasArray(int array[], int longitud, int numero) {
    int n = 0;
    int i = 0;
    for (i = 0; i < longitud; i++) {
        if (numero == array[i])
            n++;
    }
    return n;
}

int buscaIndice(int a[], int longitud, int numero) {
    int index = 0;

    while (index < longitud && a[index] != numero) ++index;

    return (index == longitud ? -1 : index);
}

int buscarIndiceNumeroNoIgual(int a[], int longitud, int numero) {
    int index = 0;

    while (index < longitud && a[index] == numero) ++index;

    return (index == longitud ? -1 : index);
}

void invertirArray(int *orig, int *dest, int longitud) {
    int i = longitud - 1;
    int j = 0;
    int swap;

    for (i; i >= 0; i--) //increment a and decrement b until they meet each other
    {
        dest[j] = orig[i];
        j++;
    }
}


int calculaGrande(int rbuf[]) {

    int empates[4];
    int i, k = 0;

    for (i = 0; i < 4; i++) {
        empates[i] = 0;
    }
    int ganador;
    for (k = 0; k < 10; k++) {
        if (k == 0) { /* se buscan reyes y treses*/
            printf("Contando reyes\n");
            int suma[4];
            suma[0] = rbuf[0] + rbuf[7];
            printf("Reyes para proceso 0: %d\n", suma[0]);
            suma[1] = rbuf[10] + rbuf[17];
            printf("Reyes para proceso 1: %d\n", suma[1]);
            suma[2] = rbuf[20] + rbuf[27];
            printf("Reyes para proceso 2: %d\n", suma[2]);
            suma[3] = rbuf[30] + rbuf[37];
            printf("Reyes para proceso 3: %d\n", suma[3]);
            int maximo = maximoArray(suma, 4);
            printf("Conteo máximo: %d\n", maximo);
            int ocurrencias = ocurrenciasArray(suma, 4, maximo);
            if (ocurrencias == 1) { //el jugador gana porque tiene más reyes
                ganador = buscaIndice(suma, 4, maximo);
                break;
            }
            else {
                for (i = 0; i < 4; i++) {
                    if (suma[i] == maximo) {
                        empates[i] = 1;
                    }
                }
            }
        }

        else if (k > 0 && k < 7) {
            int suma[4];
            suma[0] = rbuf[k] * empates[0];
            suma[1] = rbuf[k + 10] * empates[1];
            suma[2] = rbuf[k + 20] * empates[2];
            suma[3] = rbuf[k + 30] * empates[3];
            if (k == 1) {
                printf("Caballos para proceso 0: %d\n", suma[0]);

                printf("Caballos para proceso 1: %d\n", suma[1]);

                printf("Caballos para proceso 2: %d\n", suma[2]);

                printf("Caballos para proceso 3: %d\n", suma[3]);
            }
            int maximo = maximoArray(suma, 4);
            if (maximo != 0) {
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
        else if (k == 8) {  /*se buscan doses y ases*/
            int suma[4];
            suma[0] = rbuf[8] + rbuf[9];
            suma[1] = rbuf[18] + rbuf[19];
            suma[2] = rbuf[28] + rbuf[29];
            suma[3] = rbuf[38] + rbuf[39];
            int maximo = maximoArray(suma, 4);
            int ocurrencias = ocurrenciasArray(suma, 4, maximo);
            if (ocurrencias == 1) { //el jugador gana porque tiene más reyes
                ganador = buscaIndice(suma, 4, maximo);
                break;
            }
        }
    }
    return ganador;
}

int calculaChica(int rbufInv[]) {

    int empates[4];
    int i, k = 0;

    for (i = 0; i < 4; i++) {
        empates[i] = 0;
    }
    int ganador;
    for (k = 0; k < 10; k++) {
        if (k == 0) { /* se buscan Ases y treses*/
            printf("Contando ases\n");
            int suma[4];
            suma[0] = rbufInv[0] + rbufInv[1];
            printf("Ases para proceso 0: %d\n", suma[0]);
            suma[1] = rbufInv[10] + rbufInv[11];
            printf("Ases para proceso 1: %d\n", suma[1]);
            suma[2] = rbufInv[20] + rbufInv[21];
            printf("Ases para proceso 2: %d\n", suma[2]);
            suma[3] = rbufInv[30] + rbufInv[31];
            printf("Ases para proceso 3: %d\n", suma[3]);
            int maximo = maximoArray(suma, 4);
            printf("Conteo máximo: %d\n", maximo);
            int ocurrencias = ocurrenciasArray(suma, 4, maximo);
            if (ocurrencias == 1) { //el jugador gana porque tiene más Ases
                ganador = buscaIndice(suma, 4, maximo);
                break;
            }
            else {
                for (i = 0; i < 4; i++) {
                    if (suma[i] == maximo) {
                        empates[i] = 1;
                    }
                }
            }
        }

        else if (k > 2 && k < 9) {
            int suma[4];
            suma[0] = rbufInv[k] * empates[0];
            suma[1] = rbufInv[k + 10] * empates[1];
            suma[2] = rbufInv[k + 20] * empates[2];
            suma[3] = rbufInv[k + 30] * empates[3];
            if (k == 3) {
                printf("Cuatros para proceso 0: %d\n", suma[0]);

                printf("Cuatros para proceso 1: %d\n", suma[1]);

                printf("Cuatros para proceso 2: %d\n", suma[2]);

                printf("Cuatros para proceso 3: %d\n", suma[3]);
            }
            int maximo = maximoArray(suma, 4);
            if (maximo != 0) {
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
        else if (k == 9) {  /*se buscan treses y reyes*/
            int suma[4];
            suma[0] = rbufInv[9] + rbufInv[2];
            suma[1] = rbufInv[19] + rbufInv[12];
            suma[2] = rbufInv[29] + rbufInv[22];
            suma[3] = rbufInv[39] + rbufInv[32];
            int maximo = maximoArray(suma, 4);
            int ocurrencias = ocurrenciasArray(suma, 4, maximo);
            if (ocurrencias == 1) { //el jugador gana porque tiene más Ases
                ganador = buscaIndice(suma, 4, maximo);
                break;
            }
        }
    }
    return ganador;
}

int cmpfunc(const void *a, const void *b) {
    return (*(int *) a - *(int *) b);
}
/* Encuentra parejas únicas en un array de longitud. Está preparado para una mano */
/* Devuelve array con 3 elementos: el número de parejas y las cartas asociadas */
int *uniquePairs(int *array, int longitud, int repeticion) {
    qsort(array, longitud, sizeof(int), cmpfunc);
    // now we have: [1, 1, 1, 4, 4, 5, 5, 7, 7] //
    int k = 0;
    for (k = 0; k < longitud; k++) {
    }

    int res[3] = {0, 99, 99}; //99 es valor fuera de rango para una carta
    int i = 0;

    while (i < longitud) {
        // take first number
        int num = array[i];
        int c = 1;
        i++;

        // count all duplicates
        while (i < longitud && array[i] == num) {
            c++;
            i++;
        }
        //printf("Number: %d, Count: %d\n", num ,c);
        // if we spotted number just 2 times, increment result
        if (c == repeticion) {
            res[0]++;

            if (res[1] == 99) {
                res[1] = num - 1;
            }
            else {
                res[2] = num - 1;
            }

        }
    }

    return res;
}

int calcularPares(int paresBuf[]) {
    /* pares */
    /* parámetros: paresBuf */
    /* devuelve entero con proceso ganador */

    //TODO parametrizar las constantes

    int jugadores[4];
    int valoresPares[4];
    int ganador = 99;
    int empates[4];
    int i = 0;
    for (i = 0; i < 4; i++) {
        empates[i] = 0;
    }
    /* se buscan duples de la misma carta*/
    jugadores[0] = paresBuf[0];
    jugadores[1] = paresBuf[5];
    jugadores[2] = paresBuf[10];
    jugadores[3] = paresBuf[15];
    int ocurrencias = ocurrenciasArray(jugadores, 4, 99);
    if (ocurrencias == 3) { //hay un proceso con duples
        ganador = buscarIndiceNumeroNoIgual(jugadores, 4, 99);
    }

    else { // hay empates
        int maximo = 99;
        maximo = maximoArrayExcluyendo(jugadores, 4, 99);
        int ocurrencias = ocurrenciasArray(jugadores, 4, maximo);
        if (ocurrencias == 1) { //el jugador gana porque el duples es mayor
            ganador = buscaIndice(jugadores, 4, maximo);

        }
        else if (ocurrencias != 4) {
            //TODO resolver empate: el más cercano a la mano
            printf("HAY EMPATE A DUPLES DE LA MISMA CARTA...\n");
        }
    }
    if (ganador == 99) { //no habia duples de la misma carta o hay empate, se buscan duples
        printf("Calculando duples...\n");
        jugadores[0] = paresBuf[2];
        jugadores[1] = paresBuf[7];
        jugadores[2] = paresBuf[12];
        jugadores[3] = paresBuf[17];
        ocurrencias = ocurrenciasArray(jugadores, 4, 2);
        if (ocurrencias == 1) { //hay un proceso con duples
            ganador = buscaIndice(jugadores, 4, 2);
        }
        else if (ocurrencias > 1) { //hay empates
            printf("Resolviendo empate a duples con primera pareja...\n");
            for (i = 0; i < 4; i++) { //se intenta resolver el empate con la primera pareja
                if (jugadores[i] == 2) {
                    valoresPares[i] = paresBuf[5 * i + 3];
                }
                else {
                    valoresPares[i] = 0;
                }

            }
            int maximo = maximoArray(valoresPares, 4);
            ocurrencias = ocurrenciasArray(valoresPares, 4, maximo);
            if (ocurrencias == 1) { //el jugador gana porque tiene cartas más altas
                ganador = buscaIndice(valoresPares, 4, maximo);
            }
            else { //vuelve a haber empates
                for (i = 0; i < 4; i++) { //se intenta resolver el empate con la segunda pareja
                    if (jugadores[i] == 2) {
                        valoresPares[i] = paresBuf[5 * i + 4];
                    }
                    else {
                        valoresPares[i] = 0;
                    }

                }
                int maximo = maximoArray(valoresPares, 4);
                ocurrencias = ocurrenciasArray(valoresPares, 4, maximo);
                if (ocurrencias == 1) { //el jugador gana porque tiene cartas más altas
                    ganador = buscaIndice(valoresPares, 4, maximo);
                }
                else { //vuelve a haber empates
                    //TODO resolver empate: el más cercano a la mano
                    printf("HAY EMPATE A DUPLES...\n");
                }

            }
        }
    }
    if (ganador == 99) { //no habia duples, se buscan medias
        printf("Calculando medias...\n");
        jugadores[0] = paresBuf[1];
        jugadores[1] = paresBuf[6];
        jugadores[2] = paresBuf[11];
        jugadores[3] = paresBuf[16];
        ocurrencias = ocurrenciasArray(jugadores, 4, 99);
        if (ocurrencias == 3) { //hay un proceso con medias
            ganador = buscarIndiceNumeroNoIgual(jugadores, 4, 99);
        }
        else if (ocurrencias != 4) { // hay empates
            int maximo = maximoArrayExcluyendo(jugadores, 4, 99);
            int ocurrencias = ocurrenciasArray(jugadores, 4, maximo);
            if (ocurrencias == 1) { //el jugador gana porque las medias son de mejores cartas
                ganador = buscaIndice(jugadores, 4, maximo);

            }
            else if (ocurrencias != 4) {
                //TODO resolver empate: el más cercano a la mano
                printf("HAY EMPATE A MEDIAS...\n");
            }

        }
    }
    if (ganador == 99) { //no habia medias, se buscan parejas sencillas
        printf("Calculando pares...\n");
        jugadores[0] = paresBuf[2];
        jugadores[1] = paresBuf[7];
        jugadores[2] = paresBuf[12];
        jugadores[3] = paresBuf[17];
        ocurrencias = ocurrenciasArray(jugadores, 4, 1);
        if (ocurrencias == 1) { //hay un proceso con parejas
            ganador = buscaIndice(jugadores, 4, 1);
        }
        else { // empates
            printf("Resolviendo empate a parejas...\n");
            for (i = 0; i < 4; i++) { //se intenta resolver el empate con la pareja
                if (jugadores[i] == 1) {
                    valoresPares[i] = paresBuf[5 * i + 3];

                }
                else {
                    valoresPares[i] = 0;
                }

            }
            int maximo = maximoArray(valoresPares, 4);
            ocurrencias = ocurrenciasArray(valoresPares, 4, maximo);
            if (ocurrencias == 1) { //el jugador gana porque tiene cartas más altas
                ganador = buscaIndice(valoresPares, 4, maximo);
            }
            else { // otra ves empates
                //TODO resolver empate: el más cercano a la mano
                printf("HAY EMPATE A PAREJAS...\n");
            }
        }
    }
    return ganador;
}

int sumaArray(int a[], int longitud) {
    int i, sum = 0;
    for (i = 0; i < longitud; i++) {
        sum = sum + a[i];
    }
    return (sum);
}

int calcularJuego(int juegoBuf[]) {
    /* JUEGO */

    int ocurrencias = 0;
    int ganador = 99;
    int i = 0;
    int juegosGanadores[7] = {31, 40, 37, 36, 35, 34, 33};

    for (i = 0; i < 7; i++) {
        ocurrencias = ocurrenciasArray(juegoBuf, 4, juegosGanadores[i]);
        if (ocurrencias == 1) {
            ganador = buscaIndice(juegoBuf, 4, juegosGanadores[i]);
            break;
        }
        else if (ocurrencias > 1) { //empates
            printf("DESHACER EMPATE CON LA MANO...\n");
        }
    }

    if (ganador == 99) { // al punto
        for (i = 30; i >= 0; i--) {
            ocurrencias = ocurrenciasArray(juegoBuf, 4, i);

            if (ocurrencias == 1) {
                ganador = buscaIndice(juegoBuf, 4, juegosGanadores[i]);
                break;
            }
            else if (ocurrencias > 1) { //empates
                printf("DESHACER EMPATE CON LA MANO...\n");
            }
        }
    }
    return ganador;
}

int tengoJuego(int suma) {
    int juegosGanadores[7] = {31, 40, 37, 36, 35, 34, 33};
    if (ocurrenciasArray(juegosGanadores, 7, suma) == 1) {
        //hay juego
        return 1;
    }
    else return 0;
}

int tengoMedias(int *paresBuf) {
    if (paresBuf[1] != 99) {
        return 1;
    }
    else {
        return 0;
    }
}

int tengoDuples(int *paresBuf) {

    if (paresBuf[0] != 99 || paresBuf[2] == 2) {
        return 1;
    }
    else {
        return 0;
    }
}

int tengoPares(int *paresBuf) {
    if (paresBuf[2] == 1) {
        return 1;
    }
    else {
        return 0;
    }
}

int cortarMus(int *valores, int *equivalencias, int *paresBuf) {


/*
 if (contrario a 5 puntos de ganar) cortar mus */
    int juego = sumaArray(valores, N_CARTAS_MANO);
    if ((juego == 31) || (tengoDuples(paresBuf) == 1) || (tengoMedias(paresBuf) == 1) ||
        (tengoPares(paresBuf) && tengoJuego(juego))) {
        return 1;
    }

    else {
        return 0;
    }


}

void musCorrido(int mus, int *rank, int *jugadorMano, int *turno, int *siguienteJugador, int bufferRcv[],
                MPI_Comm parent) {

    (*turno)++;
    if (mus == 1) {
        printf("[jugador %d] CORTO MUS!!\n", *rank);
        *jugadorMano = *rank;
        MPI_Send(jugadorMano, 1, MPI_INT, 0, 0, parent);
        MPI_Send(siguienteJugador, 1, MPI_INT, 0, 0, parent);
        MPI_Send(turno, 1, MPI_INT, 0, 0, parent);
        MPI_Bcast(bufferRcv, 3, MPI_INT, 0, parent);
//jugar lances: empiezo yo
    } else {
        MPI_Send(jugadorMano, 1, MPI_INT, 0, 0, parent);
        MPI_Send(siguienteJugador, 1, MPI_INT, 0, 0, parent);
        MPI_Send(turno, 1, MPI_INT, 0, 0, parent);
        MPI_Bcast(bufferRcv, 3, MPI_INT, 0, parent);
    }
}

void marcarDescarte(Carta *wMazo, int sizeMazo, int id) {
    int i;
    for (i = 0; i <= sizeMazo - 1; i++) {
        if (wMazo[i].id = id) {
            wMazo[i].estado = 2;
        }
    }
}

int *preparaPares(int equivalencias[], int *pares) {
/* PARES */
// pares[5]; /*primera posición: duplesIguales, 1 entero*/
/*segunda posición: medias, 1 entero */
/*tercera posición: duples parejas y pareja, 3 enteros */

    int duplesIguales = 99; //99 significa no hay duples; cualquier otro valor, es el orden de la carta de la que si hay
    int medias = 99; //99 significa no hay medias; cualquier otro valor, es el orden de la carta de la que si hay


    int *parejas = (int *) malloc(3 * sizeof(int));

    parejas = uniquePairs(equivalencias, N_CARTAS_MANO, 4);

    if (parejas[0] > 0) {
        duplesIguales = parejas[1];
//printf("Duples de la misma carta: %s\n", caras[duplesIguales]);
    }

    parejas = uniquePairs(equivalencias, N_CARTAS_MANO, 3);
    if (parejas[0] > 0) {
        medias = parejas[1];
/*printf("MEDIAS DE: %s\n", caras[medias]);*/
    }

    parejas = uniquePairs(equivalencias, N_CARTAS_MANO, 2);

    pares[0] = duplesIguales;
    pares[1] = medias;
    pares[2] = parejas[0];
    pares[3] = parejas[1];
    pares[4] = parejas[2];
}
