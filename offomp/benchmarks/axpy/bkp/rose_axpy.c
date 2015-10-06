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
#include "libxomp.h" 

void zero(double *A, long n)
{
   long i;
  for (i = 0; i < n; i++) {
    A[i] = 0.0;
  }
}
/* initialize a vector with random floating point numbers */

void init(double *A, long n)
{
   long i;
  for (i = 0; i < n; i++) {
    A[i] = ((double )(drand48()));
  }
}

double check(double *A,double *B, long n)
{
   long i;
  double sum = 0.0;
  for (i = 0; i < n; i++) {
    sum += fabs(A[i] - B[i]);
  }
  return sum;
}

struct OUT__1__5182___data 
{
  void *num_threads_p;
}
;
static void OUT__1__5182__(void *__out_argv);

int main(int argc,char *argv[])
{
  int status = 0;
  XOMP_init(argc,argv);
  omp_init_devices(); 
  long n;
  double *y_omp;
  double *y_ompacc;
  double *x;
  double a = 123.456;
  n = 1024*1024*1024;
  if (argc >= 2) 
    n = atoi(argv[1]);
  y_omp = ((double *)(malloc((n * sizeof(double )))));
  y_ompacc = ((double *)(malloc((n * sizeof(double )))));
  x = ((double *)(malloc((n * sizeof(double )))));
  srand48(1 << 12);
  init(x,n);
  init(y_omp,n);
  memcpy(y_ompacc,y_omp,(n * sizeof(double )));
  int num_threads;
  struct OUT__1__5182___data __out_argv1__5182__;
  __out_argv1__5182__.num_threads_p = ((void *)(&num_threads));
  XOMP_parallel_start(OUT__1__5182__,&__out_argv1__5182__,1,0,"/data/yy8/2013-8-multiple-gpu-work/benchmarks/axpy/axpy.c",60);
  XOMP_parallel_end("/data/yy8/2013-8-multiple-gpu-work/benchmarks/axpy/axpy.c",64);
  double omp_time = read_timer_ms();
  axpy_omp(x,y_omp,n,a);
  omp_time = (read_timer_ms() - omp_time);
  double ompacc_time = axpy_ompacc_mdev_v2(x,y_ompacc,n,a);
  printf("axpy(%d): checksum: %g; time(ms):\tOMP(%d threads)\tOMPACC\n",n,check(y_omp,y_ompacc,n),num_threads);
  printf("\t\t\t\t\t\t%4f\t%4f, %d devices\n",omp_time,ompacc_time, omp_get_num_active_devices());
  free(y_omp);
  free(y_ompacc);
  free(x);
  XOMP_terminate(status);
  return 0;
}

static void OUT__1__5182__(void *__out_argv)
{
  int *num_threads = (int *)(((struct OUT__1__5182___data *)__out_argv) -> num_threads_p);
  if (omp_get_thread_num() == 0) 
     *num_threads = omp_get_num_threads();
}
