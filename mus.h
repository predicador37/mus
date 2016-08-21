//
// Created by predicador on 15/06/16.
//

#ifndef MUS_MUS_H
#define MUS_MUS_H

struct carta {

    int cara;
    int palo;
    int id;
    int orden;
    int estado; // 0: en mazo, 1: en mano, 2: descartada
};
/* fin struct carta */
typedef struct carta Carta;

int rand_lim(int limit);

int crear_mazo(Carta *mazo);

void print_mazo(Carta *wMazo, int sizeMazo);

void barajar_mazo(Carta *wMazo);

void cortar_mazo(Carta *wMazo, char *paloCorte);

int add_mod(int a, int b, int m);

void enviar_mazo(Carta *wMazo, int proceso, MPI_Comm wComm, int nCartas);

void recibir_mazo(Carta *wMazo, int proceso, MPI_Comm wCommm, int nCartas, MPI_Status *stat);

void repartir_carta(Carta wCarta, int proceso, MPI_Comm wComm);

Carta recibir_carta(int proceso, MPI_Comm wCommm, MPI_Status *stat);

void determinar_repartidor(int corte, int repartidor, char * palo_corte, Carta mazo[], MPI_Comm parent, const char * palos[], MPI_Status stat);

int repartidor_reparte(int rank, int repartidor,  int size_mazo, int size_descartadas, Carta mazo[], Carta mano_cartas[], MPI_Comm parent, MPI_Status stat);

void jugador_recibe_cartas(int rank, int repartidor, Carta mano_cartas[],  MPI_Comm parent, MPI_Status *stat);

int cuenta_cartas_mano(Carta *wMano, int  cara);

int maximo_array(int array[], int longitud);

int maximo_array_excluyendo(int array[], int longitud, int excluido);

int ocurrenciasArray(int array[], int longitud, int numero);

int buscaIndice(int a[], int longitud, int numero);

int buscarIndiceNumeroNoIgual(int a[], int longitud, int numero);

void invertirArray(int *orig, int *dest, int longitud);

int calculaGrande(int rbuf[], int jugadorMano);

int calculaChica(int rbufInv[]);

int cmpfunc(const void *a, const void *b);

void unique_airs(int array[], int longitud, int repeticion,  int parejas[]);

int calcularPares(int paresBuf[], int jugadorMano);

int sumaArray(int a[], int longitud);

int calcularJuego(int juegoBuf[], int mano);

void *preparaPares(int equivalencias[], int *pares);

int tengoJuego(int suma);

int tengo_juego_decente(int suma);

int tengoMedias(int *paresBuf);

int tengoDuples(int *paresBuf);

int tengoPares(int *paresBuf);

int cortarMus(int *valores, int *equivalencias, int *paresBuf);

void marcar_descarte(Carta *wMazo, int sizeMazo, int id);

int ordago();

void envido(int envites[], int *equivalencias, int longitud, int lance, int apuestaVigor, int jugador_mano, int rank);

int busca_indice(int a[], int longitud, int numero);

int que_pareja_soy(int rank, int jugadorMano);

int misma_pareja(int rank1, int rank2);

int contar_ocurrencias(int envites_jugadores[], int longitud, int valor);

int apuesta_terminada(int envites_jugadores[], int longitud);

int hay_apuesta(int envites_jugadores[], int longitud);

int max(int a, int b);

int min(int a, int b);

void print_envite(int envite, int siguiente_jugador, int hay_apuesta,  int envite_N);

int clean_stdin();

int pareja_pasa(int envites_jugadores[]);

int envites_misma_pareja(int envites_jugadores[]);

int envite_pareja(int pareja, int mano, int envites[]);

int calcular_envite(int envites[], int envite, int envite_N, int envite_vigor);

int esta_valor_en_array(int val, int *arr, int size);

#endif //MUS_MUS_H
