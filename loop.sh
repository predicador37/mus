#!/bin/bash
for i in {1..10000}
do
mpirun -n 1 ./Debug/juego
done
