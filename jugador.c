//
// Created by predicador on 15/01/16.
//

#include "mus.h"
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N_CARTAS_MAZO 40
#define N_CARTAS_MANO 4
#define N_CARTAS_PALO 10
#define N_PALOS 4
#define N_JUGADORES 4
#define N_LANCES 4

int main(int argc, char **argv) {
    int rank, size, namelen, version, subversion, psize, corte, postre, jugadorMano, repartidor, sizeMazo, sizeDescartadas, ok;
    int buffer[3], rbuf[50];
    MPI_Comm parent;
    Carta mazo[N_CARTAS_MAZO];
    Carta mano[N_CARTAS_MANO];
    char *palos[] = {"Oros", "Copas", "Espadas", "Bastos"};
    char *caras[] = {"As", "Dos", "Tres", "Cuatro", "Cinco",
                     "Seis", "Siete", "Sota", "Caballo", "Rey"};
    int cuentaCartas[N_CARTAS_PALO];
    char *paloCorte = (char *) malloc(7 * sizeof(char));
    char processor_name[MPI_MAX_PROCESSOR_NAME];

    MPI_Status stat;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Get_processor_name(processor_name, &namelen);
    MPI_Get_version(&version, &subversion);
    printf("[jugador %d] Jugador con identificador %d  de un total de  %d  en  %s  ejecutando  MPI  %d.%d\n", rank,
           rank, size,
           processor_name, version, subversion);

    MPI_Comm_get_parent(&parent);

    if (parent == MPI_COMM_NULL) {
        printf("[jugador %d] Error: no se ha encontrado proceso padre.\n", rank);
        exit(1);
    }

    MPI_Comm_remote_size(parent, &psize);

    if (psize != 1) {

        printf("[jugador %d] Error:  el número de padres debería ser 1 y no %d.\n", rank, psize);
        exit(2);
    }

    printf("[jugador %d] Jugador %d: Listo para jugar!\n", rank, rank);
    MPI_Bcast(&sizeMazo, 1, MPI_INT, 0, parent);
    MPI_Bcast(&sizeDescartadas, 1, MPI_INT, 0, parent);
    MPI_Bcast(&corte, 1, MPI_INT, 0, parent);


    if (rank == corte) {
        /* Este proceso debe realizar el corte */
        /* Para ello debe recibir el mazo */
        printf("[jugador %d] Proceso corte recibiendo mazo\n", rank);
        recibirMazo(mazo, 0, parent, N_CARTAS_MAZO, &stat);
        cortarMazo(mazo, &paloCorte);
        printf("[jugador %d] El palo de corte es: %s\n", rank, paloCorte);
        int j = 0;
        for (j = 0; j < N_PALOS; j++) {
            if (strcmp(paloCorte, palos[j]) == 0) {
                /* corresponde repartir primero al primer jugador de su derecha si sale oros;
                 * al segundo si sale copas; al tercero si sale espadas y al mismo que cortó si sale bastos*/

                repartidor = add_mod(corte, j + 1, 4);

                /* Envío del id del repartidor al proceso maestro */
                MPI_Send(&repartidor, 1, MPI_INT, 0, 0, parent);
            }
        }
    }
    MPI_Bcast(&repartidor, 1, MPI_INT, 0, parent); //recepción del repartidor desde el proceso maestro

    //int manoId = add_mod(postre, 1, 4);


    if (rank == repartidor && sizeMazo == 40) {

        printf("[repartidor %d] Proceso repartidor recibiendo mazo\n", rank);
        recibirMazo(mazo, 0, parent, N_CARTAS_MAZO, &stat);

        /* A continuación tiene lugar el reparto de cartas secuencial*/
        /* Por este requisito no se ha utilizado scatter*/
        /* La e/s se redirige al proceso maestro */

        int i = 0;
        int j = 0;
        int k = 0;

        int siguienteJugador = repartidor;
        for (i = 0; i < N_CARTAS_MANO; i++) {
            for (j = 0; j < N_JUGADORES; j++) { /* En total se reparten 16 cartas */

                siguienteJugador = add_mod(siguienteJugador, 1, 4);
                buffer[0] = i;
                buffer[1] = siguienteJugador;

                if (siguienteJugador != repartidor) {
                    repartirCarta(mazo[k], siguienteJugador, MPI_COMM_WORLD);
                    mazo[k].estado = 1; // la carta pasa a estado repartida
                    MPI_Send(&buffer, 2, MPI_INT, 0, 0, parent);

                }
                else {
                    /* Para repartirse a sí mismo no tiene sentido utilizar MPI */
                    MPI_Send(&buffer, 2, MPI_INT, 0, 0, parent);
                    mazo[k].estado = 1;// la carta pasa a estado repartida
                    mano[i] = mazo[k];
                    buffer[0] = rank;
                    buffer[1] = i;
                    buffer[2] = mano[i].valor;
                    MPI_Send(&buffer, 3, MPI_INT, 0, 0, parent);
                }
                sizeMazo--; /* es necesario almacenar el tamaño del mazo después de repartir */
                sizeDescartadas++;
                k++; /* un contador auxiliar para recuperar cartas del mazo */
            }
        }

        /* Todos los procesos deben conocer cuál es el tamaño actual del mazo */


        //MPI_Bcast(&sizeMazo, 1, MPI_INT, repartidor, MPI_COMM_WORLD);
        //MPI_Barrier(MPI_COMM_WORLD);
        /* El proceso maestro debe contar con el mazo actualizado */
        MPI_Send(&sizeMazo, 1, MPI_INT, 0, 0, parent);
        enviarMazo(mazo, 0, parent, N_CARTAS_MAZO);


    }
    else { /* el proceso no es corte ni postre; es decir, un jugador estándar */
        int i = 0;

        for (i = 0; i < N_CARTAS_MANO; i++) {

            mano[i] = recibirCarta(repartidor, MPI_COMM_WORLD, stat);
            buffer[0] = rank;
            buffer[1] = i;
            buffer[2] = mano[i].valor;
            MPI_Send(&buffer, 3, MPI_INT, 0, 0, parent);
        }


    }//se termina el reparto
    MPI_Bcast(&sizeMazo, 1, MPI_INT, 0, parent);
    MPI_Barrier(MPI_COMM_WORLD);
    //printf("[proceso %d] tamaño del mazo: %d\n", rank, sizeMazo);

    int siguienteJugador = add_mod(repartidor, 1, 4);


    /* una vez repartidas las cartas, mus corrido */
    /* hay que evaluar la mano... */
    // todo encapsular en funciones
    int i = 0;
    int juego = 0;
    int invertido[N_CARTAS_PALO];
    int pares[5];
    int equivalencias[N_CARTAS_MANO];

    for (i = 0; i < N_CARTAS_MANO; i++) {
        equivalencias[i] = mano[i].equivalencia;
    }

    int valores[N_CARTAS_MANO];
    for (i = 0; i < N_CARTAS_MANO; i++) {
        valores[i] = mano[i].valor;
    }

    /* Grande */
    int cuenta = 0;

    for (i = N_CARTAS_PALO - 1; i >= 0; i--) {
        cuenta = cuentaCartasMano(mano, caras[i]);
        cuentaCartas[N_CARTAS_PALO - i - 1] = cuenta;
        //printf("CUENTA %d para carta %s: %d\n", i, caras[i], cuenta);
    }

    /* chica */

    invertirArray(cuentaCartas, invertido, N_CARTAS_PALO);

    /* pares */

    preparaPares(equivalencias, pares);

    /* juego */

    juego = sumaArray(valores, N_CARTAS_MANO);


    int ronda = 0;
    int turno = 0;
    int bufferRcv[3] = {99, siguienteJugador, turno};
    int descartes[4] = {99, 99, 99, 99};
    int descarte = 99;
    jugadorMano = 99;
    int temp;
    int turnoDescartes = 1;
    while (jugadorMano == 99) {


        if (rank == siguienteJugador) {


            if ((turno % 4) == 0 && (turno != 0) && (turnoDescartes == 1)) {


                for (i = 0; i < N_CARTAS_MANO; i++) {
                    //se descarta cualquier carta que no sea un rey
                    if (mano[i].equivalencia != 10) {
                        MPI_Send(&mano[i].id, 1, MPI_INT, 0, 0, parent);
                        mano[i] = recibirCarta(0, parent, stat);
                        valores[i] = mano[i].valor;
                        equivalencias[i] = mano[i].equivalencia;

                        // enviar peticion de carta y descarte

                    }
                    else {
                        descarte = 99;
                        MPI_Send(&descarte, 1, MPI_INT, 0, 0, parent);

                    }

                }

                // volver a evaluar pares para ver si se corta el mus
                preparaPares(equivalencias, pares);

                siguienteJugador = add_mod(siguienteJugador, 1, 4);

                MPI_Send(&siguienteJugador, 1, MPI_INT, 0, 0, parent);
                MPI_Bcast(&turnoDescartes, 1, MPI_INT, 0, parent);
                // todo este broadcast bloquea programa
                // MPI_Bcast(&siguienteJugador, 1, MPI_INT, 0, parent);

                //MPI_Bcast(&temp, 1, MPI_INT, 0, parent);
                //siguienteJugador = temp;

                //break;

            }
            else {
                MPI_Bcast(&turnoDescartes, 1, MPI_INT, 0, parent);
                // todo quizá poner un else aqui y abajo
                int mus = cortarMus(valores, equivalencias, pares);

                siguienteJugador = add_mod(siguienteJugador, 1, 4);
                musCorrido(mus, &rank, &jugadorMano, &turno, &siguienteJugador, bufferRcv, parent);
            }
        }
        else { // al jugador no le toca

            if ((turno % 4) == 0 && (turno != 0) && (turnoDescartes == 1)) {


                MPI_Bcast(&turnoDescartes, 1, MPI_INT, 0, parent);


            }
            else {
                MPI_Bcast(&turnoDescartes, 1, MPI_INT, 0, parent);
                MPI_Bcast(&bufferRcv, 3, MPI_INT, 0, parent);
                //MPI_Bcast(&siguienteJugador, 1, MPI_INT, 0, parent);
                jugadorMano = bufferRcv[0];

                siguienteJugador = bufferRcv[1];
                turno = bufferRcv[2];
            }
        }
        MPI_Bcast(&temp, 1, MPI_INT, 0, parent);
        siguienteJugador = temp;

    }

    /* envites */
    int enviteMano[2];
    int envite[2];
    int enviteAnterior[2];
    envite[0]=0;
    envite[1]=0;
    enviteAnterior[0]=0;
    enviteAnterior[1]=0;
    int enviteContraria[2];
    enviteContraria[0]=99;
    enviteContraria[1]=0;
    int hayPares;
    int hayJuego;

    for (i=0; i< N_LANCES+1; i++) {



        /* ronda de envites */
        if (i == 2) { // ver si hay pares
            hayPares = tengoPares(pares);
            MPI_Gather(&hayPares, 1, MPI_INT, rbuf, 1, MPI_INT, 0, parent);
        }
        if (i == 3) {

            hayJuego = tengoJuego(sumaArray(valores, 4));
            printf("[proceso %d] HAY JUEGO: %d\n", rank, hayJuego);
            MPI_Gather(&hayJuego, 1, MPI_INT, rbuf, 1, MPI_INT, 0, parent);
            MPI_Bcast(&hayJuego, 2, MPI_INT, 0, parent);
        }
        if ((i == 0) || (i == 1) || ((i == 2) && (hayPares == 1)) || ((i == 3) && (hayJuego == 1)) ||
            ((i == 4) && (hayJuego == 0))) {
            /* empieza la mano */
            if (rank == jugadorMano) {
                //enviar envite a maestro

                // TODO: funcion envite
                // campo 1: 0: no, 2: envida o sí, 5: sube o iguala 5
                // campo 2: a qué pareja le toca contestar: 0 postre, 1 mano
                if (i==3 && hayJuego ==1){
                    enviteMano[0] = envido(valores, N_CARTAS_MANO, i, 0); // calculo de la suma
                }
                else if (i==4 && hayJuego ==0){
                    enviteMano[0] = envido(valores, N_CARTAS_MANO, i, 0); //
                }
                else { //resto de lances, no se trabaja con valores
                    enviteMano[0] = envido(equivalencias, N_CARTAS_MANO, i, 0); //no hay apuestas, así que 0
                }

                enviteMano[1] = 1;
                MPI_Send(&enviteMano, 2, MPI_INT, 0, 0, parent);
                MPI_Bcast(&envite, 2, MPI_INT, 0, parent);
                enviteAnterior[0]=envite[0];
                enviteAnterior[1]=envite[1];
                //MPI_Gather(enviteMano, 2, MPI_INT, rbuf, 2, MPI_INT, 0, parent);
                //MPI_Bcast(&enviteContraria, 2, MPI_INT, 0, parent);

            }
            else { //resto de jugadores
                // recibir envite del maestro
                MPI_Bcast(&envite, 2, MPI_INT, 0, parent);
                enviteAnterior[0]=envite[0];
                enviteAnterior[1]=envite[1];
                // calcular envite
                //envite[0] = envido(equivalencias, N_CARTAS_MANO, 0);;
                //envite[1] = 99;
               // MPI_Gather(envite, 2, MPI_INT, rbuf, 10, MPI_INT, 0, parent);
                //MPI_Bcast(&enviteContraria, 2, MPI_INT, 0, parent);
                //printf("[proceso %d] contraria envida: %d\n", enviteContraria[0]);
            }

            //while (envite[1] != 99) {
                //cualquier jugador calcula su envite y lo envía
                // calcular envite
                if ((i==2) && (tengoPares(pares) == 0) ){
                    envite[0]=0; // no tengo pares, no envido
                }
             else if ((i==3) && tengoJuego(sumaArray(valores, 4))==0){
                envite[0]=0; // no tengo juego, no envido
                }
                    else if ((i==3) && tengoJuego(sumaArray(valores, 4))==1) {
                    envite[0] = envido(valores, N_CARTAS_MANO, i, enviteAnterior[0]);
                }
                    else if ((i==4) && tengoJuego(sumaArray(valores, 4))==0) {
                envite[0] = envido(valores, N_CARTAS_MANO, i, enviteAnterior[0]);
            }
            else {//resto de lances
                 envite[0] = envido(equivalencias, N_CARTAS_MANO, i, enviteAnterior[0]);
                }

                printf("[proceso %d] Envido %d a lance %d\n", rank, envite[0], i);
                envite[1] = queParejaSoy(rank, jugadorMano);
                MPI_Gather(envite, 2, MPI_INT, rbuf, 2, MPI_INT, 0, parent);
                MPI_Bcast(&enviteContraria, 2, MPI_INT, 0, parent);
            //}
        }
    }

    // MPI_Barrier(MPI_COMM_WORLD);
    /* Envío de datos al maestro para que evalúe*/
    MPI_Gather(cuentaCartas, 10, MPI_INT, rbuf, 10, MPI_INT, 0, parent);
    MPI_Gather(invertido, 10, MPI_INT, rbuf, 10, MPI_INT, 0, parent);
    MPI_Gather(pares, 5, MPI_INT, rbuf, 5, MPI_INT, 0, parent);
    MPI_Gather(&juego, 1, MPI_INT, rbuf, 5, MPI_INT, 0, parent);

    /* no cortar mus */
    // si eres mano y puedes descartar
    enviarMazo(mano, 0, parent, N_CARTAS_MANO);

    MPI_Comm_disconnect(&parent);
    MPI_Finalize();
    return 0;
}