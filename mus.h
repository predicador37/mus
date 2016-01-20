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
    int orden;
    int valor;
    int equivalencia;
    int estado; // 0: en mazo, 1: en mano, 2: descartada
};
/* fin struct carta */
typedef struct carta Carta;

int crearMazo(Carta *mazo, char *strCara[],
              char *strPalo[], int intValor[], int intEquivalencias[]);

void printMazo(Carta *wMazo, int sizeMazo);

void barajarMazo(Carta *wMazo);

void cortarMazo(Carta *wMazo, char **paloCorte);

int add_mod(int a, int b, int m);

void enviarMazo(Carta *wMazo, int proceso, MPI_Comm wComm);

void recibirMazo(Carta *wMazo, int proceso, MPI_Comm wCommm, MPI_Status * stat);

void repartirCarta(Carta wCarta, int proceso, MPI_Comm wComm);

Carta recibirCarta(int proceso, MPI_Comm wCommm, MPI_Status stat);

int cuentaCartasMano(Carta *wMano, char *cara);

int maximoArray(int array[], int longitud);

int maximoArrayExcluyendo(int array[], int longitud, int excluido);

int ocurrenciasArray(int array[], int longitud, int numero);

int buscaIndice(int a[], int longitud, int numero);

int buscarIndiceNumeroNoIgual(int a[], int longitud, int numero);

void invertirArray(int *orig, int *dest, int longitud);

int calculaGrande(int rbuf[]);

int calculaChica(int rbufInv[]);

int cmpfunc(const void *a, const void *b);

int *uniquePairs(int array[], int longitud, int repeticion);

int calcularPares(int paresBuf[]);

int sumaArray(int a[], int longitud);

int calcularJuego(int juegoBuf[]);
int tengoJuego(int suma);
int tengoMedias(int *paresBuf);
int tengoDuples(int *paresBuf);
int tengoPares(int *paresBuf);
int cortarMus(int *valores, int *equivalencias, int *paresBuf);

#endif //MUS_MUS_H
