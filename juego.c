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
#define N_JUGADORES 4
#define N_PAREJAS 2
#define N_LANCES 4
#define DEBUG 1


extern const char * caras[];
extern const char * palos[];
extern const char * lancesEtiquetas[];
extern int valores[];
extern int equivalencias[];



int main(int argc, char **argv) {
    /*
     * DECLARACIÓN E INICIALIZACIÓN DE VARIABLES
     */
    int rank, size, version, subversion, namelen, universe_size, size_mazo, size_mano, proceso, corte, repartidor, postre, mano,
            ultimo, siguiente_jugador, mus, token, descarte, repartidor_descartes, turno, n_cartas_a_descartar;
    char processor_name[MPI_MAX_PROCESSOR_NAME], worker_program[100];
    int cartas_a_descartar[N_CARTAS_MANO];
    MPI_Comm juego_comm;
    MPI_Status stat;
   Carta mazo[N_CARTAS_MAZO], mano_jugador[N_CARTAS_MANO], descartada;

    srand(time(NULL)); /* randomize */
    /*
     * INICIALIZACIÓN DE MPI Y CREACIÓN DE PROCESOS JUGADORES
     */
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

    strcpy(worker_program, "./Debug/jugador");
    printf("[maestro] Generando %d procesos ejecutando %s\n", universe_size - 1, worker_program);
    MPI_Comm_spawn(worker_program, MPI_ARGV_NULL, universe_size - 1, MPI_INFO_NULL, 0, MPI_COMM_SELF, &juego_comm,
                   MPI_ERRCODES_IGNORE);
    printf("[maestro] Ejecutado proceso maestro con identificador %d de un total de %d\n", rank, size);
    proceso = 0;
    int i = 0;
    for (i = 0; i < 4; i++) {
        MPI_Send(&i, 1, MPI_INT, i, 0, juego_comm);
        MPI_Recv(&proceso, 1, MPI_INT, i, 0, juego_comm, MPI_STATUS_IGNORE);
        printf("[jugador %d] Jugador %d: Listo para jugar!\n", proceso, proceso);
    }


    /*
     * INICIALIZACIÓN DEL MAZO
     */


    size_mazo = crear_mazo(mazo); // llena el mazo de cartas

    printf("[maestro] Tamaño del mazo: %d\n", size_mazo);

    barajar_mazo(mazo); //Baraja el mazo
    print_mazo(mazo, N_CARTAS_MAZO);

    /*
     * DETERMINACIÓN DE JUGADOR QUE CORTARÁ MAZO
     */
    //int N = 0, M = N_JUGADORES - 1; // valores del intervalo para el corte
    //corte = M + rand() /
    //            (RAND_MAX / (N - M + 1) + 1); // proceso aleatorio de entre los existentes para determinar el corte

    corte = rand_lim(N_JUGADORES-1);
    debug("El jugador que cortará es: %d\n", corte);
    MPI_Bcast(&corte, 1, MPI_INT, MPI_ROOT,
              juego_comm); // envío del id de proceso que realizará el corte a todos los jugadores
    debug("Corte broadcasted: %d\n", corte);
    MPI_Bcast(&size_mazo, 1, MPI_INT, MPI_ROOT, juego_comm);//Envío del tamaño del mazo*/

    /*
     * RECEPCIÓN DEL JUGADOR REPARTIDOR POR PARTE DEL CORTE
     */

    enviar_mazo(mazo, corte, juego_comm, N_CARTAS_MAZO); // envío del mazo al jugador que va a cortar la baraja

    debug("Mazo enviado");
    MPI_Recv(&repartidor, 1, MPI_INT, corte, 0, juego_comm, MPI_STATUS_IGNORE);
    printf("[maestro] El jugador repartidor es: %d\n", repartidor);
    recibir_mazo(mazo, corte, juego_comm, N_CARTAS_MAZO,
                 MPI_STATUS_IGNORE); //se recibe de nuevo el mazo del jugador que ha cortado
    debug("Mazo recibido");
    MPI_Bcast(&repartidor, 1, MPI_INT, MPI_ROOT, juego_comm); //envío del repartidor a todos los procesos

    /*
     * REPARTO DE CARTAS PARA MUS CORRIDO
     */

    // envío del mazo al jugador repartidor
    barajar_mazo(mazo); //Baraja el mazo
    enviar_mazo(mazo, repartidor, juego_comm, N_CARTAS_MAZO);
    debug("Mazo enviado");
    // e/s auxiliar reparto de cartas
    for (i = 0; i <= (N_CARTAS_MANO * N_JUGADORES - 1); i++) {
        int buffer[3];
        MPI_Recv(&buffer, 3, MPI_INT, repartidor, 0, juego_comm, MPI_STATUS_IGNORE);
        printf("[repartidor %d] Repartida carta %d al jugador %d\n", repartidor, buffer[0], buffer[1]);
        int siguiente = buffer[1];
        int carta = buffer[0];
        MPI_Recv(&buffer, 3, MPI_INT, siguiente, 0, juego_comm, MPI_STATUS_IGNORE);
        printf("[jugador %d] Jugador %d recibe carta %d \n",siguiente, siguiente, carta);
    }
    /* Recepción de mazo una vez repartido*/
    MPI_Recv(&size_mazo, 1, MPI_INT, repartidor, 0, juego_comm, MPI_STATUS_IGNORE);
    recibir_mazo(mazo, repartidor, juego_comm, N_CARTAS_MAZO, MPI_STATUS_IGNORE);
    printf("[maestro] tamaño del mazo: %d\n", size_mazo);
    MPI_Bcast(&size_mazo, 1, MPI_INT, MPI_ROOT, juego_comm); //envío del tamaño del mazo a resto de procesos

    /*
     * MUS CORRIDO PARA DETERMINAR QUIEN ES MANO
     */

    mus = 0;

//token = 1 : evaluar + decidir mus
//token = 2 : no mus
//token = 3 : descartar
//token = 4 : repartir
    siguiente_jugador = repartidor;
    turno = 0;
    // Se envía el mazo al jugador a la derecha del repartidor
    while (mus == 0) {
    turno++;
     debug("[maestro] Comienza el turno : %d", turno);
    siguiente_jugador = add_mod(siguiente_jugador, 1, 4);
        token = 1;
                MPI_Send(&token, 1, MPI_INT, siguiente_jugador, 0, juego_comm);
                recibir_mazo(mano_jugador, siguiente_jugador, juego_comm, N_CARTAS_MANO, MPI_STATUS_IGNORE);
                printf("[maestro] Mano del jugador %d\n", siguiente_jugador);
                print_mazo(mano_jugador, N_CARTAS_MANO);
                debug("Envío de mazo a jugador %d", siguiente_jugador);
                enviar_mazo(mazo, siguiente_jugador, juego_comm, N_CARTAS_MAZO);
                // El jugador decide si quiere mus

                MPI_Recv(&mus, 1, MPI_INT, siguiente_jugador, 0, juego_comm, MPI_STATUS_IGNORE);

    if (mus == 1) { // corta el mus
        // En caso de no querer mus, ese jugador es mano y el anterior postre, al que habrá que pasar el mazo
        printf("[jugador %d] Corta mus", siguiente_jugador);
        mano = siguiente_jugador;
        //int ultimo = add_mod(siguiente_jugador, 4, 4);
        ultimo = siguiente_jugador;
        siguiente_jugador = add_mod(siguiente_jugador, 1, 4);
       // while(siguiente_jugador != ultimo) {
        for (i=0; i< N_JUGADORES-1;i++) {
            debug("siguiente jugador en enviar token 2: %d", siguiente_jugador);
            token = 2;
            MPI_Send(&token, 1, MPI_INT, siguiente_jugador, 0, juego_comm);
            siguiente_jugador = add_mod(siguiente_jugador, 1, 4);
        }
        break;
    }
    else { // hay mus
        printf("[jugador %d] Pide mus\n", siguiente_jugador);
        if (turno % 4 != 0){
            //En caso de querer mus, devuelve el mazo al maestro que a su vez lo pasará al jugador siguiente

            recibir_mazo(mazo, siguiente_jugador, juego_comm, N_CARTAS_MAZO, MPI_STATUS_IGNORE);

        }
        else {
            printf("FIN DE RONDA SIN CORTAR MUS\n");

            // Si pasa una ronda completa sin cortar mus, el mazo pasa al jugador de la derecha del último que repartió
            recibir_mazo(mazo, siguiente_jugador, juego_comm, N_CARTAS_MAZO, MPI_STATUS_IGNORE);
            debug("mazo devuelto por jugador %d", siguiente_jugador);
            siguiente_jugador = add_mod(siguiente_jugador, 1, 4);
            repartidor_descartes = siguiente_jugador;
            token = 4; // este jugador será el que reparta los descartes
            debug("Envío de token=%d a jugador %d", token, siguiente_jugador);
            MPI_Send(&token, 1, MPI_INT, siguiente_jugador, 0, juego_comm);
            debug("Envío de mazo a jugador %d", siguiente_jugador);
            enviar_mazo(mazo, siguiente_jugador, juego_comm, N_CARTAS_MAZO);
            i=0;
            int j;
            while(i<4) {
            // Se hacen los descartes

            siguiente_jugador = add_mod(siguiente_jugador, 1, 4);
                debug("[maestro] Siguiente turno para jugador %d", siguiente_jugador);
                if (siguiente_jugador != repartidor_descartes) {
                    debug("[maestro] El siguiente jugador %d no reparte", siguiente_jugador);
                    token = 3; // este jugador será el que haga descartes y pida cartas
                    MPI_Send(&token, 1, MPI_INT, siguiente_jugador, 0, juego_comm);
                }

              printf("[maestro] Jugador %d, ¿cuántas cartas quieres?\n", siguiente_jugador);
                //recibir cuantas cartas quiere
                MPI_Recv(&n_cartas_a_descartar, 1, MPI_INT, siguiente_jugador, 0, juego_comm, MPI_STATUS_IGNORE);
                printf("[jugador %d] Quiero %d cartas\n", siguiente_jugador, n_cartas_a_descartar);
                //recibir las cartas descartadas
                MPI_Recv(cartas_a_descartar, n_cartas_a_descartar, MPI_INT, siguiente_jugador, 0, juego_comm, MPI_STATUS_IGNORE);
                // envío de número de cartas a descartar a repartidor
                MPI_Send(&n_cartas_a_descartar, 1, MPI_INT, repartidor_descartes, 0, juego_comm);

                // envío de array con ids de cartas a descartar a repartidor

                MPI_Send(cartas_a_descartar, n_cartas_a_descartar, MPI_INT, repartidor_descartes, 0, juego_comm);
                for ( j = 0; j < n_cartas_a_descartar; j++) {


                    descartada = recibir_carta(repartidor_descartes, juego_comm, MPI_STATUS_IGNORE);
                    repartir_carta(descartada, siguiente_jugador, juego_comm);

            }
                debug("[maestro] Fin del reparto de cartas\n");
                i++;
                debug("Iteración de jugador número %d", i);
            } //fin while descartes
            MPI_Recv(&size_mazo, 1, MPI_INT, repartidor_descartes, 0, juego_comm, MPI_STATUS_IGNORE);

        } // fin else cuando no se corta el mus y acaba la ronda
    } //fin else cuando jugador pide mus
} // fin while mus corrido (se pide mus)
    //TODO: comprobar estados de las cartas del mazo devuelto


    /*
     * FASE DE LANCES
     */

    // cada ronda cuatro lances
    // GRANDE
    // CHICA
    // PARES
    // JUEGO
   // debug("TAMAÑO DEL MAZO DESPUÉS DE DESCARTES: %d", size_mazo);
    //resultado: comparar cartas de una pareja despecto de la otra


    debug("[maestro] FINALIZADO!");
    /*free(mazo->palo);
    free(mazo->cara);*/

    //MPI_Comm_disconnect(&juego_comm);
    //MPI_Barrier(juego_comm);
    MPI_Finalize();
    return 0;
}