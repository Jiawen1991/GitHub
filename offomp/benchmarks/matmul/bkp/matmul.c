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

void zero(REAL *A, int n)
{
    int i, j;
#pragma omp for private (i, j)
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            A[i * n + j] = 0.0;
        }
    }
}

void init(REAL *A, int n)
{
    int i, j;

#pragma omp for private (i, j)
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            A[i * n + j] = (double)drand48();
        }
    }
}

double maxerror(REAL *A, REAL *B, int n)
{
    int i, j;
    double error = 0.0;

    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            double diff = (A[i * n + j] - B[i * n + j]) / A[i * n + j];
    //        printf("%4f -- %4f\n", A[i*n+j], B[i*n+j]);
            if (diff < 0)
                diff = -diff;
            if (diff > error)
                error = diff;
        }
    }
    return error;
}

void iter_matmul(REAL *A, REAL *B, REAL *C, int n)
{
    int i, j, k;

    for (i = 0; i < n; i++)
        for (k = 0; k < n; k++) {
            REAL c = 0.0;
            for (j = 0; j < n; j++)
                c += A[i * n + j] * B[j * n + k];
            C[i * n + k] = c;
        }
}

void omp_matmul(REAL *A, REAL *B, REAL *C, int n)
{
    int i, j, k;
#pragma omp parallel for shared(A, B, C, n) private(i,j,k)
    for (i = 0; i < n; i++)
        for (k = 0; k < n; k++) {
            REAL c = 0.0;
            for (j = 0; j < n; j++)
                c += A[i * n + j] * B[j * n + k];
            C[i * n + k] = c;
        }
}

/* one device */
void ompacc_matmul(REAL *A, REAL *B, REAL *C, int n)
{
    int i, j, k;
#pragma omp target map(out:C[0:n][0:n]), map(in:n,A[0:n][0:n],B[0:n][0:n])
#pragma omp parallel for private(i,j,k)
    for (i = 0; i < n; i++)
        for (k = 0; k < n; k++) {
            REAL c = 0.0;
            for (j = 0; j < n; j++)
                c += A[i * n + j] * B[j * n + k];
            C[i * n + k] = c;
        }
}

#if 0
/* multiple device */

/* A, C row-major partition */
void ompacc_matmul_mdev_1(REAL *A, REAL *B, REAL *C, int n)
{
    int i, j, k;
#pragma omp target device(*) map(from:C[0:n]{0:n}>>(*)), map(to:n,A[0:n]{0:n}>>(*),B[0:n][0:n])
#pragma omp parallel for private(i,j,k) dist_iteration match_range C[:]
    for (i = 0; i < n; i++)
        for (k = 0; k < n; k++) {
            REAL c = 0.0;
            for (j = 0; j < n; j++)
                c += A[i * n + j] * B[j * n + k];
            C[i * n + k] = c;
        }
}

/* B, C column-major partition */
void ompacc_matmul_mdev_2(REAL *A, REAL *B, REAL *C, int n)
{
    int i, j, k;
#pragma omp target device(*) map(from:C{0:n}[0:n]>>(*)), map(to:n,A[0:n][0:n],B{0:n}[0:n]>>(*)
    for (i = 0; i < n; i++)
#pragma omp parallel for private(i,j,k) dist_iteration match_range C{}[]
        for (k = 0; k < n; k++) {
            REAL c = 0.0;
            for (j = 0; j < n; j++)
                c += A[i * n + j] * B[j * n + k];
            C[i * n + k] = c;
        }
}

/* A,B, C row-column partition */
void ompacc_matmul_mdev_3(REAL *A, REAL *B, REAL *C, int n)
{
    int i, j, k;
#pragma omp target device(*)=>(:)(:) map(from:C[0:n][0:n]>>(:)(:)), map(to:n,A[0:n]{0:n}>>(:){:},B{0:n}[0:n]>>{:}())
#pragma omp parallel for private(i,j,k) dist_iteration match_range C[]{}
    for (i = 0; i < n; i++)
#pragma omp parallel for private(i,j,k) dist_iteration match_range C{}[]
        for (k = 0; k < n; k++) {
            REAL c = 0.0;
            for (j = 0; j < n; j++)
                c += A[i * n + j] * B[j * n + k];
            C[i * n + k] = c;
        }
}
#endif

void openacc_matmul(REAL *A, REAL *B, REAL *C, int n)
{
    int i, j, k;
/* #pragma acc kernels copyin(A[0:n][0:n],B[0:n][0:n]) copyout(C[0:n][0:n]) */
//#pragma acc kernels loop copyin(A[0:n*n],B[0:n*n]) copyout(C[0:n*n])
#pragma acc parallel loop copyin(A[0:n*n],B[0:n*n]) copyout(C[0:n*n]) collapse(2)
    for (i = 0; i < n; i++)
        for (k = 0; k < n; k++) {
            REAL c = 0.0;
            for (j = 0; j < n; j++)
                c += A[i * n + j] * B[j * n + k];
            C[i * n + k] = c;
        }
}

int main(int argc, char *argv[])
{
    int n;
    int num_threads;
    REAL *A, *B, *C_seq, *C_omp_for, *C_acc;
    double seq_elapsed, omp_for_elapsed, acc_elapsed;

    if (argc != 2) {
        fprintf(stderr, "Usage: matmul <n>\n");
        exit(1);
    }
    n = atoi(argv[1]);

    A = (REAL*)malloc(n * n * sizeof(REAL));
    B = (REAL*)malloc(n * n * sizeof(REAL));
    C_seq = (REAL*)malloc(n * n * sizeof(REAL));
    C_omp_for = (REAL*)malloc(n * n * sizeof(REAL));
    C_acc = (REAL*)malloc(n * n * sizeof(REAL));

    srand48(1<<12);
   // shared(A, B, C_seq, C_seq_tile, C_omp_for, C_omp_task, C_omp_dtile, C_omp_drecursive_tile, C_omp_drecursive_tile_dupB, n, num_threads)
#pragma omp parallel 
    {
#pragma omp master
        { num_threads = omp_get_num_threads();
        }

        init(A, n);
        init(B, n);
        zero(C_seq, n);
        zero(C_omp_for, n);
        zero(C_acc, n);
    }

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
    ompacc_matmul(A, B, C_acc, n);
    acc_elapsed = omp_get_wtime() - acc_elapsed;
#else
    acc_elapsed = omp_get_wtime();
    openacc_matmul(A, B, C_acc, n);
    acc_elapsed = omp_get_wtime() - acc_elapsed;
#endif

    printf("=======================================================================\n");
    printf("\t\tmatmul(%dx%d) example on %d threads(cores)\n", n, n, num_threads);
    printf("-----------------------------------------------------------------------\n");
    printf("Performance:  Runtime (s)\t MFLOPS\t\t\t Error\n");
    printf("-----------------------------------------------------------------------\n");
    printf("Sequential      :  %4f \t\t %4f\t\t%g\n", seq_elapsed, 2.0 * n * n * n / (1.0e6 * (seq_elapsed)), maxerror(C_seq, C_seq, n));
    printf("OMP For         :  %4f \t\t %4f\t\t%g\n", omp_for_elapsed, 2.0 * n * n * n / (1.0e6 * (omp_for_elapsed)), maxerror(C_seq, C_omp_for, n));

#ifndef OPENACC
    printf("OMP ACC         :  %4f \t\t %4f\t\t%g\n", acc_elapsed, 2.0 * n * n * n / (1.0e6 * (acc_elapsed)), maxerror(C_seq, C_acc, n));
#else
    printf("OpenACC         :  %4f \t\t %4f\t\t%g\n", acc_elapsed, 2.0 * n * n * n / (1.0e6 * (acc_elapsed)), maxerror(C_seq, C_acc, n));
#endif

    free(C_acc);
    free(C_omp_for);
    free(C_seq);
    free(B);
    free(A);
    return 0;
}
