# make gol is enough
CC = gcc
MPICC = mpicc
FLAGS = -std=c99 -Wall -Ofast -march=native

gol:
	$(MPICC) $(FLAGS) gol.c -lm -o gol
clean:
	rm -f *.out
