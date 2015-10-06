// Experimental test input for Accelerator directives
// Liao 1/15/2013
/* the following headers crashed ROSE on coil because of stddef.h not found
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
*/
#include "axpy.h"

#define VEC_LEN 1024000 //use a fixed number for now
/* zero out the entire vector */
void zero(REAL *A, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        A[i] = 0.0;
    }
}

/* initialize a vector with random floating point numbers */
void init(REAL *A, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        A[i] = (double)drand48();
    }
}

REAL check(REAL*A, REAL*B, int n)
{
    int i;
    REAL sum = 0.0;
    for (i = 0; i < n; i++) {
        sum += A[i] - B[i];
    }
    return sum;
}

int main(int argc, char *argv[])
{
  int n;
  REAL *y_omp, *y_ompacc, *x;
  REAL a = 123.456;

  n = VEC_LEN;
  if (argc >= 2)
  	n = atoi(argv[1]);

  y_omp = (REAL *) malloc(n * sizeof(REAL));
  y_ompacc = (REAL *) malloc(n * sizeof(REAL));
  x = (REAL *) malloc(n * sizeof(REAL));

  srand48(1<<12);
  init(x, n);
  init(y_omp, n);
  memcpy(y_ompacc, y_omp, n*sizeof(REAL));
  
  int num_threads;
  #pragma omp parallel shared (num_threads)
  {
      if (omp_get_thread_num() == 0)
      num_threads = omp_get_num_threads();
  }
  double omp_time = read_timer(); /* the omp_get_wtime is not working in my test */
  axpy_omp(x, y_omp, n, a); 
  omp_time = read_timer() - omp_time;
  
  /* openmp acc version */
  double ompacc_time = read_timer();
//  axpy_ompacc(x, y_ompacc, n, a);
  axpy_ompacc_mdev_1(x, y_ompacc, n, a);
  ompacc_time = read_timer() - ompacc_time;

  printf("axpy(%d): checksum: %g; time(s):\tOMP(%d threads)\tOMPACC\n", n, check(y_omp, y_ompacc, n),num_threads);
  printf("\t\t\t\t\t\t%4f\t%4f\n", omp_time, ompacc_time);
  
  free(y_omp);
  free(y_ompacc);
  free(x);
  return 0;
}

