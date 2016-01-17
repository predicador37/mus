//
// Created by predicador on 15/01/16.
//

#ifndef MUS_MUS_H
#define MUS_MUS_H
#include <mpi.h>
struct carta {

    char *cara;
    char *palo;
    int id;
    int valor;
};
/* fin struct carta */
typedef struct carta Carta;

int crearMazo(Carta * mazo, char *strCara[],
                char *strPalo[], int intValor[]);
void printMazo( Carta * wMazo, int sizeMazo);
void barajarMazo(Carta * wMazo);
void cortarMazo(Carta * wMazo, char ** paloCorte);
int add_mod(int a, int b, int m);
void enviarMazo(Carta * wMazo, int proceso, MPI_Comm wComm);
void recibirMazo(Carta * wMazo, int proceso, MPI_Comm wCommm, MPI_Status stat);
void enviarCarta (Carta wCarta, int proceso, MPI_Comm wComm);
Carta recibirCarta( int proceso, MPI_Comm wCommm, MPI_Status stat);
#endif //MUS_MUS_H
