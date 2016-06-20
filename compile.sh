#!/bin/bash

mpicc -o juego juego.c mus.c
mpicc -o jugador jugador.c mus.c
