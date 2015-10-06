/*
 * Rectangular matrix multiplication, started from MIT Cilk matmul.cilk example
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "omp.h"
#include <pthread.h>
#include <string.h>
#define REAL float
#include "libxomp.h" 
#include "xomp_cuda_lib_inlined.cu" 

void zero(float *A,int n)
{
  int i;
  int j;
{
    int _p_i;
    int _p_j;
    long p_index_;
    long p_lower_;
    long p_upper_;
    XOMP_loop_default(0,n - 1,1,&p_lower_,&p_upper_);
    for (p_index_ = p_lower_; p_index_ <= p_upper_; p_index_ += 1) {
      for (_p_j = 0; _p_j < n; _p_j++) {
        A[p_index_ * n + _p_j] = 0.0;
      }
    }
    XOMP_barrier();
  }
}

void init(float *A,int n)
{
  int i;
  int j;
{
    int _p_i;
    int _p_j;
    long p_index_;
    long p_lower_;
    long p_upper_;
    XOMP_loop_default(0,n - 1,1,&p_lower_,&p_upper_);
    for (p_index_ = p_lower_; p_index_ <= p_upper_; p_index_ += 1) {
      for (_p_j = 0; _p_j < n; _p_j++) {
        A[p_index_ * n + _p_j] = ((double )(drand48()));
      }
    }
    XOMP_barrier();
  }
}

double maxerror(float *A,float *B,int n)
{
  int i;
  int j;
  double error = 0.0;
  for (i = 0; i < n; i++) {
    for (j = 0; j < n; j++) {
      double diff = ((A[i * n + j] - B[i * n + j]) / A[i * n + j]);
//        printf("%4f -- %4f\n", A[i*n+j], B[i*n+j]);
      if (diff < 0) {
        diff = -diff;
      }
      if (diff > error) {
        error = diff;
      }
    }
  }
  return error;
}

void iter_matmul(float *A,float *B,float *C,int n)
{
  int i;
  int j;
  int k;
  for (i = 0; i < n; i++) 
    for (k = 0; k < n; k++) {
      float c = 0.0;
      for (j = 0; j < n; j++) 
        c += A[i * n + j] * B[j * n + k];
      C[i * n + k] = c;
    }
}

struct OUT__3__7132___data 
{
  void *A_p;
  void *B_p;
  void *C_p;
  void *n_p;
}
;
static void OUT__3__7132__(void *__out_argv);

void omp_matmul(float *A,float *B,float *C,int n)
{
  int i;
  int j;
  int k;
  struct OUT__3__7132___data __out_argv2__7132__;
  __out_argv2__7132__ . n_p = ((void *)(&n));
  __out_argv2__7132__ . C_p = ((void *)(&C));
  __out_argv2__7132__ . B_p = ((void *)(&B));
  __out_argv2__7132__ . A_p = ((void *)(&A));
  XOMP_parallel_start(OUT__3__7132__,&__out_argv2__7132__,1,0,"/home/yy8/2013-8-multiple-gpu-work/benchmarks/matrixmultiplication/matmul.c",73);
  XOMP_parallel_end("/home/yy8/2013-8-multiple-gpu-work/benchmarks/matrixmultiplication/matmul.c",80);
}
/* one device */

__global__ void OUT__2__7132__(int n,float *_dev_A,float *_dev_B,float *_dev_C)
{
  int _p_i;
  int _p_j;
  int _p_k;
  int _dev_i = blockDim.x * blockIdx.x + threadIdx.x;
  if (_dev_i >= 0 && _dev_i <= n - 1) {
    for (_p_k = 0; _p_k < n; _p_k++) {
      float c = 0.0;
      for (_p_j = 0; _p_j < n; _p_j++) 
        c += _dev_A[_dev_i * n + _p_j] * _dev_B[_p_j * n + _p_k];
      _dev_C[_dev_i * n + _p_k] = c;
    }
  }
}

void ompacc_matmul(float *A,float *B,float *C,int n)
{
  int i;
  int j;
  int k;
{
    float *_dev_A;
    int _dev_A_size = sizeof(float ) * (n - 0) * (n - 0);
    _dev_A = ((float *)(xomp_deviceMalloc(_dev_A_size)));
    xomp_memcpyHostToDevice(((void *)_dev_A),((const void *)A),_dev_A_size);
    float *_dev_B;
    int _dev_B_size = sizeof(float ) * (n - 0) * (n - 0);
    _dev_B = ((float *)(xomp_deviceMalloc(_dev_B_size)));
    xomp_memcpyHostToDevice(((void *)_dev_B),((const void *)B),_dev_B_size);
    float *_dev_C;
    int _dev_C_size = sizeof(float ) * (n - 0) * (n - 0);
    _dev_C = ((float *)(xomp_deviceMalloc(_dev_C_size)));
/* Launch CUDA kernel ... */
    int _threads_per_block_ = xomp_get_maxThreadsPerBlock();
    int _num_blocks_ = xomp_get_max1DBlock(n - 1 - 0 + 1);
    OUT__2__7132__<<<_num_blocks_,_threads_per_block_>>>(n,_dev_A,_dev_B,_dev_C);
    xomp_freeDevice(_dev_A);
    xomp_freeDevice(_dev_B);
    xomp_memcpyDeviceToHost(((void *)C),((const void *)_dev_C),_dev_C_size);
    xomp_freeDevice(_dev_C);
  }
}
#if 0
/* multiple device */
/* A, C row-major partition */
/* B, C column-major partition */
/* A,B, C row-column partition */
#endif

void openacc_matmul(float *A,float *B,float *C,int n)
{
  int i;
  int j;
  int k;
/* #pragma acc kernels copyin(A[0:n][0:n],B[0:n][0:n]) copyout(C[0:n][0:n]) */
//#pragma acc kernels loop copyin(A[0:n*n],B[0:n*n]) copyout(C[0:n*n])
  
#pragma acc parallel loop copyin(A[0:n*n],B[0:n*n]) copyout(C[0:n*n]) collapse(2)
  for (i = 0; i < n; i++) 
    for (k = 0; k < n; k++) {
      float c = 0.0;
      for (j = 0; j < n; j++) 
        c += A[i * n + j] * B[j * n + k];
      C[i * n + k] = c;
    }
}

struct OUT__1__7132___data 
{
  void *n_p;
  void *num_threads_p;
  void *A_p;
  void *B_p;
  void *C_seq_p;
  void *C_omp_for_p;
  void *C_acc_p;
}
;
static void OUT__1__7132__(void *__out_argv);

int main(int argc,char *argv[])
{
  int n;
  int num_threads;
  float *A;
  float *B;
  float *C_seq;
  float *C_omp_for;
  float *C_acc;
  double seq_elapsed;
  double omp_for_elapsed;
  double acc_elapsed;
  if (argc != 2) {
    fprintf(stderr,"Usage: matmul <n>\n");
    exit(1);
  }
  n = atoi(argv[1]);
  A = ((float *)(malloc((n * n) * sizeof(float ))));
  B = ((float *)(malloc((n * n) * sizeof(float ))));
  C_seq = ((float *)(malloc((n * n) * sizeof(float ))));
  C_omp_for = ((float *)(malloc((n * n) * sizeof(float ))));
  C_acc = ((float *)(malloc((n * n) * sizeof(float ))));
  srand48((1 << 12));
  struct OUT__1__7132___data __out_argv1__7132__;
  __out_argv1__7132__ . C_acc_p = ((void *)(&C_acc));
  __out_argv1__7132__ . C_omp_for_p = ((void *)(&C_omp_for));
  __out_argv1__7132__ . C_seq_p = ((void *)(&C_seq));
  __out_argv1__7132__ . B_p = ((void *)(&B));
  __out_argv1__7132__ . A_p = ((void *)(&A));
  __out_argv1__7132__ . num_threads_p = ((void *)(&num_threads));
  __out_argv1__7132__ . n_p = ((void *)(&n));
  XOMP_parallel_start(OUT__1__7132__,&__out_argv1__7132__,1,0,"/home/yy8/2013-8-multiple-gpu-work/benchmarks/matrixmultiplication/matmul.c",184);
  XOMP_parallel_end("/home/yy8/2013-8-multiple-gpu-work/benchmarks/matrixmultiplication/matmul.c",195);
/* sequential run */
  seq_elapsed = omp_get_wtime();
//    iter_matmul(A, B, C_seq, n);
  seq_elapsed = omp_get_wtime() - seq_elapsed;
/* openmp parallel for version */
  omp_for_elapsed = omp_get_wtime();
//    omp_matmul(A, B, C_omp_for, n);
  omp_for_elapsed = omp_get_wtime() - omp_for_elapsed;
/* we currently cannot do the OpenMP acc and OpenACC run in once */
#ifndef OPENACC
/* openmp acc version */
  acc_elapsed = omp_get_wtime();
  ompacc_matmul(A,B,C_acc,n);
  acc_elapsed = omp_get_wtime() - acc_elapsed;
#else
#endif
  printf("=======================================================================\n");
  printf("\t\tmatmul(%dx%d) example on %d threads(cores)\n",n,n,num_threads);
  printf("-----------------------------------------------------------------------\n");
  printf("Performance:  Runtime (s)\t MFLOPS\t\t\t Error\n");
  printf("-----------------------------------------------------------------------\n");
  printf("Sequential      :  %4f \t\t %4f\t\t%g\n",seq_elapsed,2.0 * n * n * n / (1.0e6 * seq_elapsed),maxerror(C_seq,C_seq,n));
  printf("OMP For         :  %4f \t\t %4f\t\t%g\n",omp_for_elapsed,2.0 * n * n * n / (1.0e6 * omp_for_elapsed),maxerror(C_seq,C_omp_for,n));
#ifndef OPENACC
  printf("OMP ACC         :  %4f \t\t %4f\t\t%g\n",acc_elapsed,2.0 * n * n * n / (1.0e6 * acc_elapsed),maxerror(C_seq,C_acc,n));
#else
#endif
  free(C_acc);
  free(C_omp_for);
  free(C_seq);
  free(B);
  free(A);
  return 0;
}

static void OUT__1__7132__(void *__out_argv)
{
  int *n = (int *)(((struct OUT__1__7132___data *)__out_argv) -> n_p);
  int *num_threads = (int *)(((struct OUT__1__7132___data *)__out_argv) -> num_threads_p);
  float **A = (float **)(((struct OUT__1__7132___data *)__out_argv) -> A_p);
  float **B = (float **)(((struct OUT__1__7132___data *)__out_argv) -> B_p);
  float **C_seq = (float **)(((struct OUT__1__7132___data *)__out_argv) -> C_seq_p);
  float **C_omp_for = (float **)(((struct OUT__1__7132___data *)__out_argv) -> C_omp_for_p);
  float **C_acc = (float **)(((struct OUT__1__7132___data *)__out_argv) -> C_acc_p);
  if (XOMP_master()) {
     *num_threads = omp_get_num_threads();
  }
  init( *A, *n);
  init( *B, *n);
  zero( *C_seq, *n);
  zero( *C_omp_for, *n);
  zero( *C_acc, *n);
}

static void OUT__3__7132__(void *__out_argv)
{
  float **A = (float **)(((struct OUT__3__7132___data *)__out_argv) -> A_p);
  float **B = (float **)(((struct OUT__3__7132___data *)__out_argv) -> B_p);
  float **C = (float **)(((struct OUT__3__7132___data *)__out_argv) -> C_p);
  int *n = (int *)(((struct OUT__3__7132___data *)__out_argv) -> n_p);
  int _p_i;
  int _p_j;
  int _p_k;
  long p_index_;
  long p_lower_;
  long p_upper_;
  XOMP_loop_default(0, *n - 1,1,&p_lower_,&p_upper_);
  for (p_index_ = p_lower_; p_index_ <= p_upper_; p_index_ += 1) {
    for (_p_k = 0; _p_k <  *n; _p_k++) {
      float c = 0.0;
      for (_p_j = 0; _p_j <  *n; _p_j++) 
        c += ( *A)[p_index_ *  *n + _p_j] * ( *B)[_p_j *  *n + _p_k];
      ( *C)[p_index_ *  *n + _p_k] = c;
    }
  }
  XOMP_barrier();
}
