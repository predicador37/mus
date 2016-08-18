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
#define MODO_JUEGO 1


extern const char * caras[];
extern const char * palos[];
extern const char * lances_etiquetas[];
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
    int piedras[N_PAREJAS] = {0, 0};
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
        printf("[jugador %d] Corta mus\n", siguiente_jugador);
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
    //TODO: CAMBIAR ENTRADA PARA PEDIR N MÁS
    i=0;
    token=1;
    envite=0;
    envite_N=0;
    envite_vigor=0;
    siguiente_jugador=mano;
    // para cada jugador:
    while(i<N_JUGADORES) {


        //recibir envite/paso de jugador

        // si modo interactivo y el siguiente jugador es humano
        if ((MODO_JUEGO == 1) && (siguiente_jugador == jugador_humano)) {
            token=3;
            MPI_Send(&token, 1, MPI_INT, siguiente_jugador, 0, juego_comm);
            do {
                if(hay_apuesta(envites_grande, N_JUGADORES) == 0) {
                    printf("Introduzca envite a %s: (1:paso, 2: envido, 3: envido N)\n", lances_etiquetas[0]);
                }
                else {
                    printf("Introduzca envite a %s: (1:no, 2: envido más, 3: envido N más)\n", lances_etiquetas[0]);
                }
            }
            while (((scanf("%d%c", &envite, &c) != 2 || c!= '\n') && clean_stdin()));

            if (envite == 2) { // cuántos envida
                do {
                    if(hay_apuesta(envites_grande, N_JUGADORES) == 0) {
                        printf("¿Cuántos envida? (introduzca un número entero) \n");
                    }
                    else {
                        printf("¿Cuántos más envida? (introduzca un número entero)\n");
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
            MPI_Recv(&envite, 1, MPI_INT, siguiente_jugador, 0, juego_comm, MPI_STATUS_IGNORE);
        }

        if (envite > 1 && envite > maximo_array(envites_grande, N_JUGADORES)) {
            envite_vigor = envite;
        }
        //TODO 18/08/2016 continuar desde aquí
        print_envite(envite, siguiente_jugador, hay_apuesta(envites_grande, N_JUGADORES));
        // si no hay apuesta en vigor y envite_N == 0, el envite es envite
        envites_grande[siguiente_jugador] = envite;
        // si no hay apuesta en vigor y envite_N != 0, el envite es envite_N
        // si hay apuesta en vigor y envite_N != 0, el envite es: apuesta_vigor + envite_N
        //si hay apuesta en vigor y envite_N ==0 y envite = 1, el envite es: apuesta_vigor
        // si hay apuesta en vigor y envite_N == 0 y envite = 0, el envite es: 1

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


    //evaluar envites de las parejas
    //mientras apuesta no esté terminada

    //TODO: para probar esto hay que usar modo interactivo, porque con la lógica actual nunca se dará esta situación
    while (apuesta_terminada(envites_grande, N_JUGADORES) == 0) {
        printf("HAY APUESTA!\n");
        //determinar pareja a la que han subido la apuesta
            //1. determinar jugador (índice) con la máxima apuesta
        int maximo = maximo_array(envites_grande, N_JUGADORES);
        debug("MÁXIMA APUESTA: %d", maximo);
        int jugador_maxima_apuesta = buscaIndice(envites_grande, 4, maximo);
            //2. determinar a qué pareja pertenece ese jugador
        int pareja_sube_apuesta = que_pareja_soy(jugador_maxima_apuesta, mano);
            //3. determinar la pareja contraria
        int pareja_apuesta_subida = add_mod(pareja_sube_apuesta, 1, 1);
        int jugador_1_pareja_subida = add_mod(jugador_maxima_apuesta, 1, 4);
        int jugador_2_pareja_subida = add_mod(jugador_maxima_apuesta, 3, 4);
        //enviar token a jugador 1 de pareja a la que han subido la apuesta
        if ((MODO_JUEGO == 1) && (jugador_1_pareja_subida == jugador_humano)) {
            token=3;
            MPI_Send(&token, 1, MPI_INT, jugador_1_pareja_subida, 0, juego_comm);
            do {
                if(hay_apuesta(envites_grande, N_JUGADORES) == 0) {
                    printf("Introduzca envite a %s: (0:no, 2: envido, >2: envido N)\n", lances_etiquetas[0]);
                }
                else {
                    printf("Introduzca envite a %s: (0:no, 2: envido más, >2: envido N más)\n", lances_etiquetas[0]);
                }
            }
            while (((scanf("%d%c", &envite, &c) != 2 || c!= '\n') && clean_stdin()));

        }

        else {
            token = 1;
            MPI_Send(&token, 1, MPI_INT, jugador_1_pareja_subida, 0, juego_comm);
            //recibir envite/paso de jugador
            MPI_Recv(&envite, 1, MPI_INT, jugador_1_pareja_subida, 0, juego_comm, MPI_STATUS_IGNORE);
        }
            print_envite(envite, jugador_1_pareja_subida, hay_apuesta(envites_grande, N_JUGADORES));
        envites_grande[jugador_1_pareja_subida] = envite;
        //enviar token a jugador 2 de pareja a la que han subido la apuesta
        if ((MODO_JUEGO == 1) && (jugador_2_pareja_subida == jugador_humano)) {
            token=3;
            MPI_Send(&token, 1, MPI_INT, jugador_2_pareja_subida, 0, juego_comm);
            do {
                if(hay_apuesta(envites_grande, N_JUGADORES) == 0) {
                    printf("Introduzca envite a %s: (0:no, 2: envido, >2: envido N)\n", lances_etiquetas[0]);
                }
                else {
                    printf("Introduzca envite a %s: (0:no, 2: envido más, >2: envido N más)\n", lances_etiquetas[0]);
                }
            }
            while (((scanf("%d%c", &envite, &c) != 2 || c!= '\n') && clean_stdin()));

        }
        else {
            MPI_Send(&token, 1, MPI_INT, jugador_2_pareja_subida, 0, juego_comm);
            //recibir envite/paso de jugador
            MPI_Recv(&envite, 1, MPI_INT, jugador_2_pareja_subida, 0, juego_comm, MPI_STATUS_IGNORE);
        }
            print_envite(envite, jugador_2_pareja_subida, hay_apuesta(envites_grande, N_JUGADORES));
        envites_grande[jugador_2_pareja_subida] = envite;
        //evaluar nuevo envite de la pareja y actualizar envites
        int j;
        siguiente_jugador = 0;
        for (j=0; j< N_JUGADORES;j++) {
            token = 3;
            MPI_Send(&token, 1, MPI_INT, j, 0, juego_comm);

        }
        MPI_Bcast(envites_grande, 4, MPI_INT, MPI_ROOT, juego_comm);
        printf("[maestro] Array de envites: %d, %d, %d, %d\n",  envites_grande[0], envites_grande[1],
               envites_grande[2], envites_grande[3]);
    }


    int j;
    siguiente_jugador = 0;
    for (j=0; j< N_JUGADORES;j++) {
        token = 2;
        MPI_Send(&token, 1, MPI_INT, j, 0, juego_comm);

    }

    // CHICA
    // PARES
    // JUEGO
   // debug("TAMAÑO DEL MAZO DESPUÉS DE DESCARTES: %d", size_mazo);
    //resultado: comparar cartas de una pareja despecto de la otra

    /* Recepción de datos para evaluar las manos de los jugadores */
    MPI_Gather(conteos, 10, MPI_INT, rbuf, 10, MPI_INT, MPI_ROOT, juego_comm);

    /*cálculo de manos*/
    ganador[0] = calculaGrande(rbuf, mano);
    printf("Mejor mano a grande: jugador %d\n", ganador[0]);

    // Cálculo de piedras

    // Calcular envite de pareja mano em
    em = envite_pareja(1, mano, envites_grande);
    // Calcular envite de pareja postre ep
    ep = envite_pareja(0, mano, envites_grande);

    // em == ep !=1? :
    if ( (em == ep) && em != 1 ) { //apuesta igualada, pareja del ganador se lleva 2 x em
        piedras[que_pareja_soy(ganador[i], mano)] += (2 * em);

    }
    // em != ep && min(em,ep) !=1 ?
    else if ((em != ep) && min(em,ep) != 1) { //alguien subió la apuesta y se rajó; ganador se lleva min(em,ep)
        piedras[que_pareja_soy(ganador[i], mano)] += min(em, ep);

    }

    // em != ep && min(em,ep) == 1 ?
    else if ((em != ep && min(em,ep == 1))) { // una pareja pasa y la otro ha envidado 2 o más; ganador se lleva 1 tanto
        piedras[que_pareja_soy(ganador[i], mano)] += 1;
    }

    // em == ep == 1? : todos pasan, nadie se lleva piedras. No hace falta hacer nada para este caso.

    printf("PIEDRAS MANO: %d\n", piedras[1]);
    printf("PIEDRAS POSTRE: %d\n", piedras[0]);

   printf("[maestro] FINALIZADO!\n");
    /*free(mazo->palo);
    free(mazo->cara);*/

    //MPI_Comm_disconnect(&juego_comm);
    //MPI_Barrier(juego_comm);
    MPI_Finalize();
    return 0;
}