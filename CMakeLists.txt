cmake_minimum_required(VERSION 3.0)
project(mus)

find_package(MPI REQUIRED)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g")
SET(CMAKE_C_COMPILER mpicc)
set(SOURCE_FILES  mus.c mus.h juego.c jugador.c)
add_executable(jugador mus.c mus.h dbg.h jugador.c )
add_executable(juego mus.h mus.c dbg.h juego.c )
