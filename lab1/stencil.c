/* 
  Parallel Computing II, Spring 2020.
  Instructor: Ting Lin @ PKU 
  This is a less naive implement of a 3D 7pt stencil.
  Version 0.1 (date: 03/08/2020)
*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>

#define L1  64
#define L2  64
#define L3  64
#define DL1 2
#define DL2 6 
#define DL3 0 
#define LT  500
#define SEED 0
#define index(i,L) ((i<0)?(i+L):((i>=L)?(i-L):(i)))

double get_walltime() {
  struct timeval tp;
  gettimeofday(&tp, NULL);
  return (double) (tp.tv_sec + tp.tv_usec*1e-6); 
}

  double input[L1+DL1][L2+DL2][L3+DL3];
  double output[L1+DL1][L2+DL2][L3+DL3];
  double diagsum[L2+DL2][L3]; // the reused block;
  
int main(int argc, char *argv[]) {
  int i, j, k, t;
  int max_it = 100;

  double time0, time1;

  // initialize data
  srand(SEED);
  for (k=0; k<L3; k++) {
    for (j=0; j<L2; j++) {
      for (i=0; i<L1; i++) {
        input[i][j][k] = rand()/(double)RAND_MAX;
      }
    }
  }

  // naive implementation
  time0 = get_walltime();
  for (t=0; t<LT; t+=2) {
    for (i=0; i<L1; i++) {
#pragma GCC unroll(8)      
for(j = 0; j< L2; j++)
      {
#pragma GCC unroll(8)
        for(k = 0; k < L3; k++)
        {
          diagsum[j][k] = input[i][index(j + 1, L2)][k] + input[i][j][index(k+1, L3)];
        }
      }
      for (j=0; j<L2; j++) {
#pragma GCC unroll(8)
        for (k=0; k<L3; k++) {
          output[i][j][k] = // this part seems will be optimized by icc.
              + 0.1 * ( 4 * input[i][j][k]
              +  diagsum[j][index(k-1,L3)]
              +  diagsum[index(j-1,L2)][k]
              +  input[index(i-1,L1)][j][k]
              +  input[index(i+1,L1)][j][k]);
        }
      }
    }
    for (i=0; i<L1; i++) {
      for(j = 0; j< L2; j++)
      {
#pragma GCC unroll(8)
        for(k = 0; k < L3; k++)
        {
          diagsum[j][k] = output[i][index(j + 1, L2)][k] + output[i][j][index(k+1, L3)];
        }
      }
      for (j=0; j<L2; j++) {
#pragma GCC unroll(8)
        for (k=0; k<L3; k++) {
          input[i][j][k] = 
              + 0.1 * ( 4 * output[i][j][k]
              + 1 * diagsum[j][index(k-1,L3)]
              + 1 * output[index(i-1,L1)][j][k]
              + 1 * output[index(i+1,L1)][j][k]
              + 1 * diagsum[index(j-1,L2)][k]);
        }
      }
    }
  }
 if(~(LT%2)) 
  for (i=0; i<L1; i++) {
      for (j=0; j<L2; j++) {
        for (k=0; k<L3; k++) {
#pragma GCC unroll(8)
          input[i][j][k] = output[i][j][k];
        }
      }
    }
  time1 = get_walltime() - time0;

  // report results
  printf("Grid size: %d x %d x %d, iterations: %d\n", L1, L2, L3, LT);
  printf("Wall time: %f\n", time1);
  // tweaks to avoid excessive compiler optimizations
  printf("The first and last entries: %f, %f\n", output[0][0][0], output[L1-1][L2-1][L3-1]);

  return 0; 
}
