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
#define DEBUG 1
#define CHAR_BUFFER 8

typedef int bool;
#define true 1
#define false 0

const char * caras[] = {"As", "Dos", "Tres", "Cuatro", "Cinco",
                        "Seis", "Siete", "Sota", "Caballo", "Rey"};
const char * palos[] = {"Oros", "Copas", "Espadas", "Bastos"};
const char * lances_etiquetas[] = {"Grande", "Chica", "Pares", "Juego", "Al punto"};
int valores[] = {1, 1, 10, 4, 5, 6, 7, 10, 10, 10};
int equivalencias[] = {1, 1, 10, 4, 5, 6, 7, 8, 9, 10};

int rand_lim(int limit) {
/* return a random number between 0 and limit inclusive.
 */

    int divisor = RAND_MAX/(limit+1);
    int retval;

    do {
        retval = rand() / divisor;
    } while (retval > limit);

    return retval;
}


/* FUNCION crearMazo: puebla un array de estructuras Carta con sus valores y palos*/
int crear_mazo(Carta *mazo) {
    int i; /* contador */
    int size_mazo = 0;

    /* iterar el mazo */
    for (i = 0; i <= N_CARTAS_MAZO - 1; i++) {
        mazo[i].cara = i % 10;
        mazo[i].palo = i / 10;
        mazo[i].id = i;
        mazo[i].orden = i % 10;
        mazo[i].estado = 0;
        size_mazo++;
    } /* fin for */
    return size_mazo;

} /* fin funcion crearMazo */



/* FUNCION printMazo: muestra por pantalla un mazo de cartas */
void print_mazo(Carta *wMazo, int size_mazo) {
    int i;
    for (i = 0; i <= size_mazo - 1; i++) {
        printf("El valor de %-8s\t de \t%s es \t%d \tcon orden \t%d y estado %d\n \t", caras[wMazo[i].cara],
               palos[wMazo[i].palo], valores[wMazo[i].cara], wMazo[i].orden, wMazo[i].estado);
        printf("\n");
    }
    printf("Fin del contenido del mazo/mano.\n");
}

/* FUNCION barajarMazo: baraja las cartas del mazo*/

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

} /* fin funcion barajar */

/* FUNCION cortarMazo: corta el mazo, esto es, saca una carta aleatoria del mazo */

/*void cortar_mazo(Carta *wMazo, char *paloCorte) {

    int r; *//* índice aleatorio para el mazo*//*
    r = rand() % (N_CARTAS_MAZO + 1 - 0) + 0;
    //r = M + rand() / (RAND_MAX / (N - M + 1) + 1);
    paloCorte = (char *) malloc(CHAR_BUFFER * sizeof(char));
    strcpy(paloCorte, wMazo[r].palo);

}*/

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

/* suma modular */
int add_mod(int a, int b, int m) {
    if (0 == b) return a;

    // return sub_mod(a, m-b, m);
    b = m - b;
    if (a >= b)
        return a - b;
    else
        return m - b + a;
}

void repartir_carta(Carta wCarta, int proceso, MPI_Comm wComm) {

    MPI_Send(&wCarta.id, 1, MPI_INT, proceso, 0, wComm);
    MPI_Send(&wCarta.orden, 1, MPI_INT, proceso, 0, wComm);
    MPI_Send(&wCarta.estado, 1, MPI_INT, proceso, 0, wComm);
    MPI_Send(&wCarta.palo, 1, MPI_INT, proceso, 0, wComm);
    MPI_Send(&wCarta.cara, 1, MPI_INT, proceso, 0, wComm);

}

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

void determinar_repartidor(int corte, int repartidor, char * palo_corte, Carta mazo[], MPI_Comm parent, const char * palos[], MPI_Status stat) {



    }

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
                repartir_carta(mazo[k], siguiente_jugador, MPI_COMM_WORLD);
                mazo[k].estado = 1; // la carta pasa a estado repartida
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

int cuentaCartasMano(Carta *wMano, int cara) {
    int i = 0;
    int cuenta = 0;
    for (i = 0; i < N_CARTAS_MANO; i++) {
        if (wMano[i].cara == cara) {
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
void unique_pairs(int *array, int longitud, int repeticion, int parejas[]) {
    qsort(array, longitud, sizeof(int), cmpfunc);
    // now we have: [1, 1, 1, 4, 4, 5, 5, 7, 7] //
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

int tengoMedias(int *paresBuf) {
    if (paresBuf[1] != 99) {
        debug("Tengo medias");
        return 1;
    }
    else {
        return 0;
    }
}

int tengoDuples(int *paresBuf) {

    if (paresBuf[0] != 99 || paresBuf[2] == 2) {
        debug("Tengo duples");
        return 1;
    }
    else {
        return 0;
    }
}

int tengoPares(int *paresBuf) {
    if ((paresBuf[0] == 1) || paresBuf[1] == 1 || paresBuf[2] == 2 || paresBuf[2] == 1) {
        debug("Tengo pares");
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

void marcar_descarte(Carta *wMazo, int sizeMazo, int id) {
    int i;
    for (i = 0; i <= sizeMazo - 1; i++) {
        if (wMazo[i].id == id) {
            wMazo[i].estado = 2;
        }
    }
}