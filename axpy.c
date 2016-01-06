#pragma offload_attribute(push,target(mic)) //{
#include <stdio.h>
#include <time.h>
#include <float.h>
#include <math.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#ifndef __cplusplus
#define bool _Bool
#define true 1
#define false 0
#endif

#define REAL float
#define NTIMES 50 

void init(REAL *A, long n) {
    long i;
    for (i = 0; i < n; i++) {
        A[i] = ((REAL) (drand48()) + 13);
    }
}

void axpy()
{
  double walltime;
  bool nthr_checked=false;
  time_t start;
  long n;
  REAL *y;
  REAL *y_ompacc;
  REAL *x;
  REAL a = 123.456;
  n = 5000000;
  long nthr=0;

    y = ((REAL *) (malloc((n * sizeof(REAL)))));
    x = ((REAL *) (malloc((n * sizeof(REAL)))));
    //y_ompacc = ((REAL *) (omp_unified_malloc((n * sizeof(REAL)))));
    //x = ((REAL *) (omp_unified_malloc((n * sizeof(REAL)))));

    srand48(1 << 12);
    init(x, n);
    init(y, n);

  long  l, i = 0;
  double start_timer = omp_get_wtime();
  start = time(NULL);
{
  for (l=0; l<NTIMES; l++) {
   #pragma omp parallel shared(x,y,n,a) private(i)
   {
   #pragma omp single nowait
    if (!nthr_checked) {
#ifdef _OPENMP
      nthr = omp_get_num_threads();
#endif
      printf( "\nWe are using %d thread(s)\n", nthr);
      nthr_checked = true;
    }

    #pragma omp for nowait
    for (i = 0; i < n; ++i)
      y[i] += a * x[i];
    }
    }
}
  walltime = omp_get_wtime() - start_timer;
  printf("\nFinished calculations.\n");
  printf("Matmul kernel wall clock time = %.2f sec\n", walltime);
}
#pragma offload_attribute(pop) //}

int main(void)
{
#pragma offload target(mic:0)
 axpy();
}
