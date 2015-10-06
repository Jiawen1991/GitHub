#ifndef __MATVEC_H__
#define __MATVEC_H__

/* change this to do saxpy or daxpy : single precision or double precision*/
#define REAL float

#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include "homp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* both the omp version and ompacc version */
extern void matvec(REAL *a, REAL *x, REAL *y, long n);
extern double matvec_ompacc_mdev(REAL *a, REAL *x, REAL *y, long n);

#ifdef __cplusplus
 }
#endif

#endif
