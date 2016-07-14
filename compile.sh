#!/bin/bash

mpicc -g -o ./Debug/juego juego.c mus.c
mpicc -g -o ./Debug/jugador jugador.c mus.c
