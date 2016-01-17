cmake_minimum_required(VERSION 3.3)
project(mus)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall ")

set(SOURCE_FILES juego.c jugador.c mus.c mus.h)
add_executable(jugador jugador.c mus.c mus.h)
add_executable(juego juego.c mus.c mus.h)
