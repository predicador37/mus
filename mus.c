//
// Created by predicador on 15/06/16.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <time.h>
#include "mus.h"
#include "dbg.h"

#define N_CARTAS_MAZO 40
#define N_CARTAS_MANO 4
#define N_JUGADORES 4
#define N_PALOS 4
#define DEBUG 0
#define CHAR_BUFFER 8

#define RESET   "\033[0m"
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

typedef int bool;
#define true 1
#define false 0

const char * caras[] = {"As", "Dos", "Tres", "Cuatro", "Cinco",
                        "Seis", "Siete", "Sota", "Caballo", "Rey"};
const char * palos[] = {"Oros", "Copas", "Espadas", "Bastos"};
const char * lances_etiquetas[] = {"Grande", "Chica", "Pares", "Juego", "Al punto"};
const char * parejas[] = {"Postre", "Mano"};
const char * respuestas[] = {"No", "Sí"};
const char * parejas_etiquetas[] = {"Pareja 0-2", "Pareja 1-3"};
int valores[] = {1, 1, 10, 4, 5, 6, 7, 10, 10, 10};
int equivalencias[] = {1, 1, 10, 4, 5, 6, 7, 8, 9, 10};

int rand_lim(int limit) {
/* devuelve un número aleatorio entre 0 y limit incluido
 */

    int divisor = RAND_MAX/(limit+1);
    int retval;

    do {
        retval = rand() / divisor;
    } while (retval > limit);

    return retval;
}


/* Puebla un array de estructuras Carta con sus valores y palos*/
int crear_mazo(Carta *mazo) {
    int i; /* contador */
    int size_mazo = 0;

    /* iterar el mazo */
    for (i = 0; i <= N_CARTAS_MAZO - 1; i++) {
        mazo[i].cara = i % 10;
        mazo[i].palo = i / 10;
        mazo[i].id = i;
        mazo[i].orden = i % 10;
        mazo[i].estado = 0; //0: en mazo, 1: repartida, 2: descartada
        size_mazo++;
    } /* fin for */
    return size_mazo;

} /* fin funcion crearMazo */



/* Muestra por pantalla un mazo de cartas */
void print_mazo(Carta *wMazo, int size_mazo) {
    int i;
    for (i = 0; i <= size_mazo - 1; i++) {
        printf("El valor de %-8s\t de \t%s es \t%d \tcon id \t%d, estado %d y cara %d\n \t", caras[wMazo[i].cara],
               palos[wMazo[i].palo], valores[wMazo[i].cara], wMazo[i].id, wMazo[i].estado, wMazo[i].cara);
        printf("\n");
    }
    printf("Fin del contenido del mazo/mano.\n");
}

/* Muestra en la consola un vector de estados de las cartas de un mazo o mano */
void print_vector_estados(Carta *wMazo, int size_mazo) {
    int i;
    for (i = 0; i <=
            size_mazo - 1; i++) {
        printf("%d::%d ", i, wMazo[i].id);
    }
    printf("\n");
    for (i = 0; i <= size_mazo - 1; i++) {
        printf("%d::%d ", i, wMazo[i].estado);
    }
    printf("\n");
}

/* Baraja las cartas del mazo*/

void barajar_mazo(Carta *wMazo) {
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

}

/* Cambia el estado de las cartas de un mazo de descartadas a en el mazo*/
int poner_descartadas_en_mazo(Carta *wMazo) {
    int i;
    int contador = 0;
    for (i = 0; i <= N_CARTAS_MAZO - 1; i++) {
       if (wMazo[i].estado == 2) {
           wMazo[i].estado = 0;
           contador++;
       }
    }
    return contador;
}

/* Cuenta las cartas de un mazo en un estado determinado*/
int contar_cartas_en_estado(Carta *wMazo, int estado) {
    int i;
    int contador = 0;
    for (i = 0; i <= N_CARTAS_MAZO - 1; i++) {
        if (wMazo[i].estado == estado) {
            contador++;
        }
    }
    return contador;
}

/* Empaqueta el envío de una estructura de mazo o mano de cartas*/
void enviar_mazo(Carta *wMazo, int proceso, MPI_Comm wComm, int nCartas) {

    int j;
    for (j = 0; j < nCartas; j++) {

        MPI_Send(&wMazo[j].id, 1, MPI_INT, proceso, 0, wComm);
        MPI_Send(&wMazo[j].orden, 1, MPI_INT, proceso, 0, wComm);
        MPI_Send(&wMazo[j].estado, 1, MPI_INT, proceso, 0, wComm);
        MPI_Send(&wMazo[j].palo, 1, MPI_INT, proceso, 0, wComm);
        MPI_Send(&wMazo[j].cara, 1, MPI_INT, proceso, 0, wComm);
    }
}

/* Empaqueta la reecepción de una estructura de mazo o mano de cartas*/
void recibir_mazo(Carta *wMazo, int proceso, MPI_Comm wComm, int nCartas, MPI_Status *stat) {

    int i;
    for (i = 0; i < nCartas; i++) {

        MPI_Recv(&wMazo[i].id, 1, MPI_INT, proceso, 0, wComm, stat);
        MPI_Recv(&wMazo[i].orden, 1, MPI_INT, proceso, 0, wComm, stat);
        MPI_Recv(&wMazo[i].estado, 1, MPI_INT, proceso, 0, wComm, stat);
        MPI_Recv(&wMazo[i].palo, 1, MPI_INT, proceso, 0, wComm, stat);
        MPI_Recv(&wMazo[i].cara, 1, MPI_INT, proceso, 0, wComm, stat);
    }
}

/* Realiza una suma modular */
int add_mod(int a, int b, int m) {
    if (0 == b) return a;

    // return sub_mod(a, m-b, m);
    b = m - b;
    if (a >= b)
        return a - b;
    else
        return m - b + a;
}

/* Empaqueta el envío de una estructura de tipo carta */
void repartir_carta(Carta wCarta, int proceso, MPI_Comm wComm) {

    MPI_Send(&wCarta.id, 1, MPI_INT, proceso, 0, wComm);
    MPI_Send(&wCarta.orden, 1, MPI_INT, proceso, 0, wComm);
    MPI_Send(&wCarta.estado, 1, MPI_INT, proceso, 0, wComm);
    MPI_Send(&wCarta.palo, 1, MPI_INT, proceso, 0, wComm);
    MPI_Send(&wCarta.cara, 1, MPI_INT, proceso, 0, wComm);

}

/* Empaqueta la recepción de una estructura de tipo carta */
Carta recibir_carta(int proceso, MPI_Comm wComm, MPI_Status *stat) {
    Carta wCarta;

    MPI_Recv(&wCarta.id, 1, MPI_INT, proceso, 0, wComm, stat);
    MPI_Recv(&wCarta.orden, 1, MPI_INT, proceso, 0, wComm, stat);
    MPI_Recv(&wCarta.estado, 1, MPI_INT, proceso, 0, wComm, stat);
    MPI_Recv(&wCarta.palo, 1, MPI_INT, proceso, 0, wComm, stat);
    MPI_Recv(&wCarta.cara, 1, MPI_INT, proceso, 0, wComm, stat);
    wCarta.estado = 1;
    return wCarta;
}


/* Encapsula la lógica de reparto de cartas para un jugador repartidor */
int repartidor_reparte(int rank, int repartidor,  int size_mazo, int size_descartadas,  Carta mazo[], Carta mano_cartas[], MPI_Comm parent, MPI_Status stat){

    recibir_mazo(mazo, 0, parent, N_CARTAS_MAZO, &stat);
    debug("Mazo recibido");
    /* A continuación tiene lugar el reparto de cartas secuencial*/
    /* Por este requisito no se ha utilizado scatter*/
    /* La e/s se redirige al proceso maestro */

    int i = 0;
    int j = 0;
    int k = 0;
    int buffer_reparto[3];

    int siguiente_jugador = repartidor;
    debug("Siguiente jugador inicial: %d", siguiente_jugador);
    for (i = 0; i < N_CARTAS_MANO; i++) {
        for (j = 0; j < N_JUGADORES; j++) { /* En total se reparten 16 cartas */

            siguiente_jugador = add_mod(siguiente_jugador, 1, 4);
            buffer_reparto[0] = i; //por qué número de carta de la mano (total de 4) se va repartiendo
            buffer_reparto[1] = siguiente_jugador; // a qué jugador se reparte

            if (siguiente_jugador != repartidor) {
                mazo[k].estado = 1; // la carta pasa a estado repartida
                repartir_carta(mazo[k], siguiente_jugador, MPI_COMM_WORLD);
                MPI_Send(&buffer_reparto, 2, MPI_INT, 0, 0, parent);

            }
            else {
                /* Para repartirse a sí mismo no tiene sentido utilizar MPI */
                MPI_Send(&buffer_reparto, 2, MPI_INT, 0, 0, parent);
                mazo[k].estado = 1;// la carta pasa a estado repartida
                mano_cartas[i] = mazo[k];
                buffer_reparto[0] =  i; //por qué número de carta de la mano (total de 4) se va repartiendo
                buffer_reparto[1] = rank; // a qué jugador se reparte: en este caso, a uno mismo (repartidor)
                buffer_reparto[2] = valores[mano_cartas[i].cara]; // qué valor tiene la carta repartida
                MPI_Send(&buffer_reparto, 3, MPI_INT, 0, 0, parent);
            }
            size_mazo--; /* es necesario almacenar el tamaño del mazo después de repartir */
            size_descartadas++;
            k++; /* un contador auxiliar para recuperar cartas del mazo */
        }
    }
    return size_mazo;

}

/* Encapsula la lógica de recepción de cartas por parte de un jugador */
void jugador_recibe_cartas(int rank, int repartidor, Carta mano_cartas[],  MPI_Comm parent, MPI_Status *stat){
    int i = 0;
    int buffer_reparto[3];

    for (i = 0; i < N_CARTAS_MANO; i++) {

        mano_cartas[i] = recibir_carta(repartidor, MPI_COMM_WORLD, stat);
        buffer_reparto[0] = rank;
        buffer_reparto[1] = i;
        buffer_reparto[2] = valores[mano_cartas[i].cara];
        MPI_Send(&buffer_reparto, 3, MPI_INT, 0, 0, parent);
    }
}

/* Devuelve el número de cartas con una cara determinada en un mazo o mano */
int cuenta_cartas_mano(Carta *wMano, int cara) {
    int i = 0;
    int cuenta = 0;
    for (i = 0; i < N_CARTAS_MANO; i++) {
        if (wMano[i].cara == cara) {
            cuenta++;
        }
    }
    return cuenta;
}

/* Devuelve el máximo de un array de enteros de longitud arbitraria*/
int maximo_array(int array[], int longitud) {
    int i = 0;
    int max = array[0];

    for (i = 0; i < longitud; i++) {
        if (max < array[i]) {
            max = array[i];
        }
    }
    return max;
}

/* Devuelve el máximo de un array de enteros de longitud arbitraria excluyendo un número dado*/
int maximo_array_excluyendo(int array[], int longitud, int excluido) {
    int i = 0;
    int max = array[0];

    for (i = 0; i < longitud; i++) {
        if (max < array[i] && array[i] != excluido) {
            max = array[i];
        }
    }
    return max;
}

/* Devuelve el número de ocurrencias de un valor determinado en un array de enteros*/
int ocurrenciasArray(int array[], int longitud, int numero) {
    int n = 0;
    int i = 0;
    for (i = 0; i < longitud; i++) {
        if (numero == array[i])
            n++;
    }
    return n;
}

/* Devuelve el índice de la posición en la que se encuentra un valor dado en un array de enteros*/
int buscaIndice(int a[], int longitud, int numero) {
    int index = 0;

    while (index < longitud && a[index] != numero) ++index;

    return (index == longitud ? -1 : index);
}

/* Devuelve el índice de la posición en la que se encuentra un valor distinto de uno dado en un array de enteros*/
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

/* Devuelve el ganador a grande dadas las manos de los jugadores y el jugador mano*/
int calcula_grande(int rbuf[], int jugadorMano) {

    int empates[4] = {0,0,0,0};
    int i, k = 0;

    int ganador = 999;
    for (k = 0; k < 10; k++) {
        if (k == 0) { /* se buscan reyes y treses*/
            int suma[4];
            suma[0] = rbuf[0] + rbuf[7];
            suma[1] = rbuf[10] + rbuf[17];
            suma[2] = rbuf[20] + rbuf[27];
            suma[3] = rbuf[30] + rbuf[37];
            int maximo = maximo_array(suma, 4);
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
            int maximo = maximo_array(suma, 4);
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
            int maximo = maximo_array(suma, 4);
            int ocurrencias = ocurrenciasArray(suma, 4, maximo);
            if (ocurrencias == 1) { //el jugador gana porque tiene mejores cartas
                ganador = buscaIndice(suma, 4, maximo);
                break;
            }
            else if (ocurrencias != 0) { //gana la mano o el más cercano
                printf("Se deshace empate con distancia a la mano...\n");

                for (i = 0; i < 4; i++) {
                    if ((suma[i] == maximo) && (empates[i] == 1)) {
                        empates[i] = 1;
                    }

                }

                if (ocurrenciasArray(empates, 4, 1) == 1) {
                    debug("DESEMPATE: sólo queda un jugador en vector de empates\n");
                    ganador = buscaIndice(suma, 4, 1);
                    break;
                }
                else {
                    ganador = deshacerEmpate(empates, jugadorMano, 1);
                }
                break;
            }
        }

    }
    if (ganador >5) {
        printf("[maestro] ERROR en funcion calcula_grande\n");
    }
    return ganador;
}
/*Calcula el jugador ganador a chica, recibiendo conteos de cartas de todos los jugadores*/
int calcula_chica(int rbufInv[], int jugadorMano) {

    int empates[4] = {0,0,0,0};
    int i, k = 0;


    int ganador = 999;
    for (k = 0; k < 10; k++) {
        if (k == 0) { /* se buscan Ases y doses*/
            int suma[4];
            suma[0] = rbufInv[0] + rbufInv[1];
            suma[1] = rbufInv[10] + rbufInv[11];
            suma[2] = rbufInv[20] + rbufInv[21];
            suma[3] = rbufInv[30] + rbufInv[31];
            int maximo = maximo_array(suma, 4);
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
            int maximo = maximo_array(suma, 4);
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
            int maximo = maximo_array(suma, 4);
            int ocurrencias = ocurrenciasArray(suma, 4, maximo);
            if (ocurrencias == 1) { //el jugador gana porque tiene más Ases
                ganador = buscaIndice(suma, 4, maximo);
                break;
            }
            else if (ocurrencias != 0) { //gana la mano o el más cercano
                printf("Se deshace empate con distancia a la mano...\n");

                for (i = 0; i < 4; i++) {
                    if ((suma[i] == maximo) && (empates[i] == 1)) {
                        empates[i] = 1;
                    }

                }

                if (ocurrenciasArray(empates, 4, 1) == 1) {
                   debug("DESEMPATE: sólo queda un jugador en vector de empates\n");
                    ganador = buscaIndice(suma, 4, 1);
                    break;
                }
                else {
                    ganador = deshacerEmpate(empates, jugadorMano, 1);
                }
                break;
            }
        }

    }
    if (ganador >5) {
        printf("[maestro] ERROR en funcion calcula_chica\n");
    }
    return ganador;
}

int cmpfunc(const void *a, const void *b) {
    return (*(int *) a - *(int *) b);
}
/* Encuentra parejas únicas en un array de longitud. Está preparado para una mano */
/* Devuelve array con 3 elementos: el número de parejas y las cartas asociadas */
void unique_pairs(int *array, int longitud, int repeticion, int parejas[]) {
    qsort(array, longitud, sizeof(int), cmpfunc);

    int k = 0;
    for (k = 0; k < longitud; k++) {
    }

   // int res[3] = {0, 99, 99}; //99 es valor fuera de rango para una carta
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
            parejas[0]++;

            if (parejas[1] == 99) {
                parejas[1] = num;
            }
            else {
                parejas[2] = num;
            }

        }
    }

    //return res;
}

/* Devuelve el ganador a pares dadas las parejas encontradas en las cartas de los jugadores y el jugador mano*/
int calcular_pares(int paresBuf[], int jugadorMano) {
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
    int ocurrencias = ocurrenciasArray(jugadores, 4, 99); //se cuentan jugadores que no tienen duples
    if (ocurrencias == 3) { //hay un proceso con duples
        ganador = buscarIndiceNumeroNoIgual(jugadores, 4, 99);
    }

    else { // hay empates o nadie tiene duples
        int maximo = 99;
        maximo = maximo_array_excluyendo(jugadores, 4, 99); //Cual es el máximo excluyendo 99
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
            int maximo = maximo_array(valoresPares, 4);
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
                int maximo = maximo_array(valoresPares, 4);
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
            int maximo = maximo_array_excluyendo(jugadores, 4, 99);
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
            int maximo = maximo_array(valoresPares, 4);

            ocurrencias = ocurrenciasArray(valoresPares, 4, maximo);
            if (ocurrencias == 1) { //el jugador gana porque tiene cartas más altas
                ganador = buscaIndice(valoresPares, 4, maximo);
            }
            else { // otra vez empates
                int empates[N_JUGADORES]={0,0,0,0};
                for (i=0;i<N_JUGADORES;i++){
                    if (valoresPares[i]==maximo){
                        empates[i]=1;

                    }

                }
                printf("Se deshace empate con distancia a la mano...\n");
                ganador = deshacerEmpate(empates, jugadorMano, 1);
            }
        }
    }
    return ganador;
}

/* DEvuelve la suma de los valores de un array de enteros de longitud dada */
int sumaArray(int a[], int longitud) {
    int i, sum = 0;
    for (i = 0; i < longitud; i++) {
        sum = sum + a[i];
    }
    return (sum);
}

/* Devuelve el ganador a juego dados los juegos de los cuatro jugadores y el jugador mano */
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
            ganador = deshacerEmpate(juegoBuf, jugadorMano,juegosGanadores[0]);
            return ganador;
        }
    }
    ocurrencias = ocurrenciasArray(juegoBuf, 4, juegosGanadores[1]);
    if (ocurrencias == 1) {
        ganador = buscaIndice(juegoBuf, 4, juegosGanadores[1]);
        return ganador;
    }
    else if (ocurrencias > 1) {// empates: gana la mano o el que esté más cerca
        printf("Se deshace empate con distancia a la mano...\n");
        ganador = deshacerEmpate(juegoBuf, jugadorMano, juegosGanadores[1]);
        return ganador;
    }

    ocurrencias = ocurrenciasArray(juegoBuf, 4, juegosGanadores[2]);
    if (ocurrencias == 1) {
        ganador = buscaIndice(juegoBuf, 4, juegosGanadores[2]);
        return ganador;
    }
    else if (ocurrencias > 1) {// empates: gana la mano o el que esté más cerca
        printf("Se deshace empate con distancia a la mano...\n");
        ganador = deshacerEmpate(juegoBuf, jugadorMano, juegosGanadores[2]);
        return ganador;
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
            return ganador;
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

/* Deshace un empate en una jugada dados los conteos de valores en función de la distancia a la mano por la derecha*/
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

/* Deshace un empate en una jugada dados los conteos distintos a un valor en función de la distancia a la mano por la derecha*/
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

/* Prepara  un array con el tipo de pares que tiene un jugador */
void *preparaPares(int equivalencias[], int *pares) {
/* PARES */
// pares[5]; /*primera posición: duplesIguales, 1 entero*/
/*segunda posición: medias, 1 entero */
/*tercera posición: duples parejas y pareja, 3 enteros */

    int duplesIguales = 99; //99 significa no hay duples; cualquier otro valor, es el orden de la carta de la que si hay
    int medias = 99; //99 significa no hay medias; cualquier otro valor, es el orden de la carta de la que si hay

    //int *parejas = (int *) malloc(3 * sizeof(int));
    int parejas[3] = {0, 99, 99}; //99 es valor fuera de rango para una carta
    unique_pairs(equivalencias, N_CARTAS_MANO, 4, parejas);

    if (parejas[0] > 0) {
        duplesIguales = parejas[1];

    }

    unique_pairs(equivalencias, N_CARTAS_MANO, 3, parejas);
    if (parejas[0] > 0) {
        medias = parejas[1];

    }

     unique_pairs(equivalencias, N_CARTAS_MANO, 2, parejas);

    pares[0] = duplesIguales;
    pares[1] = medias;
    pares[2] = parejas[0];
    pares[3] = parejas[1];
    pares[4] = parejas[2];
}

/*Determina si un jugador tiene juego, aunque no sea decente*/
int tengoJuego(int suma) {
    int juegosGanadores[8] = {31, 32, 40, 37, 36, 35, 34, 33};
    if (ocurrenciasArray(juegosGanadores, 8, suma) == 1) {
        //hay juego
        debug("Tengo juego");
        return 1;
    }
    else {
        return 0;
    }
}

/*Determina si un jugador tiene juego decente, entendido este como 31, 32 o 40*/
int tengo_juego_decente(int suma) {
    int juegosGanadores[3] = {31, 32, 40};
    if (ocurrenciasArray(juegosGanadores, 3, suma) == 1) {
        //hay juego
        debug("Tengo juego decente");
        return 1;
    }
    else {
        return 0;
    }
}
/* Determina si un jugador tiene medias */
int tengoMedias(int *paresBuf) {
    if (paresBuf[1] != 99) {
        debug("Tengo medias");
        return 1;
    }
    else {
        return 0;
    }
}
/* Determina si un jugador tiene duples */
int tengoDuples(int *paresBuf) {

    if (paresBuf[0] != 99 || paresBuf[2] == 2) {
        debug("Tengo duples");
        return 1;
    }
    else {
        return 0;
    }
}

/* Determina si un jugador tiene pares */
int tengoPares(int *paresBuf) {
    if ((paresBuf[0] == 1) || paresBuf[1] == 1 || paresBuf[2] == 2 || paresBuf[2] == 1) {
        debug("Tengo pares");
        return 1;
    }
    else {
        return 0;
    }
}

/* Determina si un jugador corta el mus en base a sus cartas */
int cortarMus(int *valores, int *equivalencias, int *paresBuf) {

/*
 if (contrario a 5 puntos de ganar) cortar mus */
    int juego = sumaArray(valores, N_CARTAS_MANO);
    if ((juego == 31) || (tengoDuples(paresBuf) == 1) || (tengoMedias(paresBuf) == 1) ||
        (tengoPares(paresBuf) && tengo_juego_decente(juego))) {
        return 1;
    }

    else {
        return 0;
    }

}

/* Marca una carta en estado descartado */
void marcar_descarte(Carta *wMazo, int sizeMazo, int id) {
    int i;
    for (i = 0; i <= sizeMazo - 1; i++) {
        if (wMazo[i].id == id) {
            wMazo[i].estado = 2;
           debug("CARTA CON ID %d ha sido marcada con estado 2\n", id);
        }
    }
}

/* Lanza un órdago en función de una probabilidad o los puntos acumulados de la pareja contraria */
int ordago(int rank, int mano, int puntos_juego[], int n_puntos_juego) {

    srand(time(0));
    double r = (double) rand() / (double) RAND_MAX;

    int pareja_soy = que_pareja_soy(rank, mano);
    int pareja_no_soy = add_mod(pareja_soy, 1, 1);

    if (puntos_juego[pareja_no_soy] > (n_puntos_juego - 10)) { //si la otra pareja se acerca a 30 o 40, se lanza órdago
        return 1;
    }

    else if (r < 0.98) { //órdago aleatorio
        return 0;
    }
    else {
        return 1;
    }
}

/* Determina el envite de un jugador en base a sus cartas, la apuesta en vigor, la pareja en la que se encuentra, sus pares, y las piedras acumuladas por la pareja contraria*/
void envido(int envites[], int *equivalencias, int longitud, int lance, int apuesta_vigor, int jugador_mano, int rank, int pares[], int juego_al_punto, int puntos_juego[], int n_puntos_juego) {

    //si hay algun envite de la otra pareja con órdago en el lance:
      //si tengo buena mano y soy mano, acepto
      //si no, lo dejo

    if ((ordago(rank, jugador_mano, puntos_juego, n_puntos_juego) == 1) && (apuesta_vigor!=99)) {
        debug("NADIE HA LANZADO ORDAGO, LO LANZO YO: %d", rank);
        envites[0]=3; //lanzo órdago al lance que sea
        envites[1]=99;
    }


    else if (lance == 0) { // a grande
        int reyes = ocurrenciasArray(equivalencias, longitud, 10);
       debug("NÚMERO DE OCURRENCIAS DE REYES: %d\n", reyes);


         if (apuesta_vigor == 99) { //alguien ha lanzado un órdago ya
             printf("[maestro] Alguien ha lanzado el desafío...HAY UN ÓRDAGO SOBRE LA MESA!\n");
             if ((reyes >= 3) && (que_pareja_soy(rank, jugador_mano) == 1)) {
                 envites[0]=2; //se acepta el órdago
                 envites[1]=0;
             }
             else { //no se coge
                 envites[0]=1;
                 envites[1]=0;
             }
         }


        else if ((reyes >= 3) && (apuesta_vigor < 2)) { // si no hay apuestas, empieza fuerte
            envites[0] = 3;
            envites[1] = 5;
        }
        else if ((reyes>=3) && (apuesta_vigor>=2)){
            if (que_pareja_soy(rank, jugador_mano) == 1) {
                envites[0] = 3;
                envites[1] = 1;
            }
            else {
                envites[0] = 2; //si soy pareja mano, subir; si no, igualar
                envites[1] = 0;
            }
        }
        else if ((reyes == 2) && (apuesta_vigor <= 2)) {
            //con un envite y dos reyes, igualamos
            envites[0] = 2;
            envites[1] = 0;
        }
        else {
            envites[0]=1; //si no tengo cartas, paso
            envites[1]=0;
        }
    }

    else if (lance == 1) { // a chica
        int ases = ocurrenciasArray(equivalencias, longitud, 1);
         debug("NÚMERO DE OCURRENCIAS DE ASES: %d\n", ases);

         if (apuesta_vigor == 99) {
             if ((ases >= 3) && (que_pareja_soy(rank, jugador_mano) == 1)) {
                 envites[0]=2; //se acepta el órdago
                 envites[1]=0;
             }
             else {
                 envites[0]=1;
                 envites[1]=0;
             }
         }

         else if ((ases >= 3) && (apuesta_vigor < 2)) { // si no hay apuestas, empieza fuerte
             envites[0] = 3;
             envites[1] = 5;
         }
         else if ((ases>=3) && (apuesta_vigor>=2)){
             if (que_pareja_soy(rank, jugador_mano) == 1) {
                 envites[0] = 3;
                 envites[1] = 1;
             }
             else {
                 envites[0] = 2; //si soy pareja mano, subir; si no, igualar
                 envites[1] = 0;
             }
         }
         else if ((ases == 2) && (apuesta_vigor <= 2)) {
             //con un envite y dos reyes, igualamos
             envites[0] = 2;
             envites[1] = 0;
         }
         else {
             envites[0]=1; //si no tengo cartas, paso
             envites[1]=0;
         }
    }
    else if (lance == 2) { // a pares
        int reyes = ocurrenciasArray(equivalencias, longitud, 10);

         if (apuesta_vigor == 99) {
             if ((reyes >= 3) && (que_pareja_soy(rank, jugador_mano) == 1)) {
                 envites[0]=2; //se acepta el órdago
                 envites[1]=0;
             }
             else {
                 envites[0]=1;
                 envites[1]=0;
             }
         }


         else if ((reyes >= 3) && (apuesta_vigor < 2)) { // si no hay apuestas, empieza fuerte
             envites[0] = 3;
             envites[1] = 5;
         }
         else if ((reyes>=3) && (apuesta_vigor>=2)){
             if (que_pareja_soy(rank, jugador_mano) == 1) {
                 envites[0] = 3;
                 envites[1] = 1;
             }
             else {
                 envites[0] = 2; //si soy pareja mano, subir; si no, igualar
                 envites[1] = 0;
             }
         }

             //si tengo otros duples
          else if (tengoDuples(pares)==1){
             if (apuesta_vigor < 2) {
                 envites[0] = 3;
                 envites[1] = 5;
             }
             else {
                 if (que_pareja_soy(rank, jugador_mano) == 1) {
                     envites[0] = 3;
                     envites[1] = 1;
                 }
                 else {
                     envites[0] = 2; //si soy pareja mano, subir; si no, igualar
                     envites[1] = 0;
                 }
             }

         }

             //si tengo otras medias

         else if (tengoMedias(pares)==1){

                 envites[0] = 2;
                 envites[1] = 0;



         }

         else if ((reyes == 2) && (apuesta_vigor <= 2)) {
             //con un envite y dos reyes, igualamos
             envites[0] = 2;
             envites[1] = 0;
         }

         else {
             envites[0]=1; //si no tengo cartas, paso
             envites[1]=0;
         }

    }
    else if (lance == 3) { // a juego
         int suma = sumaArray(equivalencias, 4);

         if (juego_al_punto == 2) {

             if (apuesta_vigor == 99) {
                 if ((suma == 31) && (que_pareja_soy(rank, jugador_mano) == 1)) {
                     envites[0]=2; //se acepta el órdago
                     envites[1]=0;
                 }
                 else {
                     envites[0]=1;
                     envites[1]=0;
                 }
             }
             else if ((suma == 31) && (apuesta_vigor < 2)) {
                 envites[0] = 3;
                 envites[1] = 5;
             }
             else if ((suma == 31) && (apuesta_vigor >= 2)){
                 if (que_pareja_soy(rank, jugador_mano) == 1) {
                     envites[0] = 3;
                     envites[1] = 1;
                 }
                 else {
                     envites[0] = 2; //si soy pareja mano, subir; si no, igualar
                     envites[1] = 0;
                 }
             }
             else if ((suma == 32) && (apuesta_vigor <= 2)) {
                 envites[0] = 2;
                 envites[1] = 0;
             }
             else {
                 envites[0]=1; //si no tengo cartas, paso
                 envites[1]=0;
             }

         }
         else {

             if (apuesta_vigor == 99) {
                 if ((suma == 30) && (que_pareja_soy(rank, jugador_mano) == 1)) {
                     envites[0]=2; //se acepta el órdago
                     envites[1]=0;
                 }
                 else {
                     envites[0]=1;
                     envites[1]=0;
                 }
             }
             else if ((suma >= 27) && (apuesta_vigor < 2)) {
                 envites[0] = 3;
                 envites[1] = 5;
             }
             else if ((suma >= 27) && (apuesta_vigor >= 2)){
                 if (que_pareja_soy(rank, jugador_mano) == 1) {
                     envites[0] = 3;
                     envites[1] = 1;
                 }
                 else {
                     envites[0] = 2; //si soy pareja mano, subir; si no, igualar
                     envites[1] = 0;
                 }
             }
             else if ((suma >= 24) && (suma < 27) && (apuesta_vigor <= 2)) {
                 envites[0] = 2;
                 envites[1] = 0;
             }
             else {
                 envites[0]=1; //si no tengo cartas, paso
                 envites[1]=0;
             }
         }
     }

}

/* Devuelve el índice o posición de un valor dado en un array de enteros */
int busca_indice(int a[], int longitud, int numero) {
    int index = 0;

    while (index < longitud && a[index] != numero) ++index;

    return (index == longitud ? -1 : index);
}

/* Determina en qué pareja se encuentra un jugador: mano o postre */
int que_pareja_soy(int rank, int jugadorMano) { //1 mano, 0 postre
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

/*Determina en qué pareja de las iniciales se encuentra un jugador: 0: pareja_02 o 1: pareja_13 */
int que_pareja_inicial_soy(int rank) {
    if ((rank == 0 )|| (rank == 2)){
        return 0;
    }
    else {
        return 1;
    }
}


/* Determina si dos jugadores pertenecen a la misma pareja */
int misma_pareja(int rank1, int rank2) {
    if (add_mod(rank1, 2, 4) == rank2 || rank1 == rank2 ) {
        return 1;
    }
    else {
        return 0;
    }
}

/* Cuenta las ocurrencias de un entero determinado en un array  */
int contar_ocurrencias(int envites_jugadores[], int longitud, int valor) {
    int i, count=0;
    for (i=0; i<longitud; i++)
    {
        if (envites_jugadores[i] == valor)
        {
            ++count; /* encontrado valor */
        }
    }
    return(count);
}

/* Devuelve el máximo de dos enteros*/
int max(int a, int b) {
    if (a >= b) return a;
    else return b;
}

/* Devuelve el mínimo de dos enteros*/
int min(int a, int b) {
    if (a <= b) return a;
    else return b;
}

/* Determina si algún jugador ha envidado */
int hay_apuesta(int envites_jugadores[], int longitud) {
    if (maximo_array(envites_jugadores, longitud) > 1) {
        return 1; // alguien ha envidado con 2 o más
    }
    else {
        return 0; // o no han envidado o han pasado, por lo que no hay envites
    }
}

/* Determina si una apuesta ha acabado o se debe seguir apostando */
int apuesta_terminada(int envites_jugadores[], int longitud) {

    //lance termina si:
    // 4 jugadores están en paso
    // 3 jugadores están en paso y 1 en 2-99
    // 2 jugadores están en paso, son de la misma pareja y 1 en 2-99
    // mayor apuesta de pareja 1 y mayor apuesta de pareja 2 son iguales (apuesta igualada)
    //0: no ha hablado
    //1: paso
    //2: envido (2 piedras, apuesta mínima)
    //3-99: envido N piedras

    if ((contar_ocurrencias(envites_jugadores, longitud, 1) >= 3 ) || (max(envites_jugadores[0], envites_jugadores[2]) == max(envites_jugadores[1], envites_jugadores[3])) || envites_misma_pareja(envites_jugadores)) {
        return 1; //4 o 3 en paso o apuesta igualada, apuesta terminada
    }
    else {
        return 0; //tal y como se ha desarrollado la lógica anterior del programa es imposible cualquier otro caso aquí
    }


}

/* Muestra el envite del jugador por la pantalla en jerga de mus */
void print_envite(int envite, int siguiente_jugador, int hay_apuesta, int envite_N) {
    if (hay_apuesta == 0) {
        switch (envite) {
            case 1:
                printf(BOLDYELLOW "[jugador %d] Paso\n" RESET, siguiente_jugador);
                break;
            case 2:
                printf(BOLDYELLOW "[jugador %d] Envido\n" RESET, siguiente_jugador);
                break;
            case 3:
                if (envite_N != 99) {
                    printf(BOLDYELLOW "[jugador %d] Envido %d\n" RESET, siguiente_jugador, envite_N);
                }
                else {
                    printf(BOLDYELLOW "[jugador %d] Órdago!\n" RESET, siguiente_jugador);
                }
                break;
        }
    }
    else {
        switch (envite) {
            case 1:
                printf(BOLDYELLOW "[jugador %d] No\n" RESET, siguiente_jugador);
                break;
            case 2:
                printf(BOLDYELLOW "[jugador %d] Lo quiero\n" RESET, siguiente_jugador);
                break;
            case 3:
                if (envite_N != 99) {
                    printf(BOLDYELLOW "[jugador %d] Envido %d más\n" RESET, siguiente_jugador, envite_N);
                }
                else {
                    printf(BOLDYELLOW "[jugador %d] Órdago!\n" RESET, siguiente_jugador);
                }
                break;
        }
    }
}


/* Limpia la salida estándar para la lectura de entrada por parte del usuario */
int clean_stdin(){
    while (getchar()!='\n') {
        return 1;
    }
}


/* Determina si los dos jugadores de una pareja pasan */
int pareja_pasa(int envites_jugadores[]) {
    if (((envites_jugadores[0] == 1) && (envites_jugadores[2] == 1))) {
        return 0; //pareja 0
    }

    else if (((envites_jugadores[1]==1) && (envites_jugadores[3] ==1))) {
        return 1; //pareja 1
    }
    else {
        return 2; //ninguna pasa
    }
}


/* Determina si los envites provienen de jugadores de la misma pareja */
int envites_misma_pareja(int envites_jugadores[]) {

    if (((pareja_pasa(envites_jugadores) == 0) && (envites_jugadores[1] > 1) && (envites_jugadores[3] > 1)) ||
        ((pareja_pasa(envites_jugadores) == 1) && (envites_jugadores[0] > 1) && (envites_jugadores[2] > 1))) {
        return 1;
    } else {
        return 0;
    }
}
/* Calcula el envite de una pareja */
    int envite_pareja(int pareja, int mano, int envites[]) {

        if (pareja == 1) { //1 mano
        return max(envites[mano], envites[add_mod(mano, 2, 4)]);
    }
    else { // 0 postre
            return max(envites[add_mod(mano, 1, 4)], envites[add_mod(mano, 3, 4)]);
    }
}

int calcular_envite(int envites[], int envite, int envite_N, int envite_vigor) {

    if ((hay_apuesta(envites, N_JUGADORES) == 0) && (envite_N == 0)) {
        // si no hay apuesta en vigor y envite_N == 0, el envite es envite
        return envite; //envido o paso
    }
    else if ((hay_apuesta(envites, N_JUGADORES) == 0) && (envite_N != 0)) {
        // si no hay apuesta en vigor y envite_N != 0, el envite es envite_N
        return envite_N; //envido N
    }
    else if ((hay_apuesta(envites, N_JUGADORES) == 1)   && (envite ==3)) {
        // si hay apuesta en vigor y envite_N != 0, el envite es: apuesta_vigor + envite_N
        if ((envite_vigor == 99) || (envite_N==99)) {
            //TODO Que el jugador no diga envido 1 más al querer un órdago
            return 99; // el órdago no se sube, se quiere o no
        }
        else {
            return (envite_vigor + envite_N); // subo N
        }
    }
    else if ((hay_apuesta(envites, N_JUGADORES) == 1)  && (envite == 2)) {
        //si hay apuesta en vigor y envite_N ==0 y envite = 1, el envite es: apuesta_vigor
       return envite_vigor; //lo quiero
    }
    else if ((hay_apuesta(envites, N_JUGADORES) == 1)  && (envite == 1)) {
        // si hay apuesta en vigor y envite_N == 0 y envite = 0, el envite es: 1
        return envite; //no lo quiero
    }

}

int esta_valor_en_array(int val, int *arr, int size){
    int i;
    for (i=0; i < size; i++) {
        if (arr[i] == val)
            return 1;
    }
    return 0;
}

int que_pareja_etiqueta_tengo(int rank) {
    if ((rank==0) || (rank==2)){
        return 0; //Pareja 0-2
    }
    else {
        return 1; //pareja 1-3
    }
}