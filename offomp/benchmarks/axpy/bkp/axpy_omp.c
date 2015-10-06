#include "axpy.h"

void axpy_omp(REAL* x, REAL* y, int n, REAL a) {
  int i;
#pragma omp parallel for shared(x, y, n, a) private(i)
  for (i = 0; i < n; ++i)
    y[i] += a * x[i];
}
