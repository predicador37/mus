/* C Example */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <time.h>
#include "mus.h"


#define N_CARTAS_MAZO 40
#define N_CARTAS_MANO 4
#define N_JUGADORES 4

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
    printf("Fin del contenido del mazo/mano.\n");
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
    printf("Mazo barajado.\n");

} /* fin funcion barajar */

/* FUNCION cortarMazo: corta el mazo, esto es, saca una carta aleatoria del mazo */

void cortarMazo(Carta *wMazo, char **paloCorte) {

    int r; /* índice aleatorio para el mazo*/
    r = rand() % (N_CARTAS_MAZO + 1 - 0) + 0;
    //r = M + rand() / (RAND_MAX / (N - M + 1) + 1);

    *paloCorte = wMazo[r].palo;

}

void printCartaById(Carta *wMazo, int id) {
    int i = 0;
    for (i = 0; i <= N_CARTAS_MAZO - 1; i++) {
        if (wMazo[i].id == id) {
            printf("El valor de %-8s\t de \t%-8s es \t%d \tcon orden \t%d y estado %d\n \t", wMazo[i].cara,
                   wMazo[i].palo, wMazo[i].valor, wMazo[i].orden, wMazo[i].estado);
            printf("\n");
        }
    }
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

void enviarMazo(Carta *wMazo, int proceso, MPI_Comm wComm, int nCartas) {

    int j = 0;
    for (j = 0; j < nCartas; j++) {
        MPI_Send(&wMazo[j].id, 1, MPI_INT, proceso, 0, wComm);
        MPI_Send(&wMazo[j].valor, 1, MPI_INT, proceso, 0, wComm);
        MPI_Send(&wMazo[j].equivalencia, 1, MPI_INT, proceso, 0, wComm);
        MPI_Send(&wMazo[j].orden, 1, MPI_INT, proceso, 0, wComm);
        MPI_Send(&wMazo[j].estado, 1, MPI_INT, proceso, 0, wComm);
        MPI_Send(wMazo[j].palo, 7, MPI_CHAR, proceso, 0, wComm);
        MPI_Send(wMazo[j].cara, 8, MPI_CHAR, proceso, 0, wComm);
    }
}


void recibirMazo(Carta *wMazo, int proceso, MPI_Comm wComm, int nCartas, MPI_Status *stat) {

    int i = 0;
    for (i = 0; i < nCartas; i++) {
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

    for (i = 0; i < longitud; i++) {
        if (max < array[i]) {
            max = array[i];
        }
    }
    return max;
}

int maximoArrayExcluyendo(int array[], int longitud, int excluido) {
    int i = 0;
    int max = array[0];

    for (i = 0; i < longitud; i++) {
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

    for (i = longitud - 1; i >= 0; i--) //increment a and decrement b until they meet each other
    {
        dest[j] = orig[i];
        j++;
    }
}


int calculaGrande(int rbuf[], int jugadorMano) {

    int empates[4];
    int i, k = 0;

    for (i = 0; i < 4; i++) {
        empates[i] = 0;
    }
    int ganador;
    for (k = 0; k < 10; k++) {
        if (k == 0) { /* se buscan reyes y treses*/
            int suma[4];
            suma[0] = rbuf[0] + rbuf[7];
            suma[1] = rbuf[10] + rbuf[17];
            suma[2] = rbuf[20] + rbuf[27];
            suma[3] = rbuf[30] + rbuf[37];
            int maximo = maximoArray(suma, 4);
            int ocurrencias = ocurrenciasArray(suma, 4, maximo);
            if (ocurrencias == 1) { //el jugador gana porque tiene más reyes
                ganador = buscaIndice(suma, 4, maximo);
                break;
            }
            else if (ocurrencias != 0) {
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
            int maximo = maximoArray(suma, 4);
            if (maximo != 0) {
                int ocurrencias = ocurrenciasArray(suma, 4, maximo);
                if (ocurrencias == 1 && empates[buscaIndice(suma, 4, maximo)] == 1) {
                    ganador = buscaIndice(suma, 4, maximo);
                    break;
                }
                else if (ocurrencias != 0) {
                    for (i = 0; i < 4; i++) {
                        if ((suma[i] == maximo) && (empates[i] == 1)) {
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
            if (ocurrencias == 1) { //el jugador gana porque tiene mejores cartas
                ganador = buscaIndice(suma, 4, maximo);
                break;
            }
            else if (ocurrencias != 0) { //gana la mano o el más cercano
                printf("Se deshace empate con distancia a la mano...\n");
                ganador = deshacerEmpate(suma, jugadorMano, 1);
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
            int suma[4];
            suma[0] = rbufInv[0] + rbufInv[1];
            suma[1] = rbufInv[10] + rbufInv[11];
            suma[2] = rbufInv[20] + rbufInv[21];
            suma[3] = rbufInv[30] + rbufInv[31];
            int maximo = maximoArray(suma, 4);
            int ocurrencias = ocurrenciasArray(suma, 4, maximo);
            if (ocurrencias == 1) { //el jugador gana porque tiene más Ases
                ganador = buscaIndice(suma, 4, maximo);
                break;
            }
            else if (ocurrencias != 0) {
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

        // if we spotted number just 2 times, increment result
        if (c == repeticion) {
            res[0]++;

            if (res[1] == 99) {
                res[1] = num;
            }
            else {
                res[2] = num;
            }

        }
    }

    return res;
}

int calcularPares(int paresBuf[], int jugadorMano) {
    /* pares */
    /* parámetros: paresBuf */
    /* devuelve entero con proceso ganador */

    //TODO parametrizar las constantes

    int jugadores[4];
    int valoresPares[4];
    int ganador = 99;

    int i = 0;

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
            printf("Se deshace empate con distancia a la mano...\n");
            ganador = deshacerEmpateComplementario(jugadores, jugadorMano, 99);
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
                    printf("Se deshace empate con distancia a la mano...\n");
                    ganador = deshacerEmpate(jugadores, jugadorMano, 2);
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
                printf("Se deshace empate con distancia a la mano...\n");
                ganador = deshacerEmpateComplementario(jugadores, jugadorMano, 99);
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
            else { // otra vez empates
                printf("Se deshace empate con distancia a la mano...\n");
                ganador = deshacerEmpate(jugadores, jugadorMano, 1);
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

int calcularJuego(int juegoBuf[], int jugadorMano) {
    /* JUEGO */
    printf("Calculando juego...\n");
    int ocurrencias = 0;
    int ganador = 99;
    int i = 0;
    int juegosGanadores[8] = {31, 32, 40, 37, 36, 35, 34, 33};

    ocurrencias = ocurrenciasArray(juegoBuf, 4, juegosGanadores[0]);
    if (ocurrencias == 1) {
        ganador = buscaIndice(juegoBuf, 4, juegosGanadores[0]);
        return ganador;
    }
    else if (ocurrencias > 1) {// empates: gana la mano o el que esté más cerca
        for (i = 0; i < N_JUGADORES; i++) {
            printf("Se deshace empate con distancia a la mano...\n");
            ganador = deshacerEmpate(juegoBuf, jugadorMano, i);
        }
    }
    ocurrencias = ocurrenciasArray(juegoBuf, 4, juegosGanadores[1]);
    if (ocurrencias == 1) {
        ganador = buscaIndice(juegoBuf, 4, juegosGanadores[1]);
        return ganador;
    }
    else if (ocurrencias > 1) {// empates: gana la mano o el que esté más cerca
        printf("Se deshace empate con distancia a la mano...\n");
        ganador = deshacerEmpate(juegoBuf, jugadorMano, i);
    }

    ocurrencias = ocurrenciasArray(juegoBuf, 4, juegosGanadores[2]);
    if (ocurrencias == 1) {
        ganador = buscaIndice(juegoBuf, 4, juegosGanadores[2]);
        return ganador;
    }
    else if (ocurrencias > 1) {// empates: gana la mano o el que esté más cerca
        printf("Se deshace empate con distancia a la mano...\n");
        ganador = deshacerEmpate(juegoBuf, jugadorMano, i);
    }

    for (i = 37; i >= 33; i--) {
        ocurrencias = ocurrenciasArray(juegoBuf, 4, i);
        if (ocurrencias == 1) {
            ganador = buscaIndice(juegoBuf, 4, i);
            return ganador;
        }
        else if (ocurrencias > 1) {// empates: gana la mano o el que esté más cerca
            printf("Se deshace empate con distancia a la mano...\n");
            ganador = deshacerEmpate(juegoBuf, jugadorMano, i);
        }
    }


    if (ganador == 99) { // al punto
        for (i = 30; i >= 0; i--) {
            ocurrencias = ocurrenciasArray(juegoBuf, 4, i);

            if (ocurrencias == 1) {
                ganador = buscaIndice(juegoBuf, 4, i);
                return ganador;
            }
            else if (ocurrencias > 1) { //empates
                printf("Se deshace empate con distancia a la mano...\n");
                return deshacerEmpate(juegoBuf, jugadorMano, i);
            }
        }
    }
    return ganador;
}

int deshacerEmpate(int *conteos, int jugadorMano, int valor) {
    int j = 0;
    for (j = 0; j < N_JUGADORES; j++) {
        if (conteos[j] == valor && j == jugadorMano) {
            return j;
        }
        else if (conteos[j] == valor && j == add_mod(jugadorMano, 1, 4)) {
            return j;
        }
        else if (conteos[j] == valor && j == add_mod(jugadorMano, 2, 4)) {
            return j;
        }
    }
    return 99;
}

int deshacerEmpateComplementario(int *conteos, int jugadorMano, int valor) {
    int j = 0;
    for (j = 0; j < N_JUGADORES; j++) {
        if (conteos[j] != valor && j == jugadorMano) {
            return j;
        }
        else if (conteos[j] != valor && j == add_mod(jugadorMano, 1, 4)) {
            return j;
        }
        else if (conteos[j] != valor && j == add_mod(jugadorMano, 2, 4)) {
            return j;
        }

    }
    return 99;
}

int tengoJuego(int suma) {
    int juegosGanadores[8] = {31, 32, 40, 37, 36, 35, 34, 33};
    if (ocurrenciasArray(juegosGanadores, 8, suma) == 1) {
        //hay juego
        return 1;
    }
    else {
        return 0;
    }
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
    if ((paresBuf[0] == 1) || paresBuf[1] == 1 || paresBuf[2] == 2 || paresBuf[2] == 1) {

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
        //printf("[jugador %d] CORTO MUS!!\n", *rank);
        *jugadorMano = *rank;
        MPI_Send(jugadorMano, 1, MPI_INT, 0, 0, parent);
        MPI_Send(siguienteJugador, 1, MPI_INT, 0, 0, parent);
        MPI_Send(turno, 1, MPI_INT, 0, 0, parent);
        MPI_Bcast(bufferRcv, 3, MPI_INT, 0, parent);
//jugar lances: empiezo yo
    } else {
        *jugadorMano = 99;
        MPI_Send(jugadorMano, 1, MPI_INT, 0, 0, parent);
        MPI_Send(siguienteJugador, 1, MPI_INT, 0, 0, parent);
        MPI_Send(turno, 1, MPI_INT, 0, 0, parent);
        MPI_Bcast(bufferRcv, 3, MPI_INT, 0, parent);
    }
}

void marcarDescarte(Carta *wMazo, int sizeMazo, int id) {
    int i;
    for (i = 0; i <= sizeMazo - 1; i++) {
        if (wMazo[i].id == id) {
            wMazo[i].estado = 2;
        }
    }
}

void *preparaPares(int equivalencias[], int *pares) {
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

    }

    parejas = uniquePairs(equivalencias, N_CARTAS_MANO, 3);
    if (parejas[0] > 0) {
        medias = parejas[1];

    }

    parejas = uniquePairs(equivalencias, N_CARTAS_MANO, 2);

    pares[0] = duplesIguales;
    pares[1] = medias;
    pares[2] = parejas[0];
    pares[3] = parejas[1];
    pares[4] = parejas[2];
}

//devuelve apuestas del lance
int calcularEnvite(int *envites, int *enviteAnterior, int jugadorMano, int *piedras) {

    int envitesPostre[4];
    int envitesMano[4];
    int envitePostre[2];
    int soloEnvitesPostre[2];
    int soloEnvitesMano[2];
    int enviteMano[2];

    envitesPostre[0] = envites[add_mod(jugadorMano, 1, 4) * 2];
    envitesPostre[1] = envites[add_mod(jugadorMano, 1, 4) * 2 + 1];
    envitesPostre[2] = envites[add_mod(jugadorMano, 3, 4) * 2];
    envitesPostre[3] = envites[add_mod(jugadorMano, 3, 4) * 2 + 1];

    envitesMano[0] = envites[(jugadorMano) * 2];
    envitesMano[1] = envites[(jugadorMano) * 2 + 1];
    envitesMano[2] = envites[add_mod(jugadorMano, 2, 4) * 2];
    envitesMano[3] = envites[add_mod(jugadorMano, 2, 4) * 2 + 1];

    soloEnvitesPostre[0] = envitesPostre[0];
    soloEnvitesPostre[1] = envitesPostre[2];

    int maximoPostre = maximoArray(soloEnvitesPostre, 2);
    envitePostre[0] = envitesPostre[buscaIndice(soloEnvitesPostre, 2, maximoPostre) * 2];
    envitePostre[1] = envitesPostre[buscaIndice(soloEnvitesPostre, 2, maximoPostre) * 2 + 1];

    soloEnvitesMano[0] = envitesMano[0];
    soloEnvitesMano[1] = envitesMano[2];
    int maximoMano = maximoArray(soloEnvitesMano, 2);
    enviteMano[0] = envitesMano[buscaIndice(soloEnvitesMano, 2, maximoMano) * 2];
    enviteMano[1] = envitesMano[buscaIndice(soloEnvitesMano, 2, maximoMano) * 2 + 1];

    if (envites[jugadorMano * 2] != 0) { //la mano ha envidado

        if (enviteAnterior[0] > envitePostre[0]) { // pareja postre no iguala la apuesta

            piedras[1]++; // la pareja mano gana una piedra
            printf("[maestro]: mano gana una piedra, en total:%d\n", piedras[1]);
            return 0;
        }
        if (enviteAnterior[0] == envitePostre[0]) { //pareja postre iguala mano
            printf("HAY APUESTA\n");
            return enviteAnterior[0]; //se devuelve la cantidad apostada, 2 o 5 en caso de esta logica
        }
        if (enviteAnterior[0] < envitePostre[0]) { //jugador mano no iguala la apuesta
            if (enviteMano[0] > envitePostre[0]) {
                // pareja postre no iguala la apuesta
                piedras[1]++; //uno por el no
                piedras[1] += envitePostre[0]; // lo que tuviera envidado la pareja postre
                return 0; //no hay apuestas
            }
            else if (enviteMano[0] == envitePostre[0]) {
                // pareja mano iguala la apuesta
                return envitePostre[0];
            }
            else if (enviteMano[0] < envitePostre[0]) {
                piedras[0]++;
                return 0;
            }

        }

    }
    else if (envites[jugadorMano * 2] == 0) { //la mano no ha envidado)
        printf("MANO NO ENVIDA\n");
        printf("ENVITE POSTRE: %d\n", envitePostre[0]);
        if (envitePostre[0] != 0) { // pareja postre si ha envidado
            printf("POSTRE HA ENVIDADO\n");
            if (envitePostre[0] > enviteMano[0]) { // pareja mano no acepta envite

                piedras[0]++; //por el no
                printf("POSTRE SE LLEVA PIEDRA, en total: %d\n", piedras[0]);
                return 0;
            }
            else if (envitePostre[0] == enviteMano[0]) { // pareja mano acepta la apuesta
                return envitePostre[0];
            }
            else if (envitePostre[0] < enviteMano[1]) { // pareja mano sube apuesta y postre no la iguala
                printf("MANO SE LLEVA APUESTA DESPUES DE RAJARSE POSTRE\n");
                piedras[1]++;
                piedras[1] += envitePostre[0];
                return 0;
            }
        }
        else if ((envitePostre[0] == 0) && (enviteMano[0] != 0)) { //envida pareja de la mano y no se la aceptan
            piedras[1]++;
            return 0;
        }
        else if ((envitePostre[0] == 0) && (enviteMano[0] == 0)) { //nadie envida
            // no hay piedras ni apuestas
            printf("[maestro] NO HAY PIEDRAS NI APUESTAS\n");
            return 0;
        }
    }
    return 0;
}

int envido(int *equivalencias, int longitud, int lance, int apuestaVigor) {

    if (ordago() == 1) { // ordago!
        return 99;
    }

    else if (lance == 0) { // a grande
        int reyes = ocurrenciasArray(equivalencias, longitud, 10);
        if (reyes >= 3) {
            return 5;
        }
        else if ((reyes == 2) && (apuestaVigor <= 2)) {
            return 2;
        }
        else {
            return 0;
        }
    }
    else if (lance == 1) { // a chica
        int ases = ocurrenciasArray(equivalencias, longitud, 1);
        if (ases >= 3) {
            return 5;
        }
        else if ((ases == 2) && (apuestaVigor <= 2)) {
            return 2;
        }
        else {
            return 0;
        }
    }
    else if (lance == 2) { // a pares
        int reyes = ocurrenciasArray(equivalencias, longitud, 10);
        if (reyes >= 3) { //duples o medias de reyes
            return 5;
        }
        else if ((reyes == 2) && (apuestaVigor <= 2)) { // pareja de reyes
            return 2;
        }
        else { // pareja de otra cosa
            return 0;
        }

    }
    else if (lance == 3) { // a juego
        int suma = sumaArray(equivalencias, 4);
        if (suma == 31) {
            return 5;
        }
        else if ((suma == 32) && (apuestaVigor <= 2)) {
            return 2;
        }
        else {
            return 0;
        }

    }

    else if (lance == 4) { // al punto
        int suma = sumaArray(equivalencias, 4);
        if (suma >= 27) {
            return 5;
        }
        else if ((suma >= 24) && (suma < 27) && (apuestaVigor <= 2)) {
            return 2;
        }
        else {
            return 0;
        }

    }
    else {
        return 0;
    }
    return 0;
}

int queParejaSoy(int rank, int jugadorMano) { //1 mano, 0 postre
    if (rank == jugadorMano) {
        return 1;
    }
    else if (add_mod(jugadorMano, 1, 4) == rank) {
        return 0;
    }
    else if (add_mod(jugadorMano, 2, 4) == rank) {
        return 1;
    }
    else if (add_mod(jugadorMano, 3, 4) == rank) {
        return 0;
    }
    else return 99;
}

int enQueParejaEstoy(int rank) {
    if ((rank == 0) || (rank == 2)) {
        return 1;
    }
    else {
        return 2;
    }
}


int ordago() {
    srand(time(0));
    double r = (double) rand() / (double) RAND_MAX;
    if (r < 0.98) {
        return 0;
    }
    else {
        return 1;
    }
}

void clearInputBuffer() // works only if the input buffer is not empty
{
    char c;
    do {
        c = getchar();
    } while (c != '\n' && c != EOF);
}