//
// Created by Miguel Expósito Martín, 72056097H
//

#include <mpi.h>
#include <stdio.h>
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
#define DEBUG 0
#define RESET   "\033[0m"
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

extern const char *caras[];
extern const char *palos[];
extern const char *lances_etiquetas[];
extern const char *parejas[];
extern const char *respuestas[];
extern const char *parejas_etiquetas[];
extern int valores[];
extern int equivalencias[];


int main(int argc, char **argv) {
    /*
     * DECLARACIÓN E INICIALIZACIÓN DE VARIABLES
     */
    int rank, size, version, subversion, namelen, universe_size, size_mazo, size_mano, proceso, jugador_humano, corte, repartidor, postre, mano,
            ultimo, siguiente_jugador, jugador_espera, mus, token, descarte, repartidor_descartes, turno, n_cartas_a_descartar, envite, envite_N, em, ep, contador, indicador_pares, juego_al_punto;
    char processor_name[MPI_MAX_PROCESSOR_NAME], worker_program[100], c;
    int cartas_a_descartar[N_CARTAS_MANO] = {99, 99, 99, 99};
    //Array de 4 posiciones para los envites, una para cada jugador
    //0: no ha hablado
    //1: paso
    //2: envido (2 piedras, apuesta mínima)
    //3-99: envido N piedras
    int envites_jugador[N_JUGADORES] = {0, 0, 0, 0};
    int tengo_pares[N_JUGADORES] = {0, 0, 0, 0};
    int tengo_juego[N_JUGADORES] = {0, 0, 0, 0};
    int piedra_no[N_LANCES] = {0,0,0,0};
    int entradas_posibles[3] = {1, 2, 3};
    int entradas_posibles_mus[2] = {0, 1};
    int entradas_posibles_vacas[2] = {3, 5};
    int entradas_posibles_puntos[2] = {30, 40};
    int config_partida_default = 1;
    int piedras[N_PAREJAS] = {0, 0};
    int piedras_parejas[N_PAREJAS] = {0, 0};
    int puntos_juego[N_PAREJAS] = {0, 0};
    int n_juegos[N_PAREJAS] = {0, 0};
    int n_vacas[N_PAREJAS] = {0, 0};
    int envites[2] = {0, 0};
    int envite_anterior[] = {0, 0};
    int conteos[10], rbuf[50], paresbuf[25], juegobuf[5]; //buffers para recibir jugadas
    int ganador[N_LANCES]; // buffer para almacenar ganadores de cada lance
    int n_puntos_juego = 40;
    int n_juegos_vaca = 3;
    int n_vacas_partida = 3;
    int modo_juego =0; //automático por defecto
    jugador_humano = 3;
    int ronda = 0;
    int fin_partida = 0;
    int indicador_ordago = 0;
    MPI_Comm juego_comm;
    MPI_Status stat;
    Carta mazo[N_CARTAS_MAZO], mano_jugador[N_CARTAS_MANO], descartada;

    srand(time(NULL)); /* randomize */

    /******************************************************************************************************************
  * INICIALIZACIÓN DE MPI Y CREACIÓN DE PROCESOS JUGADORES   ******************************************************************************************************************/

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
    proceso = 0;
    int i = 0;
    for (i = 0; i < N_JUGADORES; i++) {
        MPI_Send(&i, 1, MPI_INT, i, 0, juego_comm);
        MPI_Recv(&proceso, 1, MPI_INT, i, 0, juego_comm, MPI_STATUS_IGNORE);
        printf(BOLDYELLOW "[jugador %d] Jugador %d: Listo para jugar!\n" RESET, proceso, proceso);
    }

    /******************************************************************************************************************
    * DECISIÓN DEL TIPO Y DURACIÓN DE LA PARTIDA
    *****************************************************************************************************************/

    //Selección de modo (interactivo: 1/automático: 0)

    do {
        printf(BOLDMAGENTA "¿[maestro] ¿Desea jugar en modo automático o interactivo? (0: automático, 1: interactivo)\n" RESET);
    } while (((scanf("%d%c", &modo_juego, &c) != 2 || c != '\n') && clean_stdin()) ||
             esta_valor_en_array(modo_juego, entradas_posibles_mus, 2) == 0);

    jugador_humano = rand_lim(N_JUGADORES - 1);

    if (modo_juego == 1) {
        printf(BOLDBLUE "[maestro] El identificador para el jugador humano es: %d\n" RESET, jugador_humano);
    }

    do {
        printf(BOLDMAGENTA "¿[maestro] ¿Desea usar la configuración de partida por defecto de 3 vacas por partida, 3 juegos por vaca y 40 puntos al juego? (0: No, 1: Sí)\n" RESET);
    } while (((scanf("%d%c", &config_partida_default, &c) != 2 || c != '\n') && clean_stdin()) ||
             esta_valor_en_array(config_partida_default, entradas_posibles_mus, 2) == 0);


    if (config_partida_default == 0) {

        do {
            printf(BOLDMAGENTA "¿[maestro] ¿Cuántas vacas jugamos por partida? (3/5)\n" RESET);
        } while (((scanf("%d%c", &n_vacas_partida, &c) != 2 || c != '\n') && clean_stdin()) ||
                 esta_valor_en_array(n_vacas_partida, entradas_posibles_vacas, 2) == 0);

        printf(BOLDBLUE "[maestro] Se juega a %d vacas por partida\n" RESET, n_vacas_partida);

        do {
            printf(BOLDMAGENTA "¿[maestro] ¿Cuántas juegos jugamos por vaca? (3/5)\n" RESET);
        } while (((scanf("%d%c", &n_juegos_vaca, &c) != 2 || c != '\n') && clean_stdin()) ||
                 esta_valor_en_array(n_juegos_vaca, entradas_posibles_vacas, 2) == 0);

        printf(BOLDBLUE "[maestro] Se juega a %d juegos por vaca\n" RESET, n_vacas_partida);

        do {
            printf(BOLDMAGENTA "¿[maestro] ¿A cuántos puntos jugamos el juego? (30/40)\n" RESET);
        } while (((scanf("%d%c", &n_puntos_juego, &c) != 2 || c != '\n') && clean_stdin()) ||
                 esta_valor_en_array(n_puntos_juego, entradas_posibles_puntos, 2) == 0);

    }

   /******************************************************************************************************************
   * COMIENZO DE PARTIDA  *****************************************************************************************************************/

    MPI_Bcast(&modo_juego, 1, MPI_INT, MPI_ROOT, juego_comm);
    MPI_Bcast(&jugador_humano, 1, MPI_INT, MPI_ROOT, juego_comm);//Envío deljugador humano*/
    MPI_Bcast(&n_puntos_juego, 1, MPI_INT, MPI_ROOT, juego_comm);

    int iteracion = 0;
    // mientras ninguna pareja gane...
    while (fin_partida == 0) {
        MPI_Bcast(&ronda, 1, MPI_INT, MPI_ROOT, juego_comm);
        /*
         * INICIALIZACIÓN DEL MAZO
         */

        if (ronda == 0) { //primera ronda: corte, reparto inicial y mus corrido
            printf("[maestro] Comienza ronda inicial del juego, con mus corrido\n");
            size_mazo = crear_mazo(mazo); // llena el mazo de cartas

            printf("[maestro] Tamaño del mazo: %d\n", size_mazo);

            barajar_mazo(mazo); //Baraja el mazo
            print_mazo(mazo, N_CARTAS_MAZO);

            /*
             * DETERMINACIÓN DE JUGADOR QUE CORTARÁ MAZO
             */

            corte = rand_lim(N_JUGADORES - 1);
            printf("[maestro] El jugador que cortará es: %d\n", corte);
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
                printf(BOLDYELLOW "[jugador %d] Recibo carta %d \n" RESET, siguiente, carta);
            }
            /* Recepción de mazo una vez repartido*/
            MPI_Recv(&size_mazo, 1, MPI_INT, repartidor, 0, juego_comm, MPI_STATUS_IGNORE);
            recibir_mazo(mazo, repartidor, juego_comm, N_CARTAS_MAZO, MPI_STATUS_IGNORE);
            printf("[maestro] tamaño del mazo: %d\n", size_mazo);
            MPI_Bcast(&size_mazo, 1, MPI_INT, MPI_ROOT, juego_comm); //envío del tamaño del mazo a resto de procesos
            for (i = 0; i < N_JUGADORES; i++) {
                MPI_Send(&i, 1, MPI_INT, i, 0, juego_comm);
                recibir_mazo(mano_jugador, i, juego_comm, N_CARTAS_MANO, MPI_STATUS_IGNORE);
                printf("[jugador %d] Juega con la siguiente mano:\n", i);
                print_mazo(mano_jugador, N_CARTAS_MANO);
            }

            /**********************************************************************************************************
  * MUS CORRIDO
 **********************************************************************************************************/

            //objetivo: determinar jugador mano

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
                printf(BOLDBLUE "[maestro] Comienza el turno %d de mus corrido\n" RESET, turno);
                siguiente_jugador = add_mod(siguiente_jugador, 1, 4);

                token = 1;
                printf("[maestro] Enviando token 1 a jugador %d\n", siguiente_jugador);
                MPI_Send(&token, 1, MPI_INT, siguiente_jugador, 0, juego_comm);
                recibir_mazo(mano_jugador, siguiente_jugador, juego_comm, N_CARTAS_MANO, MPI_STATUS_IGNORE);
                printf("[maestro] Mano del jugador %d\n", siguiente_jugador);
                print_mazo(mano_jugador, N_CARTAS_MANO);
                debug("Envío de mazo a jugador %d", siguiente_jugador);
                MPI_Send(&size_mazo, 1, MPI_INT, siguiente_jugador, 0, juego_comm);
                enviar_mazo(mazo, siguiente_jugador, juego_comm, N_CARTAS_MAZO);
                // El jugador humano decide si quiere mus; entrada por teclado
                if ((modo_juego == 1) && (siguiente_jugador == jugador_humano)) {
                    do {
                        printf(BOLDMAGENTA "[jugador %d] ¿Quieres mus?: (0:Dame mus, 1:no hay mus)\n" RESET,
                               siguiente_jugador);
                    } while (((scanf("%d%c", &mus, &c) != 2 || c != '\n') && clean_stdin()) ||
                             esta_valor_en_array(mus, entradas_posibles_mus, 2) == 0);
                    MPI_Send(&mus, 1, MPI_INT, siguiente_jugador, 0, juego_comm);

                } else { //modo automático
                    MPI_Recv(&mus, 1, MPI_INT, siguiente_jugador, 0, juego_comm, MPI_STATUS_IGNORE);
                }


                if (mus == 1) { // se corta el mus
                    // En caso de no querer mus, ese jugador es mano y el anterior postre, al que habrá que pasar el mazo
                    // en posteriores rondas repartirá el que haya sido mano cada vez
                    printf(BOLDRED "[jugador %d] No hay mus \n" RESET, siguiente_jugador);
                    mano = siguiente_jugador;
                    postre = add_mod(siguiente_jugador, 3, 4);

                    siguiente_jugador = add_mod(siguiente_jugador, 1, 4);
                    for (i = 0; i < N_JUGADORES - 1; i++) {
                        debug("siguiente jugador en enviar token 2: %d", siguiente_jugador);
                        token = 2;
                        MPI_Send(&token, 1, MPI_INT, siguiente_jugador, 0, juego_comm);

                        siguiente_jugador = add_mod(siguiente_jugador, 1, 4);
                    }
                    break;
                } else { // hay mus
                    printf(BOLDGREEN "[jugador %d] Quiero mus\n" RESET, siguiente_jugador);
                    if (turno % 4 != 0) {
                        //En caso de querer mus, devuelve el mazo al maestro que a su vez lo pasará al jugador siguiente
                        printf("[maestro] Recibido mazo de jugador %d\n", siguiente_jugador);
                        MPI_Recv(&size_mazo, 1, MPI_INT, siguiente_jugador, 0, juego_comm, MPI_STATUS_IGNORE);
                        recibir_mazo(mazo, siguiente_jugador, juego_comm, N_CARTAS_MAZO, MPI_STATUS_IGNORE);


                    } else { //comienza fase de descartes
                        printf(BOLDBLUE "Fin de ronda sin cortar mus. Comienza fase de descartes.\n" RESET);

                        /*********************************************************************************************
                         * FASE DE DESCARTES MUS CORRIDO
                         *********************************************************************************************/

                        // Si pasa una ronda completa sin cortar mus, el mazo pasa al jugador de la derecha del último que repartió
                        MPI_Recv(&size_mazo, 1, MPI_INT, siguiente_jugador, 0, juego_comm, MPI_STATUS_IGNORE);
                        recibir_mazo(mazo, siguiente_jugador, juego_comm, N_CARTAS_MAZO, MPI_STATUS_IGNORE);
                        debug("mazo devuelto por jugador %d", siguiente_jugador);
                        siguiente_jugador = add_mod(siguiente_jugador, 1, 4);
                        repartidor_descartes = siguiente_jugador;
                        token = 4; // este jugador será el que reparta los descartes
                        debug("Envío de token=%d a jugador %d", token, siguiente_jugador);
                        MPI_Send(&token, 1, MPI_INT, siguiente_jugador, 0, juego_comm);
                        debug("Envío de mazo a jugador %d\n", siguiente_jugador);

                        MPI_Send(&size_mazo, 1, MPI_INT, siguiente_jugador, 0, juego_comm);
                        enviar_mazo(mazo, siguiente_jugador, juego_comm, N_CARTAS_MAZO);
                        i = 0;
                        int j;
                        while (i < 4) {
                            // Se hacen los descartes

                            siguiente_jugador = add_mod(siguiente_jugador, 1, 4);
                            debug("[maestro] Siguiente turno para jugador %d", siguiente_jugador);
                            if (siguiente_jugador != repartidor_descartes) {
                                debug("[maestro] El siguiente jugador %d no reparte", siguiente_jugador);
                                token = 3; // este jugador será el que haga descartes y pida cartas
                                MPI_Send(&token, 1, MPI_INT, siguiente_jugador, 0, juego_comm);
                            } else {
                                debug("[maestro] LE TOCA AL REPARTIDOR\n");
                            }


                            if ((modo_juego == 1) && (siguiente_jugador == jugador_humano)) {
                                recibir_mazo(mano_jugador, siguiente_jugador, juego_comm, N_CARTAS_MANO,
                                             MPI_STATUS_IGNORE);
                                n_cartas_a_descartar = 0;
                                for (j = 0; j < N_CARTAS_MANO; j++) {

                                    do {
                                        printf(BOLDMAGENTA "¿[jugador %d] Quieres descartar %s de %s? (0: no, 1: sí)\n " RESET,
                                               siguiente_jugador, caras[mano_jugador[j].cara],
                                               palos[mano_jugador[j].palo]);
                                    } while (((scanf("%d%c", &descarte, &c) != 2 || c != '\n') && clean_stdin()) ||
                                             esta_valor_en_array(descarte, entradas_posibles_mus, 2) == 0);
                                    cartas_a_descartar[j] = 99; //inicialización siempre
                                    if (descarte == 1) {
                                        //añadir id a lista
                                        cartas_a_descartar[n_cartas_a_descartar] = mano_jugador[j].id;
                                        n_cartas_a_descartar++;
                                        //mostrar por pantalla lista de ids a descartar

                                    }

                                }
                                printf(BOLDYELLOW "[jugador %d] Quiero %d cartas\n" RESET, siguiente_jugador,
                                       n_cartas_a_descartar);

                                MPI_Send(&n_cartas_a_descartar, 1, MPI_INT, jugador_humano, 0, juego_comm);
                                MPI_Send(cartas_a_descartar, n_cartas_a_descartar, MPI_INT, jugador_humano, 0,
                                         juego_comm);
                            } else { //jugador automático
                                printf(BOLDMAGENTA"[maestro] Jugador %d, ¿cuántas cartas quieres?\n" RESET,
                                       siguiente_jugador);
                                //recibir cuantas cartas quiere
                                MPI_Recv(&n_cartas_a_descartar, 1, MPI_INT, siguiente_jugador, 0, juego_comm,
                                         MPI_STATUS_IGNORE);
                                printf(BOLDYELLOW "[jugador %d] Quiero %d cartas\n "RESET, siguiente_jugador,
                                       n_cartas_a_descartar);
                                //recibir las cartas descartadas
                                MPI_Recv(cartas_a_descartar, n_cartas_a_descartar, MPI_INT, siguiente_jugador, 0,
                                         juego_comm,
                                         MPI_STATUS_IGNORE);

                            }


                            // envío de número de cartas a descartar a repartidor
                            MPI_Send(&n_cartas_a_descartar, 1, MPI_INT, repartidor_descartes, 0, juego_comm);

                            // envío de array con ids de cartas a descartar a repartidor
                            MPI_Send(cartas_a_descartar, n_cartas_a_descartar, MPI_INT, repartidor_descartes, 0,
                                     juego_comm);
                            MPI_Recv(&size_mazo, 1, MPI_INT, repartidor_descartes, 0, juego_comm, MPI_STATUS_IGNORE);


                            for (j = 0; j < n_cartas_a_descartar; j++) {


                                descartada = recibir_carta(repartidor_descartes, juego_comm, MPI_STATUS_IGNORE);
                                repartir_carta(descartada, siguiente_jugador, juego_comm);
                                printf("[maestro] Repartidor %d reparte carta con id %d a jugador %d\n",
                                       repartidor_descartes, descartada.id, siguiente_jugador);

                            }

                            debug("[maestro] Fin del reparto de cartas\n");
                            i++;


                        } //fin while descartes

                        MPI_Recv(&size_mazo, 1, MPI_INT, repartidor_descartes, 0, juego_comm, MPI_STATUS_IGNORE);
                        recibir_mazo(mazo, repartidor_descartes, juego_comm, N_CARTAS_MAZO, MPI_STATUS_IGNORE);
                        //ya lo ha barajado repartidor, volverlo a hacer no tiene efecto real
                        barajar_mazo(mazo);


                    } // fin else cuando no se corta el mus y acaba la ronda
                } //fin else cuando jugador pide mus
            } // fin while mus corrido (se pide mus)
            //sincronización del jugador mano
            MPI_Bcast(&mano, 1, MPI_INT, MPI_ROOT, juego_comm);
        } // end if ronda inicial

            /*************************************************************************************************************
             * MUS NORMAL
             *************************************************************************************************************/
        else { //rondas normales sin mus corrido
            printf("[maestro] Comienza ronda %d del juego\n", ronda);

            //inicializar mazo
            size_mazo = crear_mazo(mazo); // llena el mazo de cartas
            MPI_Bcast(&size_mazo, 1, MPI_INT, MPI_ROOT, juego_comm);
            printf("[maestro] Tamaño del mazo: %d\n", size_mazo);

            //barajear mazo
            barajar_mazo(mazo); //Baraja el mazo
            print_mazo(mazo, N_CARTAS_MAZO);
            //determinar mano y postre : pasan al jugador de la derecha
            mano = add_mod(mano, 1, 4);
            postre = add_mod(postre, 1, 4);
            printf(BOLDBLUE "[maestro] Ahora el jugador mano es: %d\n" RESET, mano);
            printf(BOLDBLUE "[maestro] Ahora el jugador postre (repartidor) es: %d\n" RESET, postre);
            repartidor = postre;
            MPI_Bcast(&repartidor, 1, MPI_INT, MPI_ROOT, juego_comm); //envío del repartidor a todos los procesos
            printf(BOLDBLUE "[maestro] Actualizando jugador mano en todos los jugadores...\n" RESET);
//Se comunica a todos los jugadores quien es la mano
            MPI_Bcast(&mano, 1, MPI_INT, MPI_ROOT, juego_comm); //envío del jugador mano a todos los procesos


            /*
             * REPARTO DE CARTAS PARA MUS
             */

            // envío del mazo al jugador repartidor (postre)

            enviar_mazo(mazo, repartidor, juego_comm, N_CARTAS_MAZO);
            debug("Mazo enviado");
            // e/s auxiliar reparto de cartas
            for (i = 0; i <= (N_CARTAS_MANO * N_JUGADORES - 1); i++) {
                int buffer[3];
                MPI_Recv(&buffer, 3, MPI_INT, repartidor, 0, juego_comm, MPI_STATUS_IGNORE);
                printf(BOLDMAGENTA "[repartidor %d] Repartida carta %d al jugador %d\n" RESET, repartidor, buffer[0],
                       buffer[1]);
                int siguiente = buffer[1];
                int carta = buffer[0];
                MPI_Recv(&buffer, 3, MPI_INT, siguiente, 0, juego_comm, MPI_STATUS_IGNORE);
                printf(BOLDYELLOW "[jugador %d] Jugador %d recibe carta %d \n" RESET, siguiente, siguiente, carta);
            }
            /* Recepción de mazo una vez repartido*/
            MPI_Recv(&size_mazo, 1, MPI_INT, repartidor, 0, juego_comm, MPI_STATUS_IGNORE);
            recibir_mazo(mazo, repartidor, juego_comm, N_CARTAS_MAZO, MPI_STATUS_IGNORE);
            printf("[maestro] tamaño del mazo: %d\n", size_mazo);
            MPI_Bcast(&size_mazo, 1, MPI_INT, MPI_ROOT, juego_comm); //envío del tamaño del mazo a resto de procesos

            //cada jugador indica si quiere o no quiere mus

            mus = 0;

//token = 1 : evaluar + decidir mus
//token = 2 : no mus
//token = 3 : descartar
//token = 4 : repartir
            siguiente_jugador = repartidor;
            turno = 0;

            printf("[maestro] Los jugadores juegan el turno %d con las siguientes manos:\n", turno);
            int m;
            for (m = 0; m < N_JUGADORES; m++) {
                recibir_mazo(mano_jugador, m, juego_comm, N_CARTAS_MANO, MPI_STATUS_IGNORE);
                printf("[maestro] Jugador %d juega con mano: \n", m);
                print_mazo(mano_jugador, N_CARTAS_MANO);
            }


            // ya NO se envía el mazo al jugador a la derecha del repartidor
            while (mus == 0) {
                turno++;
                printf(BOLDBLUE "[maestro] Comienza el turno de MUS : %d\n" RESET, turno);
                siguiente_jugador = add_mod(siguiente_jugador, 1, 4);
                token = 1;
                debug("[maestro] Enviando token 1 a jugador %d\n", siguiente_jugador);
                MPI_Send(&token, 1, MPI_INT, siguiente_jugador, 0, juego_comm);
                recibir_mazo(mano_jugador, siguiente_jugador, juego_comm, N_CARTAS_MANO, MPI_STATUS_IGNORE);
                printf("[maestro] Mano del jugador %d\n", siguiente_jugador);
                print_mazo(mano_jugador, N_CARTAS_MANO);

                // El jugador humano decide si quiere mus; entrada por teclado
                if ((modo_juego == 1) && (siguiente_jugador == jugador_humano)) {
                    do {
                        printf(BOLDMAGENTA "[jugador %d] ¿Quieres mus?: (0:Dame mus, 1:no hay mus)\n" RESET,
                               siguiente_jugador);
                    } while (((scanf("%d%c", &mus, &c) != 2 || c != '\n') && clean_stdin()) ||
                             esta_valor_en_array(mus, entradas_posibles_mus, 2) == 0);
                    MPI_Send(&mus, 1, MPI_INT, siguiente_jugador, 0, juego_comm);

                } else { //modo automático
                    MPI_Recv(&mus, 1, MPI_INT, siguiente_jugador, 0, juego_comm, MPI_STATUS_IGNORE);
                }


                if (mus == 1) { // se corta el mus

                    printf(BOLDRED "[jugador %d] No hay mus\n" RESET, siguiente_jugador);
                    siguiente_jugador = add_mod(siguiente_jugador, 1, 4);
                    for (i = 0; i < N_JUGADORES - 1; i++) {
                        debug("siguiente jugador en enviar token 2: %d", siguiente_jugador);
                        token = 2;
                        MPI_Send(&token, 1, MPI_INT, siguiente_jugador, 0, juego_comm);
                        siguiente_jugador = add_mod(siguiente_jugador, 1, 4);
                    }
                    break;
                } else { // hay mus
                    printf(BOLDYELLOW "[jugador %d] Quiero mus\n" RESET, siguiente_jugador);

                    /**************************************************************************************************
                     * FASE DE DESCARTES MUS NORMAL
                     **************************************************************************************************/

                    if (turno % 4 == 0) {  //hemos pasado por todos, comienza fase de descartes
                        printf(BOLDBLUE "[maestro] Fin de ronda sin cortar mus. Comienza fase de descartes.\n" RESET);
                        repartidor_descartes = postre;

                        i = 0;
                        int j;
                        token = 4; // este jugador será el que reparta los descartes
                        debug("Envío de token=%d a jugador %d", token, siguiente_jugador);
                        MPI_Send(&token, 1, MPI_INT, siguiente_jugador, 0, juego_comm);
                        debug("Actualizar mazo en jugador repartidor %d\n", siguiente_jugador);

                        MPI_Send(&size_mazo, 1, MPI_INT, siguiente_jugador, 0, juego_comm);
                        enviar_mazo(mazo, siguiente_jugador, juego_comm, N_CARTAS_MAZO);
                        while (i < 4) {
                            // Se hacen los descartes

                            siguiente_jugador = add_mod(siguiente_jugador, 1, 4);
                            debug("[maestro] Siguiente turno para jugador %d", siguiente_jugador);
                            if (siguiente_jugador != repartidor_descartes) {
                                debug("[maestro] El siguiente jugador %d no reparte", siguiente_jugador);
                                token = 3; // este jugador será el que haga descartes y pida cartas
                                MPI_Send(&token, 1, MPI_INT, siguiente_jugador, 0, juego_comm);
                            } else {
                                debug("[maestro] LE TOCA AL REPARTIDOR\n");

                            }


                            if ((modo_juego == 1) && (siguiente_jugador == jugador_humano)) {
                                recibir_mazo(mano_jugador, siguiente_jugador, juego_comm, N_CARTAS_MANO,
                                             MPI_STATUS_IGNORE);
                                n_cartas_a_descartar = 0;
                                for (j = 0; j < N_CARTAS_MANO; j++) {

                                    do {
                                        printf(BOLDMAGENTA "¿[jugador %d] Quieres descartar %s de %s? (0: no, 1: sí)\n" RESET,
                                               siguiente_jugador, caras[mano_jugador[j].cara],
                                               palos[mano_jugador[j].palo]);
                                    } while (((scanf("%d%c", &descarte, &c) != 2 || c != '\n') && clean_stdin()) ||
                                             esta_valor_en_array(descarte, entradas_posibles_mus, 2) == 0);
                                    cartas_a_descartar[j] = 99; //inicialización siempre
                                    if (descarte == 1) {
                                        //añadir id a lista
                                        cartas_a_descartar[n_cartas_a_descartar] = mano_jugador[j].id;
                                        n_cartas_a_descartar++;
                                        //mostrar por pantalla lista de ids a descartar

                                    }
                                }
                                printf(BOLDYELLOW "[jugador %d] Quiero %d cartas\n" RESET, siguiente_jugador, n_cartas_a_descartar);

                                       MPI_Send(&n_cartas_a_descartar, 1, MPI_INT, jugador_humano, 0, juego_comm);
                                MPI_Send(cartas_a_descartar, n_cartas_a_descartar, MPI_INT, jugador_humano, 0,
                                         juego_comm);
                            } else { //jugador automático
                                printf(BOLDMAGENTA "[maestro] Jugador %d, ¿cuántas cartas quieres?\n" RESET,
                                       siguiente_jugador);
                                //recibir cuantas cartas quiere
                                MPI_Recv(&n_cartas_a_descartar, 1, MPI_INT, siguiente_jugador, 0, juego_comm,
                                         MPI_STATUS_IGNORE);
                                printf(BOLDYELLOW "[jugador %d] Quiero %d cartas\n" RESET, siguiente_jugador,
                                       n_cartas_a_descartar);
                                //recibir las cartas descartadas
                                MPI_Recv(cartas_a_descartar, n_cartas_a_descartar, MPI_INT, siguiente_jugador, 0,
                                         juego_comm,
                                         MPI_STATUS_IGNORE);

                            }


                            // envío de número de cartas a descartar a repartidor
                            MPI_Send(&n_cartas_a_descartar, 1, MPI_INT, repartidor_descartes, 0, juego_comm);

                            // envío de array con ids de cartas a descartar a repartidor
                            MPI_Send(cartas_a_descartar, n_cartas_a_descartar, MPI_INT, repartidor_descartes, 0,
                                     juego_comm);
                            MPI_Recv(&size_mazo, 1, MPI_INT, repartidor_descartes, 0, juego_comm, MPI_STATUS_IGNORE);


                            for (j = 0; j < n_cartas_a_descartar; j++) {


                                descartada = recibir_carta(repartidor_descartes, juego_comm, MPI_STATUS_IGNORE);
                                repartir_carta(descartada, siguiente_jugador, juego_comm);
                                printf(BOLDMAGENTA "[maestro] Repartidor %d reparte carta con id %d a jugador %d\n" RESET,
                                       repartidor_descartes, descartada.id, siguiente_jugador);


                            }

                            debug(BOLDBLUE
                                          "[maestro] Fin del reparto de cartas\n"
                                          RESET);
                            i++;


                        }
                        //Actualiza mazo desde repartidor
                        MPI_Recv(&size_mazo, 1, MPI_INT, repartidor_descartes, 0, juego_comm, MPI_STATUS_IGNORE);
                        recibir_mazo(mazo, repartidor_descartes, juego_comm, N_CARTAS_MAZO, MPI_STATUS_IGNORE);

                        //barajar_mazo(mazo); no es necesario; lo hace el repartidor



                    } // fin else cuando no se corta el mus y acaba la ronda
                } //fin else cuando jugador pide mus

            } // fin while mus corrido (se pide mus)



        }

        // se pasa el mazo al jugador postre
        MPI_Bcast(&mano, 1, MPI_INT, MPI_ROOT, juego_comm);
        MPI_Bcast(&postre, 1, MPI_INT, MPI_ROOT, juego_comm);
        enviar_mazo(mazo, postre, juego_comm, N_CARTAS_MAZO);
        printf("[maestro] Manos con las que se juega la partida: \n");
        siguiente_jugador = mano;
        for (i = 0; i < 4; i++) {
            recibir_mazo(mano_jugador, siguiente_jugador, juego_comm, N_CARTAS_MANO, MPI_STATUS_IGNORE);
            printf("[maestro] Mano del jugador %d\n", siguiente_jugador);
            print_mazo(mano_jugador, N_CARTAS_MANO);
            siguiente_jugador = add_mod(siguiente_jugador, 1, 4);
        }

        /*************************************************************************************************************
         * FASE DE LANCES
         *************************************************************************************************************/

        // cada ronda cuatro lances

        //token = 1 : evaluar + decidir envite
        //token = 2 : esperar a ver qué dicen los otros
        //token = 3 : apuesta igualada


        int l;
        for (l = 0; l < N_LANCES; l++) { // iterar N_LANCES...

            printf(BOLDBLUE "[maestro] Iniciando lance de %s\n" RESET, lances_etiquetas[l]);

            piedra_no[l] = 0; //inicializar a cero en cada lance!

            siguiente_jugador = mano;

            if (l == 2) {
                //preguntar a cada jugador si tiene pares
                indicador_pares = 0;
                MPI_Gather(conteos, 1, MPI_INT, tengo_pares, 1, MPI_INT, MPI_ROOT, juego_comm);

                i = mano;
                contador = 0;
                indicador_pares = 0;
                int pares_humano = 0;
                while (contador < N_JUGADORES) {
                    if ((modo_juego == 1) && (i == jugador_humano)) { //modo interactivo
                        do {
                            printf(BOLDMAGENTA "[jugador %d] ¿Tienes pares?: (0:No, 1:Sí)\n" RESET, i);
                        } while (((scanf("%d%c", &pares_humano, &c) != 2 || c != '\n') && clean_stdin()) ||
                                 esta_valor_en_array(pares_humano, entradas_posibles_mus, 2) == 0);
                        if (tengo_pares[i] != pares_humano){
                            printf(BOLDRED "[maestro] Te has equivocado con los pares!!!\n" RESET);
                            //si el jugador miente, se le ignora para evitar bloqueos
                        }
                        else {
                            tengo_pares[i] = pares_humano;
                        }
                    }
                    printf(BOLDYELLOW "[maestro] Jugador %d %s tengo pares\n" RESET, i, respuestas[tengo_pares[i]]);
                    i = add_mod(i, 1, 4);
                    contador++;
                }

                //si nadie tiene, no se calcula ganador. 0 piedras para cada pareja.
                if (ocurrenciasArray(tengo_pares, 4, 0) == 4) {
                    printf("NADIE TIENE PARES!!!\n");
                    indicador_pares = 0;
                    //enviar indicador de pares a jugadores
                    for (i = 0; i < N_JUGADORES; i++) { //no importa el orden
                        MPI_Send(&indicador_pares, 1, MPI_INT, i, 0, juego_comm);

                    }
                }
                    //si tiene sólo una pareja, esta ganará una piedra en el conteo al final
                else if ((tengo_pares[0] == 0) && (tengo_pares[2] == 0) &&
                         ((tengo_pares[1] == 1) || (tengo_pares[3] == 1))) {
                    //gana piedra pareja 1-3
                    ganador[l] = 1; //da igual a qué jugador le hagamos ganador, sólo importa que sea de la pareja ganadora
                    indicador_pares = 1;
                    //enviar indicador de pares
                    for (i = 0; i < N_JUGADORES; i++) { //no importa el orden
                        MPI_Send(&indicador_pares, 1, MPI_INT, i, 0, juego_comm);

                    }
                } else if ((tengo_pares[1] == 0) && (tengo_pares[3] == 0) &&
                           ((tengo_pares[0] == 1) || (tengo_pares[2] == 1))) {
                    //gana piedra pareja 0-2
                    ganador[l] = 0; //da igual a qué jugador le hagamos ganador, sólo importa que sea de la pareja ganadora
                    indicador_pares = 1;
                    //enviar indicador de pares
                    for (i = 0; i < N_JUGADORES; i++) { //no importa el orden
                        MPI_Send(&indicador_pares, 1, MPI_INT, i, 0, juego_comm);

                    }
                } else {
                    //si tienen las dos parejas, entonces se calcula ganador
                    indicador_pares = 2;
                    //enviar indicador de pares
                    for (i = 0; i < N_JUGADORES; i++) { //no importa el orden
                        MPI_Send(&indicador_pares, 1, MPI_INT, i, 0, juego_comm);

                    }
                }
            } else if (l == 3) {//preguntar a cada jugador si tiene juego

                MPI_Gather(conteos, 1, MPI_INT, tengo_juego, 1, MPI_INT, MPI_ROOT, juego_comm);
                i = mano;

                contador = 0;
                //Juego al punto:
                //0: una pareja no tiene juego y la otra sí
                //1: ninguna pareja tiene juego. Se juega al punto
                //2: las dos parejas tienen juego.
                int juego_humano = 0;
                while (contador < N_JUGADORES) {

                    if ((modo_juego == 1) && (i == jugador_humano)) { //modo interactivo
                        do {
                            printf(BOLDMAGENTA "[jugador %d] ¿Tienes juego?: (0:No, 1:Sí)\n" RESET, i);
                        } while (((scanf("%d%c", &juego_humano, &c) != 2 || c != '\n') && clean_stdin()) ||
                                 esta_valor_en_array(juego_humano, entradas_posibles_mus, 2) == 0);
                        if (tengo_juego[i] != juego_humano) {
                            printf(BOLDRED "[maestro] Te has equivocado con el juego!!!\n" RESET);
                            //si el jugador miente, se le ignora para evitar bloqueos
                        }
                        else {
                            tengo_juego[i] = juego_humano;
                        }
                    }

                    printf(BOLDYELLOW "[maestro] Jugador %d %s tengo juego\n" RESET, i, respuestas[tengo_juego[i]]);
                    i = add_mod(i, 1, 4);
                    contador++;
                }

                if (ocurrenciasArray(tengo_juego, 4, 0) == 4) {
                    //No hay juego. Se juega al punto.
                    juego_al_punto = 1;
                    printf(BOLDBLUE "[maestro] No hay juego. Se juega al punto.\n" RESET);
                }
                    //si tiene sólo una pareja, esta ganará una piedra en el conteo al final
                else if ((tengo_juego[0] == 0) && (tengo_juego[2] == 0) &&
                         ((tengo_juego[1] == 1) || (tengo_juego[3] == 1))) {
                    ganador[l] = 1; //da igual a qué jugador le hagamos ganador, sólo importa que sea de la pareja ganadora
                    juego_al_punto = 0;
                } else if ((tengo_juego[1] == 0) && (tengo_juego[3] == 0) &&
                           ((tengo_juego[0] == 1) || (tengo_juego[2] == 1))) {
                    ganador[l] = 0; //da igual a qué jugador le hagamos ganador, sólo importa que sea de la pareja ganadora
                    juego_al_punto = 0;
                } else {
                    juego_al_punto = 2; //hay juego, se inicia fase de envites
                }
                MPI_Bcast(&juego_al_punto, 1, MPI_INT, MPI_ROOT, juego_comm);

            }

            if ((l == 0) || (l == 1) || ((l == 2) && (indicador_pares ==
                                                        2)) || ((l == 3) && (juego_al_punto !=
                                                                             0))) { //los envites sólo tienen lugar si hay pares en ambas parejas y en el resto de los lances
                // para cada jugador:
                i = 0;
                token = 1;

                /****************************************************************************************************
                 * FASE DE ENVITES
                 *****************************************************************************************************/
                printf(BOLDBLUE "[maestro] Comienza fase de envites\n" RESET);
                while (i < N_JUGADORES) {
                    envite = 0;
                    envite_N = 0;

                    //recibir envite/paso de jugador
                    //enviar token a jugador, empezando por la mano
                    token = 1;
                    printf("[maestro]: Siguiente jugador: %d, mano: %d\n", siguiente_jugador, mano);
                    MPI_Send(&token, 1, MPI_INT, siguiente_jugador, 0, juego_comm);
                    debug("[maestro] Token %d enviado con valor %d a jugador %d...", i, token, siguiente_jugador);
                    if (((modo_juego == 1) && ((siguiente_jugador == jugador_humano) ) && ((l == 0) || (l==1))) || //grande o chica
                        ((modo_juego == 1) && (siguiente_jugador == jugador_humano) && (l == 2) && (indicador_pares==2) && (tengo_pares[siguiente_jugador] == 1)) || //pares y tengo pares
                        ((modo_juego == 1) && (siguiente_jugador == jugador_humano) && (l == 3) && (juego_al_punto == 2) && (tengo_juego[siguiente_jugador] == 1)) ||  //juego y tengo juego
                        ((modo_juego == 1) && (siguiente_jugador == jugador_humano) && (l == 3) && (juego_al_punto == 1))) { //juego al punto
                        printf("[maestro] Jugador %d, estas son tus cartas:\n", jugador_humano);
                        recibir_mazo(mano_jugador, siguiente_jugador, juego_comm, N_CARTAS_MANO, MPI_STATUS_IGNORE);

                        print_mazo(mano_jugador, N_CARTAS_MANO);
                        do {
                            if (hay_apuesta(envites_jugador, N_JUGADORES) == 0) {
                                printf(BOLDMAGENTA "[jugador %d] Introduzca envite a %s: (1:paso, 2: envido, 3: envido N / Órdago)\n" RESET,
                                       siguiente_jugador,
                                       lances_etiquetas[l]);
                            } else {
                                printf(BOLDMAGENTA "[jugador %d] Introduzca envite a %s: (1:no, 2: lo quiero, 3: envido N más)\n "RESET,
                                       siguiente_jugador, lances_etiquetas[l]);
                            }
                        } while (((scanf("%d%c", &envite, &c) != 2 || c != '\n') && clean_stdin()) ||
                                 esta_valor_en_array(envite, entradas_posibles, 3) == 0);

                        if (envite == 3) { // cuántos envida
                            do {
                                if (hay_apuesta(envites_jugador, N_JUGADORES) == 0) {
                                    printf(BOLDMAGENTA "[jugador %d] ¿Cuántos envida? (introduzca un número entero) \n" RESET,
                                           siguiente_jugador);
                                } else {
                                    printf(BOLDMAGENTA "[jugador %d] ¿Cuántos más envida? (introduzca un número entero, 99 para órdago)\n" RESET,
                                           siguiente_jugador);
                                }
                            } while (((scanf("%d%c", &envite_N, &c) != 2 || c != '\n') && clean_stdin()));
                        }
                    } else {
                        MPI_Send(envites_jugador, 4, MPI_INT, siguiente_jugador, 0, juego_comm);

                        // para jugadores automáticos, se recibe desde los procesos jugadores
                        MPI_Recv(envites, 2, MPI_INT, siguiente_jugador, 0, juego_comm, MPI_STATUS_IGNORE);
                        envite = envites[0];
                        envite_N = envites[1];
                    }
                    // Hay que enviar los envites al jugador para que pueda hacer su cálculo



                    if ((l == 0) || (l == 1) || (l == 2) && (tengo_pares[siguiente_jugador] == 1) ||
                        (l == 3) && (juego_al_punto == 2) && tengo_juego[siguiente_jugador] == 1 ||
                        (l == 3) && (juego_al_punto == 1)) { //si no tengo pares o juego no hablo
                        print_envite(envite, siguiente_jugador, hay_apuesta(envites_jugador, N_JUGADORES), envite_N);
                    }

                    debug("¿HAY APUESTA?: %d\n", hay_apuesta(envites_jugador, N_JUGADORES));
                    debug("Apuesta anterior pareja postre: %d\n", envite_anterior[0]);
                    debug("Apuesta anterior pareja mano: %d\n", envite_anterior[1]);

                    envites_jugador[siguiente_jugador] = calcular_envite(envites_jugador, envite, envite_N,
                                                                         max(envite_anterior[0], envite_anterior[1]));

                    if (envites_jugador[siguiente_jugador] > 1 &&
                        envites_jugador[siguiente_jugador] >= maximo_array(envites_jugador, N_JUGADORES)) {
                        if (envites_jugador[add_mod(siguiente_jugador, 2, 4)] != 0) {
                            envite_anterior[que_pareja_soy(siguiente_jugador,
                                                           mano)] = envites_jugador[siguiente_jugador];
                        } else {
                            envite_anterior[que_pareja_soy(siguiente_jugador,
                                                           mano)] += envites_jugador[siguiente_jugador];
                        }

                    }

                   debug(BOLDBLUE "[maestro] Envites hasta el momento (0: no habla, 1: pasa, 2: envida, N: más, 99:órdago): \n [Jugador 0]: %d\n [Jugador 1]: %d\n [Jugador 2]: %d\n [Jugador 3]: %d\n" RESET,
                           envites_jugador[0], envites_jugador[1],
                           envites_jugador[2], envites_jugador[3]);


                    int j;
                    jugador_espera = siguiente_jugador;
                    for (j = 0; j < N_JUGADORES - 1; j++) {
                        jugador_espera = add_mod(jugador_espera, 1, 4);
                        token = 2;
                        MPI_Send(&token, 1, MPI_INT, jugador_espera, 0, juego_comm);

                    }
                    //publicar envites al resto de jugadores

                    MPI_Bcast(envites_jugador, 4, MPI_INT, MPI_ROOT, juego_comm);
                    siguiente_jugador = add_mod(siguiente_jugador, 1, 4);
                    i++;
                }

                int k;

                printf(BOLDBLUE "[maestro] Envites (0: no habla, 1: pasa, 2: envida, N: más, 99:órdago): \n [Jugador 0]: %d\n [Jugador 1]: %d\n [Jugador 2]: %d\n [Jugador 3]: %d\n" RESET,
                       envites_jugador[0], envites_jugador[1],
                       envites_jugador[2], envites_jugador[3]);

                debug("APUESTA: %d\n", apuesta_terminada(envites_jugador, N_JUGADORES));

                //si pareja contraria no acepta un envite, sumar una piedra (envite anterior) a la otra inmediatamente
                if (((max(envites_jugador[0], envites_jugador[2]) == 1) ||
                     (max(envites_jugador[1], envites_jugador[3]) == 1)) &&
                    (max(envites_jugador[0], envites_jugador[2]) != max(envites_jugador[1], envites_jugador[3]))) {
                    for (k = 0; k < N_JUGADORES; k++) {
                        if (envites_jugador[k] != 1) {
                            debug("[maestro] Piedra por no.\n");
                            piedras[que_pareja_soy(k, mano)]++;
                            piedra_no[l] = 1;
                            break;
                        }

                    }
                    printf(BOLDBLUE "[maestro] %s se lleva piedra de %s por el no\n" RESET,
                           parejas_etiquetas[que_pareja_etiqueta_tengo(k)], lances_etiquetas[l]);

                }

                //evaluar envites de las parejas
                //mientras apuesta no esté terminada

                /******************************************************************************************************
                 * SUBIDA DE APUESTAS
                 ******************************************************************************************************/
                int iteracion = 0;
                while (apuesta_terminada(envites_jugador, N_JUGADORES) == 0) {
                    printf(BOLDBLUE "[maestro] Han subido el envite.\n" RESET);
                    // ahora el juego de apuestas debe tener lugar entre el jugador que apostó y el que la ha subido, no los otros
                    // para ello se calcula:

                    // 1) la máxima apuesta
                    int maximo = maximo_array(envites_jugador, N_JUGADORES);
                   debug("MÁXIMA APUESTA: %d", maximo);
                    // 2) el jugador con la máxima apuesta
                    int jugador_maxima_apuesta = buscaIndice(envites_jugador, 4, maximo);
                    // 3) la pareja con el jugador con la máxima apuesta
                    int pareja_sube_apuesta = que_pareja_soy(jugador_maxima_apuesta, mano);
                    // 4) el jugador que apostó inicialmente de la otra pareja
                    int jugador_1_pareja_subida = add_mod(jugador_maxima_apuesta, 1, 4);
                    int jugador_2_pareja_subida = add_mod(jugador_maxima_apuesta, 3, 4);
                    int jugador_apuesta_inicial = buscaIndice(envites_jugador, 4,
                                                              max(envites_jugador[jugador_1_pareja_subida],
                                                                  envites_jugador[jugador_2_pareja_subida]));
                   debug("Jugador al que han subido la apuesta: %d\n", jugador_apuesta_inicial);
                    token = 1;
                    //enviar token a jugador con mayor apuesta de pareja a la que han subido la apuesta
                    MPI_Send(&token, 1, MPI_INT, jugador_apuesta_inicial, 0, juego_comm);
                    if (((modo_juego == 1) && ((jugador_apuesta_inicial == jugador_humano))  && ((l == 0) || (l==1))) || //grande o chica
                        ((modo_juego == 1) && (jugador_apuesta_inicial == jugador_humano) && (l == 2) && (tengo_pares[jugador_apuesta_inicial] == 1)) || //pares y tengo pares
                        ((modo_juego == 1) && (jugador_apuesta_inicial == jugador_humano) && (l == 3) && (juego_al_punto == 2) && (tengo_juego[jugador_apuesta_inicial] == 1)) ||  //juego y tengo juego
                        ((modo_juego == 1) && (jugador_apuesta_inicial == jugador_humano) && (l == 3) && (juego_al_punto == 1))) { //juego al punto


                        envite = 0;
                        envite_N = 0;
                        recibir_mazo(mano_jugador,jugador_apuesta_inicial, juego_comm, N_CARTAS_MANO, MPI_STATUS_IGNORE);
                        printf("[maestro] Jugador %d, estas son tus cartas:\n", jugador_apuesta_inicial);
                        print_mazo(mano_jugador, N_CARTAS_MANO);

                        do {
                            if (hay_apuesta(envites_jugador, N_JUGADORES) == 0) {
                                printf(BOLDMAGENTA "[jugador %d] Introduzca envite a %s: (1:paso, 2: envido, 3: envido N)\n" RESET,
                                       jugador_apuesta_inicial, lances_etiquetas[l]);
                            } else {
                                printf(BOLDMAGENTA "[jugador %d] Introduzca envite a %s: (1:no, 2: lo quiero, 3: envido N más)\n" RESET,
                                       jugador_apuesta_inicial, lances_etiquetas[l]);
                            }
                        } while (((scanf("%d%c", &envite, &c) != 2 || c != '\n') && clean_stdin()) ||
                                 esta_valor_en_array(envite, entradas_posibles, 3) == 0);

                        if (envite == 3) { // cuántos envida
                            do {
                                if (hay_apuesta(envites_jugador, N_JUGADORES) == 0) {
                                    printf(BOLDMAGENTA "[jugador %d] ¿Cuántos envida? (introduzca un número entero) \n" RESET,
                                           jugador_apuesta_inicial);
                                } else {
                                    printf(BOLDMAGENTA "[jugador %d] ¿Cuántos más envida? (introduzca un número entero)\n" RESET,
                                           jugador_apuesta_inicial);
                                }
                            } while (((scanf("%d%c", &envite_N, &c) != 2 || c != '\n') && clean_stdin()));
                        }

                    } else { //modo automático
                       debug("[maestro] MODO AUTOMÁTICO\n");

                        //recibir envite/paso de jugador
                        MPI_Send(envites_jugador, 4, MPI_INT, jugador_apuesta_inicial, 0, juego_comm);
                        MPI_Recv(envites, 2, MPI_INT, jugador_apuesta_inicial, 0, juego_comm, MPI_STATUS_IGNORE);
                        envite = envites[0];
                        envite_N = envites[1];
                    }
                    print_envite(envite, jugador_apuesta_inicial, hay_apuesta(envites_jugador, N_JUGADORES), envite_N);
                    //evaluar nuevo envite de la pareja y actualizar envites
                    envites_jugador[jugador_apuesta_inicial] = calcular_envite(envites_jugador, envite, envite_N,
                                                                               max(envite_anterior[0],
                                                                                   envite_anterior[1]));
                    if (envites_jugador[jugador_apuesta_inicial] > 1 &&
                        envites_jugador[jugador_apuesta_inicial] >= maximo_array(envites_jugador, N_JUGADORES)) {
                        if (envites_jugador[add_mod(jugador_apuesta_inicial, 2, 4)] !=
                            0) { //si el otro miembro de la pareja ya ha envidado, sustituir por este que es mayor
                            envite_anterior[que_pareja_soy(jugador_apuesta_inicial,
                                                           mano)] = envites_jugador[jugador_apuesta_inicial];
                        } else { //si no, sumar el envite a la apuesta anterior
                            envite_anterior[que_pareja_soy(jugador_apuesta_inicial,
                                                           mano)] += envites_jugador[jugador_apuesta_inicial];
                        }

                    }

                    int j;
                    siguiente_jugador = 0;
                    for (j = 0; j < N_JUGADORES; j++) {
                        token = 3;
                        MPI_Send(&token, 1, MPI_INT, j, 0, juego_comm);

                    }
                    MPI_Bcast(envites_jugador, 4, MPI_INT, MPI_ROOT, juego_comm);
                    printf(BOLDBLUE "[maestro] Envites (0: no habla, 1: pasa, 2: envida, N: más, 99:órdago): \n [Jugador 0]: %d\n [Jugador 1]: %d\n [Jugador 2]: %d\n [Jugador 3]: %d\n" RESET,
                           envites_jugador[0], envites_jugador[1],
                           envites_jugador[2], envites_jugador[3]);
                    //si pareja contraria no acepta un envite, sumar (envite anterior) a la otra inmediatamente
                    if (((max(envites_jugador[0], envites_jugador[2]) == 1) ||
                         (max(envites_jugador[1], envites_jugador[3]) == 1)) &&
                        (max(envites_jugador[0], envites_jugador[2]) != max(envites_jugador[1], envites_jugador[3]))) {
                        for (k = 0; k < N_JUGADORES; k++) {
                            if (envites_jugador[k] != 1) {
                                debug("[maestro] Piedras por no: %d\n", envite_anterior[que_pareja_soy(add_mod(k,1,4), mano)]);
                                piedras[que_pareja_soy(k, mano)] += envite_anterior[que_pareja_soy(add_mod(k,1,4), mano)];
                                piedra_no[l] = 1;
                                break;
                            }

                        }
                        printf(BOLDBLUE "[maestro] %s se lleva %d piedras de %s por el no\n" RESET,
                               parejas_etiquetas[que_pareja_etiqueta_tengo(k)], envite_anterior[que_pareja_soy(add_mod(k,1,4), mano)], lances_etiquetas[l]);

                    }


                    printf(BOLDBLUE "[maestro] Fin de ronda de apuestas\n" RESET);
                    iteracion++;
                }//apuesta terminada
                int j;
                siguiente_jugador = 0;

                for (j = 0; j < N_JUGADORES; j++) {
                    token = 2;
                    debug("[maestro] Enviado token %d a jugador %d\n", token, j);
                    MPI_Send(&token, 1, MPI_INT, j, 0, juego_comm);

                }
            }

            //resultado: comparar cartas de una pareja despecto de la otra
            /**********************************************************************************************************
             * EVALUACIÓN DE MANOS
             **********************************************************************************************************/
            switch (l) {
                case 0:
                    //grande
                    /* Recepción de datos para evaluar las manos de los jugadores */
                    MPI_Gather(conteos, 10, MPI_INT, rbuf, 10, MPI_INT, MPI_ROOT, juego_comm);

                    /*cálculo de manos*/

                    ganador[l] = calcula_grande(rbuf, mano);
                    break;
                case 1:
                    //chica
                    MPI_Gather(conteos, 10, MPI_INT, rbuf, 10, MPI_INT, MPI_ROOT, juego_comm);

                    /*cálculo de manos*/

                    ganador[l] = calcula_chica(rbuf, mano);
                    break;
                case 2:
                    //pares
                    if (indicador_pares == 2) {
                        MPI_Gather(conteos, 5, MPI_INT, paresbuf, 5, MPI_INT, MPI_ROOT, juego_comm);
                        contador = 0;
                        ganador[l] = calcular_pares(paresbuf, mano);
                    }
                    break;
                case 3:
                    //juego
                    if (juego_al_punto != 0) {
                        MPI_Gather(conteos, 1, MPI_INT, juegobuf, 1, MPI_INT, MPI_ROOT, juego_comm);
                        ganador[l] = calcularJuego(juegobuf, mano);

                    }


            }

            /**********************************************************************************************************
             * CONTEOS
             **********************************************************************************************************/
            // Cálculo de piedras ¡
            if ((l == 2) && (indicador_pares == 0)) {
                //lance de pares, ninguna pareja tiene pares
                //no se hace nada
                printf(BOLDBLUE "[maestro] Ninguna pareja se lleva piedra de pares\n" RESET);

            } else if ((l == 2) && (indicador_pares == 1)) {
                //lance de pares, solo una pareja tiene pares; se lleva una piedra
                piedras[que_pareja_soy(ganador[l], mano)]++;
                printf(BOLDBLUE "[maestro] %s se lleva la piedra de pares\n" RESET,
                       parejas_etiquetas[que_pareja_etiqueta_tengo(ganador[l])]);
            } else if ((l == 3) && (juego_al_punto == 0)) {
                //lance de juego, sólo una pareja tiene juego; se lleva una piedra
                piedras[que_pareja_soy(ganador[l], mano)]++;
                printf(BOLDBLUE "[maestro] %s se lleva la piedra de juego\n" RESET,
                       parejas_etiquetas[que_pareja_etiqueta_tengo(ganador[l])]);
            } else { //resto de casos: se calculan las piedras en base a envites
                if (piedra_no[l] != 1) {
                    printf("[maestro] Calculando piedras, ninguna previa...\n");
                    // Calcular envite de pareja mano em
                    em = envite_pareja(1, mano, envites_jugador);
                    debug("[maestro] Envite final de pareja mano ha sido: %d\n", em);
                    debug("[maestro] Envite anterior de pareja mano ha sido: %d\n", envite_anterior[1]);
                    // Calcular envite de pareja postre ep
                    ep = envite_pareja(0, mano, envites_jugador);
                    debug("[maestro] Envite final de pareja postre ha sido: %d\n", ep);
                    debug("[maestro] Envite anterior de pareja postre ha sido: %d\n", envite_anterior[0]);
                    // em == ep !=1? :
                    if ((em == ep) && (em == 99)) {
                        printf(BOLDRED "[maestro] Ha habido ÓRDAGO; gana el juego la pareja %s\n" RESET,
                               parejas_etiquetas[que_pareja_etiqueta_tengo(ganador[l])]);
                        piedras[0] = 0;
                        piedras[1] = 0;
                        piedras_parejas[0] = 0;
                        piedras_parejas[1] = 0;
                        puntos_juego[0]=0;
                        puntos_juego[1]=0;
                        n_juegos[que_pareja_inicial_soy(ganador[l])] += 1;
                        indicador_ordago = 1;

                    } else if ((em == ep) &&
                               (em != 1) && (em !=
                                             99)) { //apuesta igualada, pareja del ganador se lleva el envite (todas las piedras)
                        piedras[que_pareja_soy(ganador[l], mano)] += (2 * em);

                    }
                    else if ((em !=
                              ep)) { // una pareja pasa y la otro ha envidado 2 o más; envite más alto se lleva 1 tanto


                        if (em > ep) {
                            piedras[1] += envite_anterior[0];
                        } else {
                            piedras[0] += envite_anterior[1];
                        }
                    }

                        // todos pasan: pareja a la que pertenece la jugada ganadora se lleva una piedra
                    else if ((em == ep) && (ep == 1)) {
                        piedras[que_pareja_soy(ganador[l], mano)]++;
                    }
                }
            }
            //reinicializar variables
            envite_anterior[0] = 0;
            envite_anterior[1] = 0;
            envites[0] = 0;
            envites[1] = 0;
            for (i = 0; i < N_JUGADORES; i++) {
                envites_jugador[i] = 0;
            }
            MPI_Bcast(&indicador_ordago, 1, MPI_INT, MPI_ROOT, juego_comm);
            if (indicador_ordago == 1) {
                break; //salir de lances
            }
        }

        if (indicador_ordago == 0) { //no hacer conteos si se ha resuelto por órdago
            for (l = 0; l < N_LANCES; l++) {

                if (((l == 0) && (piedra_no[l] != 1)) || ((l == 1) && (piedra_no[l] != 1)) ||
                    ((l == 2) && (indicador_pares == 2) && (piedra_no[l] != 1)) ||
                    ((l == 3) && (juego_al_punto != 0) && (piedra_no[l] != 1))) {
                    printf(BOLDBLUE "[maestro] Mejor mano a %s: jugador %d\n" RESET, lances_etiquetas[l], ganador[l]);
                    printf(BOLDBLUE "[maestro] Mejor pareja a %s: %s\n" RESET, lances_etiquetas[l],
                           parejas_etiquetas[que_pareja_etiqueta_tengo(ganador[l])]);
                }
            }

            printf(BOLDBLUE "[maestro] PIEDRAS GANADAS POR PAREJA MANO: %d\n" RESET, piedras[1]);
            printf(BOLDBLUE "[maestro] PIEDRAS GANADAS POR PAREJA POSTRE: %d\n" RESET, piedras[0]);

            if (que_pareja_inicial_soy(mano) == 0) {
                piedras_parejas[0] += piedras[1];
                piedras_parejas[1] += piedras[0];
            } else {
                piedras_parejas[0] += piedras[0];
                piedras_parejas[1] += piedras[1];
            }
            printf(BOLDBLUE "[maestro] PIEDRAS PAREJA 0-2: %d\n" RESET, piedras_parejas[0]);
            printf(BOLDBLUE "[maestro] PIEDRAS PAREJA 1-3: %d\n" RESET, piedras_parejas[1]);

            puntos_juego[0] = piedras_parejas[0];
            puntos_juego[1] = piedras_parejas[1];
        }



        //si una pareja gana cuarenta puntos o más, gana un juego
        if (puntos_juego[0] >= n_puntos_juego) {
            printf(BOLDRED "[maestro] GANA JUEGO PAREJA 0-2\n" RESET);
            n_juegos[0]++;
            piedras_parejas[0] = 0;
            puntos_juego[0] = 0;
            piedras_parejas[1] = 0;
            puntos_juego[1] = 0;
        } else if (puntos_juego[1] >= n_puntos_juego) {
            printf(BOLDRED "[maestro] GANA JUEGO PAREJA 1-3\n" RESET);
            n_juegos[1]++;
            piedras_parejas[1] = 0;
            puntos_juego[1] = 0;
            piedras_parejas[0] = 0;
            puntos_juego[0] = 0;
        }
        ronda++;

        //si una pareja es la mejor a N juegos, entonces gana una vaca
        if (n_juegos[0] == ((n_juegos_vaca / 2) + 1)) {
            printf(BOLDRED "[maestro] GANA VACA PAREJA 0-2\n" RESET);
            n_vacas[0]++;
            n_juegos[0] = 0;
            piedras_parejas[0] = 0;
            puntos_juego[0] = 0;
            piedras_parejas[1] = 0;
            puntos_juego[1] = 0;
            ronda = 0;
        } else if (n_juegos[1] == ((n_juegos_vaca / 2) + 1)) {
            printf(BOLDRED "[maestro] GANA VACA PAREJA 1-3\n" RESET);
            n_vacas[1]++;
            n_juegos[1] = 0;
            piedras_parejas[1] = 0;
            puntos_juego[1] = 0;
            piedras_parejas[0] = 0;
            puntos_juego[0] = 0;
            ronda = 0;
        }

        MPI_Bcast(puntos_juego, 2, MPI_INT, MPI_ROOT, juego_comm);

        //reinicialización de variables para la próxima partida
        piedras[0] = 0;
        piedras[1] = 0;
        envite_anterior[0] = 0;
        envite_anterior[1] = 0;
        envites[0] = 0;
        envites[1] = 0;
        for (i = 0; i < N_JUGADORES; i++) {
            envites_jugador[i] = 0;
        }
        printf(BOLDBLUE "*******************************************\n" RESET);
        printf(BOLDBLUE "PUNTUACIONES:\n"RESET);
        printf(BOLDBLUE "*******************************************\n"RESET);
        printf(BOLDBLUE "PAREJA 0-2\n"RESET);
        printf(BOLDBLUE "RONDA: %d amarracos y %d piedras\n" RESET, puntos_juego[0] / 5, puntos_juego[0] % 5);
        printf(BOLDBLUE "JUEGOS: %d\n"RESET, n_juegos[0]);
        printf(BOLDBLUE "VACAS: %d\n"RESET, n_vacas[0]);
        printf(BOLDBLUE "*******************************************\n");
        printf(BOLDBLUE "PAREJA 1-3\n"RESET);
        printf(BOLDBLUE "RONDA: %d amarracos y %d piedras\n"RESET, puntos_juego[1] / 5, puntos_juego[1] % 5);
        printf(BOLDBLUE "JUEGOS: %d\n"RESET, n_juegos[1]);
        printf(BOLDBLUE "VACAS: %d\n"RESET, n_vacas[1]);
        printf(BOLDBLUE "*******************************************\n");


        if ((n_vacas[0] == ((n_vacas_partida / 2) + 1))) {
            // fin de partida
            fin_partida = 1;
            printf(BOLDRED "¡¡GANA LA PARTIDA LA PAREJA 0-2!!\n" RESET);
            MPI_Bcast(&fin_partida, 1, MPI_INT, MPI_ROOT, juego_comm);
        } else if ((n_vacas[1] == ((n_vacas_partida / 2) + 1))) {
            // fin de partida
            fin_partida = 1;
            printf(BOLDRED "¡¡GANA LA PARTIDA LA PAREJA 1-3!!\n" RESET);
            MPI_Bcast(&fin_partida, 1, MPI_INT, MPI_ROOT, juego_comm);

        } else {
            //sigue partida
            fin_partida = 0;
            MPI_Bcast(&fin_partida, 1, MPI_INT, MPI_ROOT, juego_comm);
        }

        if ((em == ep) && (em == 99)) {
            ronda = 0;
            indicador_ordago = 0;
        }
        if (modo_juego == 1) {
            printf(BOLDBLUE "[maestro] Presione cualquier tecla para continuar...\n" RESET);
            getchar();
        }

    } //while fin_partida != 0
    printf(BOLDMAGENTA "[maestro] FINALIZADO!\n" RESET);


    int token_end = 0;
    for (i = 0; i < N_JUGADORES; i++) {
        MPI_Recv(&token_end, 1, MPI_INT, i, 0, juego_comm, MPI_STATUS_IGNORE);
        if (token_end == 1) {
            printf(BOLDYELLOW "[jugador %d] FINALIZADO\n" RESET, i);
        }
    }
    //MPI_Comm_disconnect(&juego_comm);
    MPI_Finalize();
    return 0;
}
