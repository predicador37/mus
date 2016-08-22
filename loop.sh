#!/bin/bash
for i in {1..1000}
do
mpirun -n 1 ./Debug/juego
done
