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
#include "homp.h"

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
        A[(p_index_ * n) + _p_j] = 0.0;
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
        A[(p_index_ * n) + _p_j] = (drand48());
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
      double diff = ((A[(i * n) + j] - B[(i * n) + j]) / A[(i * n) + j]);
//        printf("%4f -- %4f\n", A[i*n+j], B[i*n+j]);
      if (diff < 0) 
        diff = -diff;
      if (diff > error) 
        error = diff;
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
        c += (A[(i * n) + j] * B[(j * n) + k]);
      C[(i * n) + k] = c;
    }
}

struct OUT__3__7117___data 
{
  void *A_p;
  void *B_p;
  void *C_p;
  void *n_p;
}
;
static void OUT__3__7117__(void *__out_argv);

void omp_matmul(float *A,float *B,float *C,int n)
{
  int i;
  int j;
  int k;
  struct OUT__3__7117___data __out_argv2__7117__;
  __out_argv2__7117__.n_p = ((void *)(&n));
  __out_argv2__7117__.C_p = ((void *)(&C));
  __out_argv2__7117__.B_p = ((void *)(&B));
  __out_argv2__7117__.A_p = ((void *)(&A));
  XOMP_parallel_start(OUT__3__7117__,&__out_argv2__7117__,1,0,"/data/yy8/2013-8-multiple-gpu-work/benchmarks/matrixmultiplication/matmul.c",73);
  XOMP_parallel_end("/data/yy8/2013-8-multiple-gpu-work/benchmarks/matrixmultiplication/matmul.c",80);
}

void openacc_matmul(float *A,float *B,float *C,int n)
{
  int i;
  int j;
  int k;
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

struct OUT__1__7117___data 
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
static void OUT__1__7117__(void *__out_argv);

#if 0
/* multiple device */

/* A, C row-major partition */
void ompacc_matmul_mdev_v1(REAL *A, REAL *B, REAL *C, int n)
{
    int i, j, k;
#pragma omp target device(*) map(from:C{0:n}[0:n]>>(*)), map(to:n,A{0:n}[0:n]>>(*),B[0:n][0:n])
#pragma omp parallel for private(i,j,k) dist_iteration match_range C[:]
    for (i = 0; i < n; i++)
        for (k = 0; k < n; k++) {
            REAL c = 0.0;
            for (j = 0; j < n; j++)
                c += A[i * n + j] * B[j * n + k];
            C[i * n + k] = c;
        }
}

/* multiple device */
/* B, C column-major partition */
void ompacc_matmul_mdev_v2(REAL *A, REAL *B, REAL *C, int n)
{
    int i, j, k;
#pragma omp target device(*) map(from:C[0:n]{0:n}>>(*)), map(to:n,A[0:n][0:n],B[0:n]{0:n}>>(*)
    for (i = 0; i < n; i++)
#pragma omp parallel for private(i,j,k) dist_iteration match_range C{}[]
        for (k = 0; k < n; k++) {
            REAL c = 0.0;
            for (j = 0; j < n; j++)
                c += A[i * n + j] * B[j * n + k];
            C[i * n + k] = c;
        }
}

/* multiple device */
/* A,B, C row-column partition */
void ompacc_matmul_mdev_v3(REAL *A, REAL *B, REAL *C, int n)
{
    int i, j, k;
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
void matmul_ompacc_mdev(REAL *A, REAL *B, REAL *C,  int n, int dist);

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
  if (argc < 2) {
    fprintf(stderr,"Usage: matmul <n> [<1|2|3>]\n");
    fprintf(stderr,"\t 1: row dist; 2: column dist; 3: both row/column dist; default 1\n");
    fprintf(stderr,"\t num of active devices can be controlled by OMP_NUM_ACTIVE_DEVICES variable\n");
    exit(1);
  }
  n = atoi(argv[1]);
  int dist = 1;
  if (argc == 3) dist = atoi(argv[2]);
  A = ((float *)(malloc(((n * n) * sizeof(float )))));
  B = ((float *)(malloc(((n * n) * sizeof(float )))));
  C_seq = ((float *)(malloc(((n * n) * sizeof(float )))));
  C_omp_for = ((float *)(malloc(((n * n) * sizeof(float )))));
  C_acc = ((float *)(malloc(((n * n) * sizeof(float )))));
  srand48((1 << 12));
  struct OUT__1__7117___data __out_argv1__7117__;
  __out_argv1__7117__.C_acc_p = ((void *)(&C_acc));
  __out_argv1__7117__.C_omp_for_p = ((void *)(&C_omp_for));
  __out_argv1__7117__.C_seq_p = ((void *)(&C_seq));
  __out_argv1__7117__.B_p = ((void *)(&B));
  __out_argv1__7117__.A_p = ((void *)(&A));
  __out_argv1__7117__.num_threads_p = ((void *)(&num_threads));
  __out_argv1__7117__.n_p = ((void *)(&n));
  XOMP_parallel_start(OUT__1__7117__,&__out_argv1__7117__,1,0,"/data/yy8/2013-8-multiple-gpu-work/benchmarks/matrixmultiplication/matmul.c",152);
  XOMP_parallel_end("/data/yy8/2013-8-multiple-gpu-work/benchmarks/matrixmultiplication/matmul.c",163);
/* sequential run */
  seq_elapsed = omp_get_wtime();
//  iter_matmul(A, B, C_seq, n);
  seq_elapsed = (omp_get_wtime() - seq_elapsed);
/* openmp parallel for version */
  omp_for_elapsed = omp_get_wtime();
  omp_matmul(A, B, C_omp_for, n);
  omp_for_elapsed = (omp_get_wtime() - omp_for_elapsed);
/* we currently cannot do the OpenMP acc and OpenACC run in once */
#ifndef OPENACC
/* openmp acc version */
  omp_init_devices();
  acc_elapsed = omp_get_wtime();
  matmul_ompacc_mdev(A,B,C_acc,n, dist);
  acc_elapsed = (omp_get_wtime() - acc_elapsed);
#else
#endif
  printf("=======================================================================\n");
  printf("\t\tmatmul(%dx%d) example on %d threads(cores)\n",n,n,num_threads);
  printf("-----------------------------------------------------------------------\n");
  printf("Performance:  Runtime (ms)\t MFLOPS\t\t\t Error\n");
  printf("-----------------------------------------------------------------------\n");
  printf("Sequential      :  %4f \t\t %4f\t\t%g\n",seq_elapsed*1.0e3,((((2.0 * n) * n) * n) / (1.0e6 * seq_elapsed)),maxerror(C_seq,C_seq,n));
  printf("OMP For         :  %4f \t\t %4f\t\t%g\n",omp_for_elapsed*1.0e3,((((2.0 * n) * n) * n) / (1.0e6 * omp_for_elapsed)),maxerror(C_omp_for,C_omp_for,n));
#ifndef OPENACC
  printf("OMP ACC         :  %4f \t\t %4f\t\t%g\n",acc_elapsed*1.0e3,((((2.0 * n) * n) * n) / (1.0e6 * acc_elapsed)),maxerror(C_omp_for,C_acc,n));
  printf("\t%d devices, dist policy: %d (1: row; 2: column; 3: row-column)\n", omp_get_num_active_devices(), dist);
#else
#endif
  free(C_acc);
  free(C_omp_for);
  free(C_seq);
  free(B);
  free(A);
  return 0;
}

static void OUT__1__7117__(void *__out_argv)
{
  int *n = (int *)(((struct OUT__1__7117___data *)__out_argv) -> n_p);
  int *num_threads = (int *)(((struct OUT__1__7117___data *)__out_argv) -> num_threads_p);
  float **A = (float **)(((struct OUT__1__7117___data *)__out_argv) -> A_p);
  float **B = (float **)(((struct OUT__1__7117___data *)__out_argv) -> B_p);
  float **C_seq = (float **)(((struct OUT__1__7117___data *)__out_argv) -> C_seq_p);
  float **C_omp_for = (float **)(((struct OUT__1__7117___data *)__out_argv) -> C_omp_for_p);
  float **C_acc = (float **)(((struct OUT__1__7117___data *)__out_argv) -> C_acc_p);
  if (XOMP_master()) {
     *num_threads = omp_get_num_threads();
  }
  init( *A, *n);
  init( *B, *n);
  zero( *C_seq, *n);
  zero( *C_omp_for, *n);
  zero( *C_acc, *n);
}

static void OUT__3__7117__(void *__out_argv)
{
  float **A = (float **)(((struct OUT__3__7117___data *)__out_argv) -> A_p);
  float **B = (float **)(((struct OUT__3__7117___data *)__out_argv) -> B_p);
  float **C = (float **)(((struct OUT__3__7117___data *)__out_argv) -> C_p);
  int *n = (int *)(((struct OUT__3__7117___data *)__out_argv) -> n_p);
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
        c += (( *A)[(p_index_ *  *n) + _p_j] * ( *B)[(_p_j *  *n) + _p_k]);
      ( *C)[(p_index_ *  *n) + _p_k] = c;
    }
  }
  XOMP_barrier();
}

__global__ void OUT__1__11058__(int i, int j,int k,float *_dev_a,float *_dev_b,float *_dev_c)
{
  int ij;
  int  _dev_i, _dev_j, _dev_k;
  int _dev_lower, _dev_upper;
  // variables for adjusted loop info considering both original chunk size and step(strip)
  int _dev_loop_chunk_size;
  int _dev_loop_sched_index;
  int _dev_loop_stride;

  // 1-D thread block: 
  int _dev_thread_num = gridDim.x * blockDim.x;
  int _dev_thread_id = blockDim.x * blockIdx.x + threadIdx.x;

  //adjust bound to be inclusive later
  int orig_start =0;
  int orig_end = i*j-1;
  int orig_step = 1;
  int orig_chunk_size = 1;

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

/* compiler should generate three of and three laucher, for simplicity, we use vx to indicate whether
 * this is for v1, v2 or v3 version of the code
 */
struct OUT__1__11058__args {
	int i; int j; int k;
	int dist;
};

void OUT__1__11058__launcher (omp_offloading_t * off, void *args) {
    struct OUT__1__11058__args * iargs = (struct OUT__1__11058__args*) args;
    int i = iargs->i;
    int j = iargs->j;
    int k = iargs->k;
	int dist = iargs->dist;

    omp_offloading_info_t * off_info = off->off_info;
//    printf("off: %X, off_info: %X, devseqid: %d\n", off, off_info, off->devseqid);
    omp_data_map_t * map_A = &off_info->data_map_info[0].maps[off->devseqid]; /* 0 means the map A */
    omp_data_map_t * map_B = &off_info->data_map_info[1].maps[off->devseqid]; /* 1 means the map B */
    omp_data_map_t * map_C = &off_info->data_map_info[2].maps[off->devseqid]; /* 2 means the map C */

    REAL * A = (REAL *)map_A->map_dev_ptr;
    REAL * B = (REAL *)map_B->map_dev_ptr;
    REAL * C = (REAL *)map_C->map_dev_ptr;

	long start;
	if (dist == 1) {
		omp_loop_map_range(map_A, 0, -1, -1, &start, &i);
	} else if (dist == 2) {
		omp_loop_map_range(map_B, 1, -1, -1, &start, &j);
	} else /* vx == 3) */ {
		omp_loop_map_range(map_A, 0, -1, -1, &start, &j);
		omp_loop_map_range(map_B, 1, -1, -1, &start, &k);
	}

	omp_device_type_t devtype = off_info->targets[off->devseqid]->type;
#if defined (DEVICE_NVGPU_SUPPORT)
	if (devtype == OMP_DEVICE_NVGPU) {
		int _threads_per_block_ = xomp_get_maxThreadsPerBlock();
		int _num_blocks_ = xomp_get_max1DBlock(length_i*n);
		//	printf("device: %d, range: %d:%d\n", __i__, start_i, length_i);
		OUT__1__11058__<<<_num_blocks_,_threads_per_block_, 0, off->stream.systream.cudaStream>>>(i, j, k, (REAL *)map_A->map_dev_ptr, (REAL *)map_B->map_dev_ptr, (REAL *)map_C->map_dev_ptr);
	} else
#endif
	if (devtype == OMP_DEVICE_LOCALTH) {
		int ii, jj, kk;
		for (ii=0; ii<i; ii++) {
			for (jj=0; jj<j; jj++) {
				REAL sum = 0.0;
				for (kk=0; kk<k; kk++) {
					sum += A[jj*k+kk] * B[kk*j+jj];
				}
				C[ii*j+j] = sum;
			}
		}
	} else {
		fprintf(stderr, "device type is not supported for this call\n");
	}
}

void matmul_ompacc_mdev(REAL *A, REAL *B, REAL *C,  int n, int dist)
{
	    double ompacc_time = read_timer_ms();
	   /* get number of target devices specified by the programmers */
	    int __num_target_devices__ = omp_get_num_active_devices(); /*XXX: = runtime or compiler generated code */

		omp_device_t *__target_devices__[__num_target_devices__];
		/**TODO: compiler generated code or runtime call to init the __target_devices__ array */
		int __i__;
		for (__i__ = 0; __i__ < __num_target_devices__; __i__++) {
			__target_devices__[__i__] = &omp_devices[__i__]; /* currently this is simple a copy of the pointer */
		}

		/**TODO: compiler generated code or runtime call to init the topology */
		omp_grid_topology_t __top__;
		int __top_ndims__;
		/**************************************** dist-specific *****************************************/
		if (dist == 1 || dist == 2) __top_ndims__ = 1;
		else /* dist == 3 */ __top_ndims__ = 2;
		/************************************************************************************************/

		int __top_dims__[__top_ndims__];
		int __top_periodic__[__top_ndims__];
		int __id_map__[__num_target_devices__];
		omp_grid_topology_init_simple (&__top__, __target_devices__, __num_target_devices__, __top_ndims__, __top_dims__, __top_periodic__, __id_map__);

		int __num_mapped_variables__ = 3; /* XXX: need compiler output */
		omp_data_map_info_t __data_map_infos__[__num_mapped_variables__];

		/* A map info */
		omp_data_map_info_t * __info__ = &__data_map_infos__[0];
		long A_dims[2]; A_dims[0] = n; A_dims[1] = n;
		omp_data_map_dist_t A_dist[2];
		omp_data_map_init_info(__info__, &__top__, A, 2, A_dims, sizeof(REAL), OMP_DATA_MAP_TO, A_dist);
		__info__->maps = (omp_data_map_t *)alloca(sizeof(omp_data_map_t) * __num_target_devices__);

		/* B map info */
		__info__ = &__data_map_infos__[1];
		long B_dims[2]; B_dims[0] = n; B_dims[1] = n;
		omp_data_map_dist_t B_dist[2];

		if (dist == 1) omp_data_map_init_dist(&B_dist[1], 0, n-1, OMP_DATA_MAP_DIST_FULL, 0, 0, 0, 0);
		else if (dist == 2) omp_data_map_init_dist(&B_dist[1], 0, n-1, OMP_DATA_MAP_DIST_EVEN, 0, 0, 0, 0);
		else /* dist == 3 */ omp_data_map_init_dist(&B_dist[1], 0, n-1, OMP_DATA_MAP_DIST_FULL, 0, 0, 0, 1);

		omp_data_map_init_info(__info__, &__top__, B, 2, B_dims, sizeof(REAL), OMP_DATA_MAP_TO, B_dist);
		__info__->maps = (omp_data_map_t *)alloca(sizeof(omp_data_map_t) * __num_target_devices__);

		__info__ = &__data_map_infos__[2];
		long C_dims[2]; C_dims[0] = n; C_dims[1] = n;
		omp_data_map_dist_t C_dist[2];
		omp_data_map_init_info(__info__, &__top__, C, 2, C_dims, sizeof(REAL), OMP_DATA_MAP_FROM, C_dist);
		__info__->maps = (omp_data_map_t *)alloca(sizeof(omp_data_map_t) * __num_target_devices__);

		/**************************************** dist-specific *****************************************/
		if (dist == 1) {
			omp_data_map_init_dist(&A_dist[0], 0, n-1, OMP_DATA_MAP_DIST_EVEN, 0, 0, 0, 0);
			omp_data_map_init_dist(&A_dist[1], 0, n-1, OMP_DATA_MAP_DIST_FULL, 0, 0, 0, 0);

			omp_data_map_init_dist(&B_dist[0], 0, n-1, OMP_DATA_MAP_DIST_FULL, 0, 0, 0, 0);
			omp_data_map_init_dist(&B_dist[1], 0, n-1, OMP_DATA_MAP_DIST_FULL, 0, 0, 0, 0);

			omp_data_map_init_dist(&C_dist[0], 0, n-1, OMP_DATA_MAP_DIST_EVEN, 0, 0, 0, 0);
			omp_data_map_init_dist(&C_dist[1], 0, n-1, OMP_DATA_MAP_DIST_FULL, 0, 0, 0, 0);
		} else if (dist == 2) {
			omp_data_map_init_dist(&A_dist[0], 0, n-1, OMP_DATA_MAP_DIST_FULL, 0, 0, 0, 0);
			omp_data_map_init_dist(&A_dist[1], 0, n-1, OMP_DATA_MAP_DIST_FULL, 0, 0, 0, 0);

			omp_data_map_init_dist(&B_dist[0], 0, n-1, OMP_DATA_MAP_DIST_FULL, 0, 0, 0, 0);
			omp_data_map_init_dist(&B_dist[1], 0, n-1, OMP_DATA_MAP_DIST_EVEN, 0, 0, 0, 0);

			omp_data_map_init_dist(&C_dist[0], 0, n-1, OMP_DATA_MAP_DIST_FULL, 0, 0, 0, 0);
			omp_data_map_init_dist(&C_dist[1], 0, n-1, OMP_DATA_MAP_DIST_EVEN, 0, 0, 0, 0);
		} else /* dist == 3 */ {
			omp_data_map_init_dist(&A_dist[0], 0, n-1, OMP_DATA_MAP_DIST_EVEN, 0, 0, 0, 0);
			omp_data_map_init_dist(&A_dist[1], 0, n-1, OMP_DATA_MAP_DIST_FULL, 0, 0, 0, 1);

			omp_data_map_init_dist(&B_dist[0], 0, n-1, OMP_DATA_MAP_DIST_FULL, 0, 0, 0, 0);
			omp_data_map_init_dist(&B_dist[1], 0, n-1, OMP_DATA_MAP_DIST_EVEN, 0, 0, 0, 1);

			omp_data_map_init_dist(&C_dist[0], 0, n-1, OMP_DATA_MAP_DIST_EVEN, 0, 0, 0, 0);
			omp_data_map_init_dist(&C_dist[1], 0, n-1, OMP_DATA_MAP_DIST_EVEN, 0, 0, 0, 1);
		}
		/************************************************************************************************/

		struct OUT__1__11058__args args;
		args.i = n;
		args.j = n;
		args.k = n;
		args.dist = dist;
		omp_offloading_info_t __offloading_info__;
		__offloading_info__.offloadings = (omp_offloading_t *) alloca(sizeof(omp_offloading_t) * __num_target_devices__);
		/* we use universal args and launcher because axpy can do it */
		omp_offloading_init_info (&__offloading_info__, &__top__, __target_devices__, __num_mapped_variables__, __data_map_infos__, OUT__1__11058__launcher, &args);

		/*********** NOW notifying helper thread to work on this offload ******************/
	#if DEBUG_MSG
		 printf("=========================================== offloading to %d targets ==========================================\n", __num_target_devices__);
	#endif
		/* here we do not need sync start */
		omp_offloading_notify_and_wait_completion(__target_devices__, __num_target_devices__, &__offloading_info__);
		ompacc_time = read_timer_ms() - ompacc_time;
		double cpu_total = ompacc_time;

#if 0
	    float A_map_to_elapsed[__num_target_devices__];
		float B_map_to_elapsed[__num_target_devices__];
		float kernel_elapsed[__num_target_devices__];
		float C_map_from_elapsed[__num_target_devices__];

		printf("=============================================================================================================================================\n");
		printf("=========================== GPU Results (%d GPUs) for C[][] = A[][]*B[][], A|B size: [%d][%d], time in ms (s/1000) ===============================\n", __num_target_devices__, n, n);
		float A_map_to_accumulated = 0.0;
		float B_map_to_accumulated = 0.0;
		float kernel_accumulated = 0.0;
		float C_map_from_accumulated = 0.0;
		float streamCreate_accumulated = 0.0;
		for (__i__ = 0; __i__ < __num_target_devices__; __i__++) {
			A_map_to_elapsed[__i__] = omp_stream_event_elapsed_ms(&__dev_stream__[__i__], 0);
			B_map_to_elapsed[__i__] = omp_stream_event_elapsed_ms(&__dev_stream__[__i__], 1);
			kernel_elapsed[__i__] = omp_stream_event_elapsed_ms(&__dev_stream__[__i__], 2);
			C_map_from_elapsed[__i__] = omp_stream_event_elapsed_ms(&__dev_stream__[__i__], 3);
			float total = A_map_to_elapsed[__i__] + B_map_to_elapsed[__i__] + kernel_elapsed[__i__] + C_map_from_elapsed[__i__];
			printf("device: %d, total: %4f\n", __i__, total);
			printf("\t\tstreamCreate overhead: %4f\n", streamCreate_elapsed[__i__]);
			printf("\t\tbreakdown: A map_to: %4f; B map_to: %4f; kernel: %4f; C map_from: %f\n", A_map_to_elapsed[__i__], B_map_to_elapsed[__i__], kernel_elapsed[__i__], C_map_from_elapsed[__i__]);
			printf("\t\tbreakdown: map_to (A and B): %4f; kernel: %4f; map_from (C): %f\n", A_map_to_elapsed[__i__] + B_map_to_elapsed[__i__], kernel_elapsed[__i__], C_map_from_elapsed[__i__]);
			A_map_to_accumulated += A_map_to_elapsed[__i__];
			B_map_to_accumulated += B_map_to_elapsed[__i__];
			kernel_accumulated += kernel_elapsed[__i__];
			C_map_from_accumulated += C_map_from_elapsed[__i__];
			streamCreate_accumulated += streamCreate_elapsed[__i__];
		}
		float total = A_map_to_accumulated + B_map_to_accumulated + kernel_accumulated + C_map_from_accumulated;
		printf("ACCUMULATED GPU time (%d GPUs): %4f\n", __num_target_devices__ , total);
		printf("\t\tstreamCreate overhead: %4f\n",streamCreate_accumulated);
		printf("\t\tbreakdown: A map_to: %4f, B map_to: %4f, kernel: %4f, C map_from %f\n", A_map_to_accumulated, B_map_to_accumulated, kernel_accumulated, C_map_from_accumulated);
		printf("\t\tbreakdown: map_to(A and B): %4f, kernel: %4f, map_from (C): %f\n", A_map_to_accumulated + B_map_to_accumulated, kernel_accumulated, C_map_from_accumulated);
		printf("AVERAGE GPU time (per GPU): %4f\n", total/__num_target_devices__);
		printf("\t\tbreakdown: A map_to: %4f, B map_to: %4f, kernel: %4f, C map_from %f\n", A_map_to_accumulated/__num_target_devices__, B_map_to_accumulated/__num_target_devices__, kernel_accumulated/__num_target_devices__, C_map_from_accumulated/__num_target_devices__);
		printf("\t\tbreakdown: map_to (A and B): %4f, kernel: %4f, map_from (C): %f\n", A_map_to_accumulated/__num_target_devices__ + B_map_to_accumulated/__num_target_devices__, kernel_accumulated/__num_target_devices__, C_map_from_accumulated/__num_target_devices__);

		double cpu_total = ompacc_time;
		printf("----------------------------------------------------------------\n");
		printf("Total time measured from CPU: %4f\n", cpu_total);
		printf("Total time measured without streamCreate: %4f\n", (cpu_total-streamCreate_accumulated));
		printf("AVERAGE total (CPU cost+GPU) per GPU: %4f\n", cpu_total/__num_target_devices__);
		printf("Total CPU cost: %4f\n", cpu_total - total/__num_target_devices__);
		printf("AVERAGE CPU cost per GPU: %4f\n", (cpu_total-total/__num_target_devices__)/__num_target_devices__);
		printf("==========================================================================================================================================\n");
#endif
}
