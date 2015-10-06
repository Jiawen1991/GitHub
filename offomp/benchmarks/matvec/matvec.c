#include <assert.h>
#include "matvec.h"

#define VEC_LEN 1024000 //use a fixed number for now

/* zero out the entire vector */
void matvec(REAL *a, REAL *x, REAL *y, long n) {
    long i, j;
    for (i = 0; i < n; ++i) {
        for (j = 0; j < n; ++j)
            y[i] += a[i * n + j] * x[j];
        //printf("x[%d]: %f, y[%d]: %f\n", i, x[i], i, y[i]);
    }
}

void zero(REAL *A, long n) {
    long i;
    for (i = 0; i < n; i++) {
        A[i] = 0.0;
    }
}

/* initialize a vector with random floating point numbers */

void init(REAL *A, long n) {
    long i;
    for (i = 0; i < n; i++) {
        A[i] = ((REAL) (drand48()) + 13);
    }
}

REAL check(REAL *A, REAL *B, long n) {
    long i;
    REAL sum = 0.0;
    for (i = 0; i < n; i++) {
//	printf("A[%d]: %f, B[%d]: %f\n", i, A[i], i, B[i]);
        sum += fabs(A[i] - B[i]);
    }
    return sum;
}

extern int matvec_mdev_v;

int main(int argc, char *argv[]) {
    int status = 0;
    omp_init_devices();
    long n = 256;
    REAL *y;
    REAL *y_ompacc;
    REAL *x;
    REAL *a;
    //n = 500000;
    printf("usage: matvec [n] (default %d) [2|3|4], 2: block_block, 3: block_align, 4 align_auto (policy), default 2\n\n", n);
    if (argc >= 2) n = atoi(argv[1]);
    if (argc >= 3) matvec_mdev_v = atoi(argv[2]);

    a = ((REAL *) (omp_unified_malloc(n * n * sizeof(REAL))));
    x = ((REAL *) (omp_unified_malloc((n * sizeof(REAL)))));
    y = ((REAL *) (malloc((n * sizeof(REAL)))));
    y_ompacc = ((REAL *) (omp_unified_malloc((n * sizeof(REAL)))));

    srand48(1 << 12);
    init(x, n);
    init(y, n);
    init(a, n * n);

    //init(y,n);
    memcpy(y_ompacc, y, (n * sizeof(REAL)));
    REAL omp_time = read_timer_ms();
    // reference serial execution for error checking
    int i; int num_its = 20; for (i=0;i<num_its;i++)matvec(a, x,y,n);
    omp_time = (read_timer_ms() - omp_time)/num_its;
    double ompacc_time = matvec_ompacc_mdev(a, x, y_ompacc, n);
    omp_fini_devices();
    REAL cksm;
    cksm = check(y,y_ompacc,n) ;
    printf("matvec(%d): checksum: %g; time(ms):\tSerial\t\tOMPACC(%d devices)\n", n, cksm,
           omp_get_num_active_devices());
    printf("\t\t\t\t\t\t%4f\t%4f\n", omp_time, ompacc_time);
    free(y);
    omp_unified_free(y_ompacc);
    omp_unified_free(x);
    omp_unified_free(a);
    return 0;
}
