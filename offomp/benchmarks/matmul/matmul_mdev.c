/*
 * Rectangular matrix multiplication, started from MIT Cilk matmul.cilk example
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define REAL float

#include "homp.h"
#include "omp.h"

void zero(REAL *A, long n) {
    long i, j;
#pragma omp for private (i, j)
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            A[i * n + j] = 0.0;
        }
    }
}

void init(REAL *A, long n) {
    long i, j;

#pragma omp for private (i, j)
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            A[i * n + j] = (double) drand48();
        }
    }
}

void print_array(char *title, char *name, REAL *A, long m, long n) {
    printf("%s:\n", title);
    long i, j;
    for (i = 0; i < m; i++) {
        for (j = 0; j < n; j++) {
            printf("%s[%d][%d]:%f\n", name, i, j, A[i * n + j]);
        }
    }
    printf("\n");
}

double maxerror(REAL *A, REAL *B, long n) {
    long i, j;
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


void iter_matmul(float *A, float *B, float *C, long n) {
    long i, j, k;
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++) {
            float c = 0.0;
            for (k = 0; k < n; k++)
                c += (A[(i * n) + k] * B[(k * n) + j]);
            C[(i * n) + j] = c;
        }
}

void omp_matmul(REAL *A, REAL *B, REAL *C, long n) {
    long i, j, k;
#pragma omp parallel for shared(A, B, C, n) private(i,j,k)
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++) {
            float c = 0.0;
            for (k = 0; k < n; k++)
                c += (A[(i * n) + k] * B[(k * n) + j]);
            C[(i * n) + j] = c;
        }
}

void openacc_matmul(float *A, float *B, float *C, long n) {
    long i, j, k;
/* #pragma acc kernels copyin(A[0:n][0:n],B[0:n][0:n]) copyout(C[0:n][0:n]) */
//#pragma acc kernels loop copyin(A[0:n*n],B[0:n*n]) copyout(C[0:n*n])

#pragma acc parallel loop copyin ( A [ 0 : n * n ], B [ 0 : n * n ] ) copyout ( C [ 0 : n * n ] ) collapse ( 2 )
    for (i = 0; i < n; i++)
        for (k = 0; k < n; k++) {
            float c = 0.0;
            for (j = 0; j < n; j++)
                c += (A[(i * n) + j] * B[(j * n) + k]);
            C[(i * n) + k] = c;
        }
}


#if 0
/* multiple device */

/* A, C row-major partition */
void ompacc_matmul_mdev_v1_block_block(REAL *A, REAL *B, REAL *C, long n) {
    long i, j, k;
#pragma omp target device(*) map(from:C[0:n][0:n] dist_data(BLOCK,DUPLICATE)),
     map(to:n,A[0:n][0:n] dist_data(BLOCK, DUPLICATE), B[0:n][0:n] dist_data(DUPLICATE,DUPLICATE))
#pragma omp parallel for private(i,j,k) dist_iteration(BLOCK)
    for (i = 0; i < n; i++)
        for (k = 0; k < n; k++) {
            REAL c = 0.0;
            for (j = 0; j < n; j++)
                c += A[i * n + j] * B[j * n + k];
            C[i * n + k] = c;
        }
}

void ompacc_matmul_mdev_v1_block_align(REAL *A, REAL *B, REAL *C, long n) {
    long i, j, k;
#pragma omp target device(*) map(from:C[0:n][0:n] dist_data(BLOCK,DUPLICATE)),
     map(to:n,A[0:n][0:n] dist_data(ALIGN(C)), B[0:n][0:n] dist_data(DUPLICATE,DUPLICATE))
#pragma omp parallel for private(i,j,k) dist_iteration(ALIGN(C[]))
    for (i = 0; i < n; i++)
        for (k = 0; k < n; k++) {
            REAL c = 0.0;
            for (j = 0; j < n; j++)
                c += A[i * n + j] * B[j * n + k];
            C[i * n + k] = c;
        }
}

void ompacc_matmul_mdev_v1_align_auto(REAL *A, REAL *B, REAL *C, long n) {
    long i, j, k;
#pragma omp target device(*) map(from:C[0:n][0:n] dist_data(ALIGN(lp1),DUPLICATE)),
     map(to:n,A[0:n][0:n] dist_data(ALIGN(lp1),DUPLICATE), B[0:n][0:n] dist_data(DUPLICATE,DUPLICATE))
#pragma omp parallel for private(i,j,k) dist_iteration(AUTO)
lp1:    for (i = 0; i < n; i++)
        for (k = 0; k < n; k++) {
            REAL c = 0.0;
            for (j = 0; j < n; j++)
                c += A[i * n + j] * B[j * n + k];
            C[i * n + k] = c;
        }
}

/* multiple device */
/* B, C column-major partition */
void ompacc_matmul_mdev_v2(REAL *A, REAL *B, REAL *C, long n)
{
    long i, j, k;
#pragma omp target device(*) map(from:C[0:n]{0:n}>>(*)), map(to:n,A[0:n][0:n],B[0:n]{0:n}>>(*)
    for (i = 0; i < n; i++)
#pragma omp parallel for private(i,j,k) dist_iteration match_range C[]{}
        for (k = 0; k < n; k++) {
            REAL c = 0.0;
            for (j = 0; j < n; j++)
                c += A[i * n + j] * B[j * n + k];
            C[i * n + k] = c;
        }
}

/* multiple device */
/* A,B, C row-column partition */
void ompacc_matmul_mdev_v3(REAL *A, REAL *B, REAL *C, long n)
{
    long i, j, k;
#pragma omp target device(*)=>(:)(:) map(from:C{0:n}{0:n}>>{:}{:}), map(to:n,A{0:n}[0:n]>>{:}(:),B[0:n]{0:n}>>(:){:})
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

/**
 * For each of the above three version of the matmul onto multiple device, the differences between them are the data distribution method.
 * The actual compiler will normally generate three set of methods for each of the dist type, in this
 * hand-written compiler code, we combine them together
 * we use dist para to do that in the compiler-generated code to do that:
 * dist = 1: A/C row dist,
 * dist = 2: B/C column dist,
 * dist = 3: A-row, B-column dist
 */
double matmul_ompacc_mdev(REAL *A, REAL *B, REAL *C, long n, int dist_dim, int dist_policy);

int main(int argc, char *argv[]) {
    long n;
    float *A;
    float *B;
    float *C_seq;
    float *C_ompacc;
    double seq_elapsed;
    double ompacc_elapsed;
    if (argc < 2) {
        fprintf(stderr, "Usage: matmul <n> [dist_dim(1|2|3)] [dist_policy(1|2|3)]\\n");
        fprintf(stderr, "\tn: matrix size (nxn)\n");
        fprintf(stderr, "\tdist_dim: 1: row dist; 2: column dist; 3: both row/column dist; default 1\n");
        fprintf(stderr, "\tdist_policy: 1: block_block; 2: block_align; 3: auto_align; default 1\n");
        exit(1);
    }
    n = atoi(argv[1]);
    int dist_dim = 1;
    int dist_policy = 1;
    if (argc == 3) dist_dim = atoi(argv[2]);
    if (argc == 4) dist_policy = atoi(argv[3]);
    if (dist_dim != 1 && dist_dim != 2 && dist_dim != 3) {
        fprintf(stderr, "Unknown dist dimensions: %d, now fall to default (1)\n", dist_dim);
        dist_dim = 1;
    }
    if (dist_policy != 1 && dist_policy != 2 && dist_policy != 3) {
        fprintf(stderr, "Unknown dist policy: %d, now fall to default (1)\n", dist_policy);
        dist_policy = 1;
    }

    A = ((float *) (omp_unified_malloc(((n * n) * sizeof(float)))));
    B = ((float *) (omp_unified_malloc(((n * n) * sizeof(float)))));
    C_seq = ((float *) (malloc(((n * n) * sizeof(float)))));
    C_ompacc = ((float *) (omp_unified_malloc(((n * n) * sizeof(float)))));
    srand48((1 << 12));
    init(A, n);
    init(B, n);

//  print_array("Array A", "A", A, n, n);
//  print_array("Array B", "B", B, n, n);

    zero(C_seq, n);
    zero(C_ompacc, n);

/* sequential run */
    seq_elapsed = read_timer_ms();
    int i;
    int num_its = 10;
    for (i=0; i<num_its;i++) iter_matmul(A, B, C_seq, n);
    seq_elapsed = (read_timer_ms() - seq_elapsed)/num_its;
    // print_array("Array C_seq", "C", C_seq, n, n);

/* we currently cannot do the OpenMP acc and OpenACC run in once */
/* openmp acc version */
    omp_init_devices();
    ompacc_elapsed = matmul_ompacc_mdev(A, B, C_ompacc, n, dist_dim, dist_policy);
    //print_array("Array C_ompacc", "C", C_ompacc, n, n);

    omp_fini_devices();

    printf("======================================================================================================\n");
    printf("\tmatmul(%dx%d) example on %d devices, dist policy: %d (1: row; 2: column; 3: row-column)\n",
           n, n, omp_get_num_active_devices(), dist_dim);
    printf("------------------------------------------------------------------------------------------------------\n");
    printf("Error: %g\n", maxerror(C_seq, C_ompacc, n));
    printf("------------------------------------------------------------------------------------------------------\n");
    printf("Performance:\t\tRuntime (ms)\t MFLOPS\n");
    printf("Sequential:\t\t%4f\t%4f\n", seq_elapsed, ((((2.0 * n) * n) * n) / (1.0e3 * seq_elapsed)));
    printf("OMPACC mdev:\t\t%4f\t%4f\n", ompacc_elapsed, ((((2.0 * n) * n) * n) / (1.0e3 * ompacc_elapsed)));
    omp_unified_free(C_ompacc);
    free(C_seq);
    omp_unified_free(B);
    omp_unified_free(A);
    return 0;
}

#if defined (DEVICE_NVGPU_SUPPORT)
#include "xomp_cuda_lib_inlined.cu"
__global__ void OUT__1__11058__(long i, long j,long k,float *_dev_a,float *_dev_b,float *_dev_c)
{
  long ij;
  long  _dev_i, _dev_j, _dev_k;
  long _dev_lower, _dev_upper;
  // variables for adjusted loop info considering both original chunk size and step(strip)
  long _dev_loop_chunk_size;
  long _dev_loop_sched_index;
  long _dev_loop_stride;

  // 1-D thread block: 
  long _dev_thread_num = gridDim.x * blockDim.x;
  long _dev_thread_id = blockDim.x * blockIdx.x + threadIdx.x;

  //adjust bound to be inclusive later
  long orig_start =0;
  long orig_end = i*j-1;
  long orig_step = 1;
  long orig_chunk_size = 1;

//  XOMP_accelerator_loop_default (0, MSIZE*MSIZE -1 , 1, &_dev_lower, &_dev_upper);
  XOMP_static_sched_init (orig_start, orig_end, orig_step, orig_chunk_size, _dev_thread_num, _dev_thread_id, \
      & _dev_loop_chunk_size , & _dev_loop_sched_index, & _dev_loop_stride);

  //XOMP_accelerator_loop_default (1, (n-1)*(m-1)-1, 1, &_dev_lower, &_dev_upper);
  while (XOMP_static_sched_next (&_dev_loop_sched_index, orig_end,orig_step, _dev_loop_stride, _dev_loop_chunk_size, _dev_thread_num, _dev_thread_id, & _dev_lower, & _dev_upper))
  {
  for (ij = _dev_lower; ij <= _dev_upper; ij ++) 
//  for (_dev_i = _dev_lower; _dev_i<= _dev_upper; _dev_i ++) 
//    for (j = 0; j < MSIZE; j++)
    {
      _dev_i = ij/k;
      _dev_j = ij%k;
      float c= 0.0;
      for (_dev_k = 0; _dev_k < k; _dev_k++)
        c += _dev_a[_dev_i * k + _dev_k] * _dev_b[_dev_k * j + _dev_j];
      _dev_c[_dev_i * j + _dev_j] = c;
    }
  } // end while
}
#endif

/* compiler should generate three of and three laucher, for simplicity, we use vx to indicate whether
 * this is for v1, v2 or v3 version of the code
 */
struct OUT__1__11058__args {
    long i;
    long j;
    long k;
    REAL *A;
    REAL *B;
    REAL *C;
    int dist;
};

void OUT__1__11058__launcher(omp_offloading_t *off, void *args) {
    struct OUT__1__11058__args *iargs = (struct OUT__1__11058__args *) args;
    long i = iargs->i;
    long j = iargs->j;
    long k = iargs->k;
    int dist = iargs->dist;

    omp_data_map_t *map_A = omp_map_get_map(off, iargs->A, 0); /* 0 means the map A */
    omp_data_map_t *map_B = omp_map_get_map(off, iargs->B, 1);  /* 1 means the map B */
    omp_data_map_t *map_C = omp_map_get_map(off, iargs->C, 2); /* 2 means the map C */

    REAL *A = (REAL *) map_A->map_dev_ptr; /* A is ixk array */
    REAL *B = (REAL *) map_B->map_dev_ptr; /* B is kxj array */
    REAL *C = (REAL *) map_C->map_dev_ptr;
#if CORRECTNESS_CHECK
    printf("kernel launcher: A: %X, B: %X, C: %X\n", A, B, C);
    print_array("A in device: ", "Adev", A, i, k);
    print_array("B in device: ", "Bdev", B, i, k);
#endif

    long start;
    if (dist == 1) {
        omp_loop_get_range(off, 0, &start, &i);
    } else if (dist == 2) {
        omp_loop_get_range(off, 0, &start, &j);
    } else /* vx == 3) */ {
        omp_loop_get_range(off, 0, &start, &i);
        omp_loop_get_range(off, 0, &start, &j);
    }
    //printf("dist: %d, dev: %d, i: %d, j: %d, k: %d\n", dist, off->devseqid, i, j, k);
    omp_device_type_t devtype = off->dev->type;
#if defined (DEVICE_NVGPU_SUPPORT)
	if (devtype == OMP_DEVICE_NVGPU) {
		int threads_per_team = omp_get_optimal_threads_per_team(off->dev);
		int teams_per_league = omp_get_optimal_teams_per_league(off->dev, threads_per_team, i*j);
		//	printf("device: %d, range: %d:%d\n", __i__, start_i, length_i);
		OUT__1__11058__<<<teams_per_league,threads_per_team, 0, off->stream->systream.cudaStream>>>(i, j, k, (REAL *)A, (REAL *)B, (REAL *)C);
	} else
#endif
    if (devtype == OMP_DEVICE_THSIM || devtype == OMP_DEVICE_HOSTCPU) {
        long ii, jj, kk;
        //	omp_set_num_threads(off->dev->num_cores);
        //printf("%d cores on host\n", off->dev->num_cores);
//#pragma omp parallel for shared(A, B, C, i,j,k) private(ii, jj, kk)
        for (ii = 0; ii < i; ii++) {
            for (jj = 0; jj < j; jj++) {
                REAL sum = 0.0;
                for (kk = 0; kk < k; kk++) {
                    sum += A[ii * k + kk] * B[kk * j + jj];
                }
                C[ii * j + jj] = sum;
            }
        }
    } else {
        fprintf(stderr, "device type is not supported for this call\n");
    }
#if CORRECTNESS_CHECK
	print_array("C in device: ", "Cdev", C, i, j);
#endif
}

double matmul_ompacc_mdev(REAL *A, REAL *B, REAL *C, long n, int dist_dim, int dist_policy) {
    double ompacc_init_time = read_timer_ms();
    /* get number of target devices specified by the programmers */
    int __top_ndims__;
    /**************************************** dist-specific *****************************************/
    if (dist_dim == 1 || dist_dim == 2) __top_ndims__ = 1;
    else /* dist == 3 */__top_ndims__ = 2;
    /************************************************************************************************/
    /* use all the devices */
    int __num_targets__ = omp_get_num_active_devices(); /*XXX: = runtime or compiler generated code */
    omp_grid_topology_t * __top__ = omp_grid_topology_init_simple(__num_targets__, __top_ndims__);
    /* init other infos (dims, periodic, idmaps) of top if needed */

    int __num_maps__ = 3; /* XXX: need compiler output */

    /* we use universal args and launcher because matmul can do it */
    struct OUT__1__11058__args args;
    args.i = n;args.j = n;args.k = n;args.A = A;args.B = B;args.C = C;args.dist = dist_dim;
    omp_offloading_info_t * __off_info__ = omp_offloading_init_info("matmul kernel", __top__, 1, OMP_OFFLOADING_DATA_CODE, __num_maps__, OUT__1__11058__launcher, &args, 1);
    omp_offloading_append_profile_per_iteration(__off_info__, 2*n*n, n*n, n);

    /* A map info */
    omp_data_map_info_t *__A_map_info__ = &__off_info__->data_map_info[0];
    omp_data_map_init_info("A", __A_map_info__, __off_info__, A, 2, sizeof(REAL), OMP_DATA_MAP_TO, OMP_DATA_MAP_AUTO);
    omp_data_map_info_set_dims_2d(__A_map_info__, n, n);

    /* B map info */
    omp_data_map_info_t *__B_map_info__ = &__off_info__->data_map_info[1];
    omp_data_map_init_info("B", __B_map_info__, __off_info__, B, 2, sizeof(REAL), OMP_DATA_MAP_TO, OMP_DATA_MAP_AUTO);
    omp_data_map_info_set_dims_2d(__B_map_info__, n, n);

    /* C map info */
    omp_data_map_info_t *__C_map_info__ = &__off_info__->data_map_info[2];
    omp_data_map_init_info("C", __C_map_info__, __off_info__, C, 2, sizeof(REAL), OMP_DATA_MAP_FROM, OMP_DATA_MAP_AUTO);
    omp_data_map_info_set_dims_2d(__C_map_info__, n, n);

    /**************************************** dist-specific *****************************************/
    /* dist_policy: block_block: 1, block_align: 2, align_auto: 3 */
    if (dist_dim == 1) {
        if (dist_policy == 1) {
            /* block_block */
            omp_data_map_dist_init_info(__C_map_info__, 0, OMP_DIST_POLICY_BLOCK, 0, n, 0);
            omp_data_map_dist_init_info(__C_map_info__, 1, OMP_DIST_POLICY_DUPLICATE, 0, n, 0);
            omp_loop_dist_init_info(__off_info__, 0, OMP_DIST_POLICY_BLOCK, 0, n, 0);
            printf("BLOCK dist policy for arrays and loop dist\n");
        } else if (dist_policy == 2) {
            /* block_align */
            omp_data_map_dist_init_info(__C_map_info__, 0, OMP_DIST_POLICY_BLOCK, 0, n, 0);
            omp_data_map_dist_init_info(__C_map_info__, 1, OMP_DIST_POLICY_DUPLICATE, 0, n, 0);
            omp_loop_dist_align_with_data_map(__off_info__, 0, 0, __C_map_info__, 0);
            printf("BLOCK dist policy for arrays, and loop dist align with array C/A row dist\n");
        } else if (dist_policy == 3) {
            /* align_auto */
            omp_loop_dist_init_info(__off_info__, 0, OMP_DIST_POLICY_AUTO, 0, n, 0);
            omp_data_map_dist_align_with_loop(__C_map_info__, 0, 0, __off_info__, 0);
            omp_data_map_dist_init_info(__C_map_info__, 1, OMP_DIST_POLICY_DUPLICATE, 0, n, 0);

            printf("AUTO dist policy for loop dist and array align with loops\n");
        } else {

        }
        omp_data_map_dist_init_info(__B_map_info__, 0, OMP_DIST_POLICY_DUPLICATE, 0, n, 0);
        omp_data_map_dist_init_info(__B_map_info__, 1, OMP_DIST_POLICY_DUPLICATE, 0, n, 0);
        omp_data_map_dist_align_with_data_map(__A_map_info__, OMP_ALL_DIMENSIONS, 0, __C_map_info__, OMP_ALL_DIMENSIONS);
    } else if (dist_dim == 2) {
        omp_data_map_dist_init_info(__A_map_info__, 0, OMP_DIST_POLICY_DUPLICATE, 0, n, 0);
        omp_data_map_dist_init_info(__A_map_info__, 1, OMP_DIST_POLICY_DUPLICATE, 0, n, 0);

        omp_data_map_dist_init_info(__C_map_info__, 0, OMP_DIST_POLICY_DUPLICATE, 0, n, 0);
        omp_data_map_dist_init_info(__C_map_info__, 1, OMP_DIST_POLICY_BLOCK, 0, n, 0);

        omp_data_map_dist_align_with_data_map(__B_map_info__, OMP_ALL_DIMENSIONS, 0, __C_map_info__, OMP_ALL_DIMENSIONS);

        omp_loop_dist_init_info(__off_info__, 1, OMP_DIST_POLICY_BLOCK, 0, n, 0);
    } else /* dist == 3 */{
        omp_data_map_dist_init_info(__C_map_info__, 0, OMP_DIST_POLICY_BLOCK, 0, n, 0);
        omp_data_map_dist_init_info(__C_map_info__, 1, OMP_DIST_POLICY_BLOCK, 0, n, 1);

        omp_data_map_dist_init_info(__A_map_info__, 0, OMP_DIST_POLICY_BLOCK, 0, n, 0); /* or align with C[0] */
        omp_data_map_dist_init_info(__A_map_info__, 1, OMP_DIST_POLICY_DUPLICATE, 0, n, 1);

        omp_data_map_dist_init_info(__B_map_info__, 0, OMP_DIST_POLICY_DUPLICATE, 0, n, 0);
        omp_data_map_dist_init_info(__B_map_info__, 1, OMP_DIST_POLICY_BLOCK, 0, n, 1); /* or align with C[1] */

        omp_loop_dist_init_info(__off_info__, 0, OMP_DIST_POLICY_BLOCK, 0, n, 0);
        omp_loop_dist_init_info(__off_info__, 1, OMP_DIST_POLICY_BLOCK, 0, n, 1);
    }
    /************************************************************************************************/

    /*********** NOW notifying helper thread to work on this offload ******************/
#if DEBUG_MSG
	printf("=========================================== offloading to %d targets ==========================================\n", __num_target_devices__);
#endif
    ompacc_init_time = read_timer_ms() - ompacc_init_time;
    /* here we do not need sync start */
    double off_total = read_timer_ms();
    int it;
    int total_its = 20;
    for (it = 0; it < total_its; it++)
        omp_offloading_start(__off_info__, it == total_its - 1);
    off_total = (read_timer_ms() - off_total) / total_its;
#if defined (OMP_BREAKDOWN_TIMING)
    omp_print_map_info(__A_map_info__);
    omp_print_map_info(__B_map_info__);
    omp_print_map_info(__C_map_info__);
	omp_offloading_info_report_profile(__off_info__);
#endif
    omp_offloading_fini_info(__off_info__);
    omp_grid_topology_fini(__top__);

    off_total += ompacc_init_time;
    return off_total;
}
