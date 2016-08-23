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
#define MODO_JUEGO 0


extern const char * caras[];
extern const char * palos[];
extern const char * lances_etiquetas[];
extern const char * parejas[];
extern int valores[];
extern int equivalencias[];



int main(int argc, char **argv) {
    /*
     * DECLARACIÓN E INICIALIZACIÓN DE VARIABLES
     */
    int rank, size, version, subversion, namelen, universe_size, size_mazo, size_mano, proceso, jugador_humano, corte, repartidor, postre, mano,
            ultimo, siguiente_jugador, jugador_espera, mus, token, descarte, repartidor_descartes, turno, n_cartas_a_descartar, envite, envite_N, envite_vigor, em, ep;
    char processor_name[MPI_MAX_PROCESSOR_NAME], worker_program[100], c;
    int cartas_a_descartar[N_CARTAS_MANO];
    //Array de 4 posiciones para los envites, una para cada jugador
    //0: no ha hablado
    //1: paso
    //2: envido (2 piedras, apuesta mínima)
    //3-99: envido N piedras
    int envites_grande[N_JUGADORES] = {0,0,0,0};
    int entradas_posibles[3] = {1, 2, 3};
    int entradas_posibles_mus[2] = {0, 1};
    int piedras[N_PAREJAS] = {0, 0};
    int envites[2] = {0,0};
    int conteos[10], rbuf[50]; //buffers para recibir jugadas
    int ganador[N_LANCES]; // buffer para almacenar ganadores de cada lance
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

    if (MODO_JUEGO==1) {
        //jugador_humano = rand() % (N_JUGADORES + 1 - 0) + 0;
        jugador_humano = 3;
        printf("El identificador para el jugador humano es: %d\n", jugador_humano);

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

        if ((MODO_JUEGO == 1) && (siguiente_jugador == 3)) {
            token = 5;
            MPI_Send(&token, 1, MPI_INT, siguiente_jugador, 0, juego_comm);
            recibir_mazo(mano_jugador, siguiente_jugador, juego_comm, N_CARTAS_MANO, MPI_STATUS_IGNORE);
            printf("[maestro] Mano del jugador %d\n", siguiente_jugador);
            print_mazo(mano_jugador, N_CARTAS_MANO);
            debug("Envío de mazo a jugador %d", siguiente_jugador);
            enviar_mazo(mazo, siguiente_jugador, juego_comm, N_CARTAS_MAZO);
            do {
                printf("[jugador %d] ¿Quieres mus?: (0:Dame mus, 1:no hay mus)\n", siguiente_jugador);
                }
            while (((scanf("%d%c", &mus, &c) != 2 || c!= '\n') && clean_stdin()) || esta_valor_en_array(mus, entradas_posibles_mus, 2) == 0);
            MPI_Send(&mus, 1, MPI_INT, siguiente_jugador, 0, juego_comm);
        }
        else {
            token = 1;
            MPI_Send(&token, 1, MPI_INT, siguiente_jugador, 0, juego_comm);
            recibir_mazo(mano_jugador, siguiente_jugador, juego_comm, N_CARTAS_MANO, MPI_STATUS_IGNORE);
            printf("[maestro] Mano del jugador %d\n", siguiente_jugador);
            print_mazo(mano_jugador, N_CARTAS_MANO);
            debug("Envío de mazo a jugador %d", siguiente_jugador);
            enviar_mazo(mazo, siguiente_jugador, juego_comm, N_CARTAS_MAZO);
            // El jugador decide si quiere mus

            MPI_Recv(&mus, 1, MPI_INT, siguiente_jugador, 0, juego_comm, MPI_STATUS_IGNORE);
        }

    if (mus == 1) { // corta el mus
        // En caso de no querer mus, ese jugador es mano y el anterior postre, al que habrá que pasar el mazo
        printf("[jugador %d] No hay mus\n", siguiente_jugador);
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
        printf("[jugador %d] Quiero mus\n", siguiente_jugador);
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
            MPI_Send(&size_mazo, 1, MPI_INT, siguiente_jugador, 0, juego_comm);
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
                else {
                    printf("[maestro] LE TOCA AL REPARTIDOR\n");
                }

                // TODO: gestionar entrada de descartes con jugador humano
                /*
                if ((MODO_JUEGO==1) && siguiente_jugador==3) {

                    //enviar token a jugador para que envíe mano
                    //recoger mano del jugador

                    printf("¿Desea descartar %s de %s? (0: no, 1: sí)\n", manoJugadorHumano[i].cara,
                           manoJugadorHumano[i].palo);
                    if (descarte == 1) {
                        //añadir id a lista
                    }
                }*/

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
                MPI_Recv(&size_mazo, 1, MPI_INT, repartidor_descartes, 0, juego_comm, MPI_STATUS_IGNORE);

                for ( j = 0; j < n_cartas_a_descartar; j++) {


                    descartada = recibir_carta(repartidor_descartes, juego_comm, MPI_STATUS_IGNORE);
                    repartir_carta(descartada, siguiente_jugador, juego_comm);
                    /*size_mazo--;
                    if (size_mazo == 0) {
                        recibir_mazo(mazo, repartidor_descartes, juego_comm, N_CARTAS_MAZO, MPI_STATUS_IGNORE);
                        print_mazo(mazo, N_CARTAS_MAZO);
                        size_mazo=N_CARTAS_MAZO;
                    }*/

            }
                debug("[maestro] Fin del reparto de cartas\n");
                i++;

            } //fin while descartes
            MPI_Recv(&size_mazo, 1, MPI_INT, repartidor_descartes, 0, juego_comm, MPI_STATUS_IGNORE);
            recibir_mazo(mazo, repartidor_descartes, juego_comm, N_CARTAS_MAZO, MPI_STATUS_IGNORE);
           // printf("VECTOR DE ESTADOS: \n");
           // print_vector_estados(mazo, N_CARTAS_MAZO);
           // printf("Cursor size_mazo: %d\n", size_mazo);
           // printf ( "Press [Enter] to continue . . ." );
            //fflush ( stdout );
            //getchar();
        } // fin else cuando no se corta el mus y acaba la ronda
    } //fin else cuando jugador pide mus
} // fin while mus corrido (se pide mus)
    //TODO: comprobar estados de las cartas del mazo devuelto
printf("[maestro] Enviando mano...\n");
//Se comunica a todos los jugadores quien es la mano
    MPI_Bcast(&mano, 1, MPI_INT, MPI_ROOT, juego_comm);

printf("[maestro] Manos con las que se juega la partida: \n");
    siguiente_jugador=mano;
    for(i=0;i<4;i++){
        recibir_mazo(mano_jugador, siguiente_jugador, juego_comm, N_CARTAS_MANO, MPI_STATUS_IGNORE);
        printf("[maestro] Mano del jugador %d\n", siguiente_jugador);
        print_mazo(mano_jugador, N_CARTAS_MANO);
        siguiente_jugador = add_mod(siguiente_jugador, 1, 4);
    }

    /*
     * FASE DE LANCES
     */

    // cada ronda cuatro lances

    //token = 1 : evaluar + decidir envite
    //token = 2 : esperar a ver qué dicen los otros
    //token = 3 : apuesta igualada




    // GRANDE

     printf("[maestro] INICIANDO LANCES...\n");
    //TODO: GUARDAR APUESTA ANTERIOR PARA CONTEO DE PIEDRAS

    i=0;
    token=1;

    envite_vigor=0;
    siguiente_jugador=mano;
    // para cada jugador:
    while(i<N_JUGADORES) {
        envite=0;
        envite_N=0;

        //recibir envite/paso de jugador

        // si modo interactivo y el siguiente jugador es humano
       // if ((MODO_JUEGO == 1) && (siguiente_jugador == jugador_humano)) {
        if ((MODO_JUEGO == 1) && ((siguiente_jugador == 3) || siguiente_jugador == 2)) {
            token=3;
            MPI_Send(&token, 1, MPI_INT, siguiente_jugador, 0, juego_comm);
            do {
                if(hay_apuesta(envites_grande, N_JUGADORES) == 0) {
                    printf("[jugador %d] Introduzca envite a %s: (1:paso, 2: envido, 3: envido N)\n", siguiente_jugador, lances_etiquetas[0]);
                }
                else {
                    printf("[jugador %d] Introduzca envite a %s: (1:no, 2: lo quiero, 3: envido N más)\n", siguiente_jugador, lances_etiquetas[0]);
                }
            }
            while (((scanf("%d%c", &envite, &c) != 2 || c!= '\n') && clean_stdin()) || esta_valor_en_array(envite, entradas_posibles, 3) == 0);

            if (envite == 3) { // cuántos envida
                do {
                    if(hay_apuesta(envites_grande, N_JUGADORES) == 0) {
                        printf("[jugador %d] ¿Cuántos envida? (introduzca un número entero) \n", siguiente_jugador);
                    }
                    else {
                        printf("[jugador %d] ¿Cuántos más envida? (introduzca un número entero)\n", siguiente_jugador);
                    }
                }
                while (((scanf("%d%c", &envite_N, &c) != 2 || c!= '\n') && clean_stdin()));
            }

        }
        else {
            //enviar token a jugador, empezando por la mano
            token=1;
            MPI_Send(&token, 1, MPI_INT, siguiente_jugador, 0, juego_comm);
            debug("[maestro] Token %d enviado...", i);

            // Hay que enviar los envites al jugador para que pueda hacer su cálculo
            MPI_Send(envites_grande, 4, MPI_INT, siguiente_jugador, 0, juego_comm);

            // para jugadores automáticos, se recibe desde los procesos jugadores
            MPI_Recv(envites, 2, MPI_INT, siguiente_jugador, 0, juego_comm, MPI_STATUS_IGNORE);
            envite = envites[0];
            envite_N = envites[1];
        }


        print_envite(envite, siguiente_jugador, hay_apuesta(envites_grande, N_JUGADORES), envite_N);

        printf("¿HAY APUESTA?: %d\n", hay_apuesta(envites_grande, N_JUGADORES));
        printf("APUESTA EN VIGOR: %d\n", envite_vigor);

        envites_grande[siguiente_jugador] = calcular_envite(envites_grande, envite, envite_N, envite_vigor);

        if ( envites_grande[siguiente_jugador] > 1 && envites_grande[siguiente_jugador] >= maximo_array(envites_grande, N_JUGADORES)) {
            if (envite == 3) {
                envite_vigor += envite_N;
            }
            else {
                envite_vigor = envite;
            }
        }

        printf("[maestro] Array de envites: %d, %d, %d, %d\n",  envites_grande[0], envites_grande[1],
               envites_grande[2], envites_grande[3]);

        int j;
        jugador_espera = siguiente_jugador;
        for (j=0; j< N_JUGADORES-1;j++) {
            jugador_espera = add_mod(jugador_espera, 1, 4);
            token = 2;
            MPI_Send(&token, 1, MPI_INT, jugador_espera, 0, juego_comm);

        }
        //publicar envites al resto de jugadores

        MPI_Bcast(envites_grande, 4, MPI_INT, MPI_ROOT, juego_comm);
        siguiente_jugador = add_mod(siguiente_jugador, 1, 4);
        i++;
    }
    int k;

        printf("[maestro] Array de envites: %d, %d, %d, %d\n",  envites_grande[0], envites_grande[1],
               envites_grande[2], envites_grande[3]);

    printf("APUESTA: %d\n",apuesta_terminada(envites_grande, N_JUGADORES));
    //evaluar envites de las parejas
    //mientras apuesta no esté terminada

    //TODO: para probar esto hay que usar modo interactivo, porque con la lógica actual nunca se dará esta situación
    while (apuesta_terminada(envites_grande, N_JUGADORES) == 0) {
        printf("HAN SUBIDO LA APUESTA!\n");
        // ahora el juego de apuestas debe tener lugar entre el jugador que apostó y el que la ha subido, no los otros
        // para ello se calcula:

        // 1) la máxima apuesta
        int maximo = maximo_array(envites_grande, N_JUGADORES);
        debug("MÁXIMA APUESTA: %d", maximo);
        // 2) el jugador con la máxima apuesta
        int jugador_maxima_apuesta = buscaIndice(envites_grande, 4, maximo);
        // 3) la pareja con el jugador con la máxima apuesta
        int pareja_sube_apuesta = que_pareja_soy(jugador_maxima_apuesta, mano);
        // 4) el jugador que apostó inicialmente de la otra pareja
        int jugador_1_pareja_subida = add_mod(jugador_maxima_apuesta, 1, 4);
        int jugador_2_pareja_subida = add_mod(jugador_maxima_apuesta, 3, 4);
        int jugador_apuesta_inicial = buscaIndice(envites_grande, 4, max(envites_grande[jugador_1_pareja_subida], envites_grande[jugador_2_pareja_subida]));
        printf("Jugador al que han subido la apuesta: %d\n", jugador_apuesta_inicial);

        //enviar token a jugador con mayor apuesta de pareja a la que han subido la apuesta
        //if ((MODO_JUEGO == 1) && (jugador_1_pareja_subida == jugador_humano)) {
        if ((MODO_JUEGO == 1) && ((jugador_apuesta_inicial == 3) || jugador_apuesta_inicial == 2)){

            envite=0;
            envite_N=0;

            do {
                if(hay_apuesta(envites_grande, N_JUGADORES) == 0) {
                    printf("[jugador %d] Introduzca envite a %s: (1:paso, 2: envido, 3: envido N)\n", jugador_apuesta_inicial, lances_etiquetas[0]);
                }
                else {
                    printf("[jugador %d] Introduzca envite a %s: (1:no, 2: lo quiero, 3: envido N más)\n", jugador_apuesta_inicial, lances_etiquetas[0]);
                }
            }
            while (((scanf("%d%c", &envite, &c) != 2 || c!= '\n') && clean_stdin()) || esta_valor_en_array(envite, entradas_posibles, 3) == 0);

            if (envite == 3) { // cuántos envida
                do {
                    if(hay_apuesta(envites_grande, N_JUGADORES) == 0) {
                        printf("[jugador %d] ¿Cuántos envida? (introduzca un número entero) \n", jugador_apuesta_inicial);
                    }
                    else {
                        printf("[jugador %d] ¿Cuántos más envida? (introduzca un número entero)\n", jugador_apuesta_inicial);
                    }
                }
                while (((scanf("%d%c", &envite_N, &c) != 2 || c!= '\n') && clean_stdin()));
            }

        }

        else { //modo automático
            token = 1;
            MPI_Send(&token, 1, MPI_INT, jugador_apuesta_inicial, 0, juego_comm);
            //recibir envite/paso de jugador

            MPI_Recv(envites, 2, MPI_INT,jugador_apuesta_inicial, 0, juego_comm, MPI_STATUS_IGNORE);
            envite = envites[0];
            envite_N = envites[1];
        }
            print_envite(envite, jugador_apuesta_inicial, hay_apuesta(envites_grande, N_JUGADORES), envite_N);
        //evaluar nuevo envite de la pareja y actualizar envites
        envites_grande[jugador_apuesta_inicial] = calcular_envite(envites_grande, envite, envite_N, envite_vigor);
        if ( envites_grande[jugador_apuesta_inicial] > 1 && envites_grande[jugador_apuesta_inicial] >= maximo_array(envites_grande, N_JUGADORES)) {
            if (envite == 3) {
                envite_vigor += envite_N;

            }
            else {
                envite_vigor = envite;
            }
        }

        int j;
        siguiente_jugador = 0;
        for (j=0; j< N_JUGADORES;j++) {
            token = 3;
            MPI_Send(&token, 1, MPI_INT, j, 0, juego_comm);

        }
        MPI_Bcast(envites_grande, 4, MPI_INT, MPI_ROOT, juego_comm);
        printf("[maestro] Array de envites: %d, %d, %d, %d\n",  envites_grande[0], envites_grande[1],
               envites_grande[2], envites_grande[3]);
        printf("Fin de ronda de apuestas\n");
    }

    printf("APUESTA TERMINADA...\n");
    int j;
    siguiente_jugador = 0;

    for (j=0; j< N_JUGADORES;j++) {
        token = 2;
        printf("[maestro] Enviado token %d a jugador %d\n", token, j);
        MPI_Send(&token, 1, MPI_INT, j, 0, juego_comm);

    }

    // CHICA
    // PARES
    // JUEGO
   // debug("TAMAÑO DEL MAZO DESPUÉS DE DESCARTES: %d", size_mazo);
    //resultado: comparar cartas de una pareja despecto de la otra

    /* Recepción de datos para evaluar las manos de los jugadores */
    MPI_Gather(conteos, 10, MPI_INT, rbuf, 10, MPI_INT, MPI_ROOT, juego_comm);
    int contador = 0;
    for (i = 0; i<40;i++) {
        contador++;
        printf("%d ", rbuf[i]);
        if (contador == 10) {
            contador = 0;
            printf("\n");
        }
    }

    /*cálculo de manos*/

    ganador[0] = calculaGrande(rbuf, mano);
    printf("[maestro] Mejor mano a grande: jugador %d\n", ganador[0]);
    printf("[maestro] Mejor pareja a grande: %s\n", parejas[que_pareja_soy(ganador[0], mano)]);

    // Cálculo de piedras

    // Calcular envite de pareja mano em
    em = envite_pareja(1, mano, envites_grande);
    printf("[maestro] Envite de pareja mano ha sido: %d\n", em);
    // Calcular envite de pareja postre ep
    ep = envite_pareja(0, mano, envites_grande);
    printf("[maestro] Envite de pareja postre ha sido: %d\n", ep);
    // em == ep !=1? :
    if ( (em == ep) && (em != 1 )) { //apuesta igualada, pareja del ganador se lleva el envite
        piedras[que_pareja_soy(ganador[0], mano)] +=  em;

    }
    // em != ep && min(em,ep) !=1 ?
    else if ((em != ep) && (min(em,ep) != 1)) { //alguien subió la apuesta y se rajó; envite más alto se lleva min(em,ep)
        if (em > ep) {
            piedras[1] += min(em, ep);
        }
        else {
            piedras[0] += min(em, ep);
        }

    }

    // em != ep && min(em,ep) == 1 ?
    else if ((em != ep) && (min(em,ep) == 1)) { // una pareja pasa y la otro ha envidado 2 o más; envite más alto se lleva 1 tanto
        if (em > ep) {
            piedras[1] += 1;
        }
        else {
            piedras[0] += 1;
        }
    }

    // TODO em == ep == 1? : todos pasan: la mejor mano se lleva una piedra

    printf("[maestro] PIEDRAS MANO: %d\n", piedras[1]);
    printf("[maestro] PIEDRAS POSTRE: %d\n", piedras[0]);

   printf("[maestro] FINALIZADO!\n");
    /*free(mazo->palo);
    free(mazo->cara);*/

    //MPI_Comm_disconnect(&juego_comm);
    //MPI_Barrier(juego_comm);
    MPI_Finalize();
    return 0;
}