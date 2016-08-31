#!/bin/bash

mpicc -g -o ./juego juego.c mus.c
mpicc -g -o ./jugador jugador.c mus.c
