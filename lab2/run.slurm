#!/bin/bash
#SBATCH -J gameoflife
#SBATCH -N 1
#SBATCH -p cpu
#SBATCH --ntasks-per-node=8
#SBATCH --cpus-per-task=1
#SBATCH --nodelist=cu05
#SBATCH -o %j_%N.out

mpiexec -n 8 ./gol
mpiexec -n 8 ./gol
mpiexec -n 8 ./gol
