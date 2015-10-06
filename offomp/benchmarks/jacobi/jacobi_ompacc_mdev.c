#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
// Add timing support
#include <sys/time.h>
#include "homp.h"

/************************************************************
 * program to solve a finite difference
 * discretization of Helmholtz equation :
 * (d2/dx2)u + (d2/dy2)u - alpha u = f
 * using Jacobi iterative method.
 *
 * Modified: Sanjiv Shah,       Kuck and Associates, Inc. (KAI), 1998
 * Author:   Joseph Robicheaux, Kuck and Associates, Inc. (KAI), 1998
 *
 * This c version program is translated by
 * Chunhua Liao, University of Houston, Jan, 2005
 *
 * Directives are used in this code to achieve parallelism.
 * All do loops are parallelized with default 'static' scheduling.
 *
 * Input :  n - grid dimension in x direction
 *          m - grid dimension in y direction
 *          alpha - Helmholtz constant (always greater than 0.0)
 *          tol   - error tolerance for iterative solver
 *          relax - Successice over relaxation parameter
 *          mits  - Maximum iterations for iterative solver
 *
 * On output
 *       : u(n,m) - Dependent variable (solutions)
 *       : f(n,m) - Right hand side function
 *************************************************************/

// flexible between REAL and double
#define REAL float
#define DEFAULT_DIMSIZE 256

void print_array(char * title, char * name, REAL * A, long n, long m) {
	printf("%s:\n", title);
	long i, j;
    for (i = 0; i < n; i++) {
        for (j = 0; j < m; j++) {
            printf("%s[%d][%d]:%f  ", name, i, j, A[i * m + j]);
        }
        printf("\n");
    }
    printf("\n");
}

void print_array_dev(char * title, int dev, char * name, REAL * A, long n, long m) {
	printf("%s, dev: %d\n", title, dev);
	long i, j;
    for (i = 0; i < n; i++) {
        for (j = 0; j < m; j++) {
            printf("%s%d[%d][%d]:%f  ", name, dev,i, j, A[i * m + j]);
        }
        printf("\n");
    }
    printf("\n");
}


/*      subroutine initialize (n,m,alpha,dx,dy,u,f)
 ******************************************************
 * Initializes data
 * Assumes exact solution is u(x,y) = (1-x^2)*(1-y^2)
 *
 ******************************************************/
void initialize(long n, long m, REAL alpha, REAL *dx, REAL * dy, REAL * u_p, REAL * f_p) {
	long i;
	long j;
	long xx;
	long yy;
    REAL (*u)[m] = (REAL(*)[m])u_p;
    REAL (*f)[m] = (REAL(*)[m])f_p;

//double PI=3.1415926;
	*dx = (2.0 / (n - 1));
	*dy = (2.0 / (m - 1));
	/* Initialize initial condition and RHS */
//#pragma omp parallel for private(xx,yy,j,i)
	for (i = 0; i < n; i++)
		for (j = 0; j < m; j++) {
			xx = ((int) (-1.0 + (*dx * (i - 1))));
			yy = ((int) (-1.0 + (*dy * (j - 1))));
			u[i][j] = 0.0;
			f[i][j] = (((((-1.0 * alpha) * (1.0 - (xx * xx)))
					* (1.0 - (yy * yy))) - (2.0 * (1.0 - (xx * xx))))
					- (2.0 * (1.0 - (yy * yy))));
		}
}

/*  subroutine error_check (n,m,alpha,dx,dy,u,f)
 implicit none
 ************************************************************
 * Checks error between numerical and exact solution
 *
 ************************************************************/
void error_check(long n, long m, REAL alpha, REAL dx, REAL dy, REAL * u_p, REAL * f_p) {
	int i;
	int j;
	REAL xx;
	REAL yy;
	REAL temp;
	REAL error;
	error = 0.0;
	REAL (*u)[m] = (REAL(*)[m])u_p;
	REAL (*f)[m] = (REAL(*)[m])f_p;
//#pragma omp parallel for private(xx,yy,temp,j,i) reduction(+:error)
	for (i = 0; i < n; i++)
		for (j = 0; j < m; j++) {
			xx = (-1.0 + (dx * (i - 1)));
			yy = (-1.0 + (dy * (j - 1)));
			temp = (u[i][j] - ((1.0 - (xx * xx)) * (1.0 - (yy * yy))));
			error = (error + (temp * temp));
		}
	error = (sqrt(error) / (n * m));
	printf("Solution Error: %2.6g\n", error);
}
void jacobi_seq(long n, long m, REAL dx, REAL dy, REAL alpha, REAL relax, REAL * u_p, REAL * f_p, REAL tol, int mits);
void jacobi_omp_mdev(long n, long m, REAL dx, REAL dy, REAL alpha, REAL relax, REAL *u_p, REAL *f_p, REAL tol,
                     int mits);

int dist_dim;
int dist_policy;

int main(int argc, char * argv[]) {
	long n = DEFAULT_DIMSIZE;
	long m = DEFAULT_DIMSIZE;
	REAL alpha = 0.0543;
	REAL tol = 0.0000000001;
	REAL relax = 1.0;
	int mits = 5000;

    fprintf(stderr,"Usage: jacobi [dist_dim(1|2|3)] [dist_policy(1|2|3)] [<n> <m> <alpha> <tol> <relax> <mits>]\n");
	fprintf(stderr, "\tdist_dim: 1: row dist; 2: column dist; 3: both row/column dist; default 1\n");
	fprintf(stderr, "\tdist_policy: 1: block_block; 2: block_align; 3: auto_align; default 1\n");
    fprintf(stderr, "\tn - grid dimension in x direction, default: %d\n", n);
    fprintf(stderr, "\tm - grid dimension in y direction, default: n if provided or %d\n", m);
    fprintf(stderr, "\talpha - Helmholtz constant (always greater than 0.0), default: %g\n", alpha);
    fprintf(stderr, "\ttol   - error tolerance for iterative solver, default: %g\n", tol);
    fprintf(stderr, "\trelax - Successice over relaxation parameter, default: %g\n", relax);
    fprintf(stderr, "\tmits  - Maximum iterations for iterative solver, default: %d\n", mits);

	dist_dim = 1;
	dist_policy = 1;
	if (argc >= 2) dist_dim = atoi(argv[1]);
	if (argc >= 3) dist_policy = atoi(argv[2]);
	if (dist_dim != 1 && dist_dim != 2 && dist_dim != 3) {
		fprintf(stderr, "Unknown dist dimensions: %d, now fall to default (1)\n", dist_dim);
		dist_dim = 1;
	}
	if (dist_policy != 1 && dist_policy != 2 && dist_policy != 3) {
		fprintf(stderr, "Unknown dist policy: %d, now fall to default (1)\n", dist_policy);
		dist_policy = 1;
	}

    if (argc == 4)      { sscanf(argv[3], "%d", &n); m = n; }
    else if (argc == 5) { sscanf(argv[3], "%d", &n); sscanf(argv[4], "%d", &m); }
    else if (argc == 6) { sscanf(argv[3], "%d", &n); sscanf(argv[4], "%d", &m); sscanf(argv[5], "%g", &alpha); }
    else if (argc == 7) { sscanf(argv[3], "%d", &n); sscanf(argv[4], "%d", &m); sscanf(argv[5], "%g", &alpha); sscanf(argv[6], "%g", &tol); }
    else if (argc == 8) { sscanf(argv[3], "%d", &n); sscanf(argv[4], "%d", &m); sscanf(argv[5], "%g", &alpha); sscanf(argv[6], "%g", &tol); sscanf(argv[7], "%g", &relax); }
    else if (argc == 9) { sscanf(argv[3], "%d", &n); sscanf(argv[4], "%d", &m); sscanf(argv[5], "%g", &alpha); sscanf(argv[6], "%g", &tol); sscanf(argv[7], "%g", &relax); sscanf(argv[8], "%d", &mits); }
    else {
    	/* the rest of arg ignored */
    }
    printf("jacobi %d %d %g %g %g %d\n", n, m, alpha, tol, relax, mits);
    printf("------------------------------------------------------------------------------------------------------\n");
    /** init the array */
    //REAL u[n][m];
    //REAL f[n][m];
	omp_init_devices();

    REAL * u = (REAL *)malloc(sizeof(REAL)*n*m);
    REAL * f = (REAL *)malloc(sizeof(REAL)*n*m);

    REAL *udev = (REAL *)omp_unified_malloc(sizeof(REAL)*n*m);
    REAL *fdev = (REAL *)omp_unified_malloc(sizeof(REAL)*n*m);

    REAL dx; /* grid spacing in x direction */
    REAL dy; /* grid spacing in y direction */

    initialize(n, m, alpha, &dx, &dy, u, f);

    memcpy(udev, u, n*m*sizeof(REAL));
    memcpy(fdev, f, n*m*sizeof(REAL));

	REAL elapsed = read_timer_ms();
	//jacobi_seq(n, m, dx, dy, alpha, relax, u, f, tol, mits);
	elapsed = read_timer_ms() - elapsed;
	printf("seq elasped time(ms): %12.6g\n", elapsed);
	double mflops = (0.001*mits*(n-2)*(m-2)*13) / elapsed;
	printf("MFLOPS: %12.6g\n", mflops);

#if CORRECTNESS_CHECK
	print_array("udev", "udev", (REAL*)udev, n, m);
#endif

	elapsed = read_timer_ms();
	jacobi_omp_mdev(n, m, dx, dy, alpha, relax, udev, fdev, tol, mits);
	elapsed = read_timer_ms() - elapsed;
	printf("mdev elasped time(ms): %12.6g\n", elapsed);
	mflops = (0.001*mits*(n-2)*(m-2)*13) / elapsed;
	printf("MFLOPS: %12.6g\n", mflops);

	//print_array("Sequential Run", "u",(REAL*)u, n, m);

	error_check(n, m, alpha, dx, dy, u, f);
	free(u); free(f);
	omp_unified_free(udev);omp_unified_free(fdev);
	omp_fini_devices();

	return 0;
}

/*      subroutine jacobi (n,m,dx,dy,alpha,omega,u,f,tol,mits)
 ******************************************************************
 * Subroutine HelmholtzJ
 * Solves poisson equation on rectangular grid assuming :
 * (1) Uniform discretization in each direction, and
 * (2) Dirichlect boundary conditions
 *
 * Jacobi method is used in this routine
 *
 * Input : n,m   Number of grid points in the X/Y directions
 *         dx,dy Grid spacing in the X/Y directions
 *         alpha Helmholtz eqn. coefficient
 *         omega Relaxation factor
 *         f(n,m) Right hand side function
 *         u(n,m) Dependent variable/Solution
 *         tol    Tolerance for iterative solver
 *         mits  Maximum number of iterations
 *
 * Output : u(n,m) - Solution
 *****************************************************************/
void jacobi_seq(long n, long m, REAL dx, REAL dy, REAL alpha, REAL omega, REAL * u_p, REAL * f_p, REAL tol, int mits) {
	long i, j, k;
	REAL error;
	REAL ax;
	REAL ay;
	REAL b;
	REAL resid;
	REAL uold[n][m];
    REAL (*u)[m] = (REAL(*)[m])u_p;
    REAL (*f)[m] = (REAL(*)[m])f_p;
	/*
	 * Initialize coefficients */
	/* X-direction coef */
	ax = (1.0 / (dx * dx));
	/* Y-direction coef */
	ay = (1.0 / (dy * dy));
	/* Central coeff */
	b = (((-2.0 / (dx * dx)) - (2.0 / (dy * dy))) - alpha);
	error = (10.0 * tol);
	k = 1;
	while ((k <= mits) && (error > tol)) {
		error = 0.0;

		/* Copy new solution into old */
		for (i = 0; i < n; i++)
			for (j = 0; j < m; j++)
				uold[i][j] = u[i][j];

		for (i = 1; i < (n - 1); i++)
			for (j = 1; j < (m - 1); j++) {
				resid = (ax * (uold[i - 1][j] + uold[i + 1][j]) + ay * (uold[i][j - 1] + uold[i][j + 1]) + b * uold[i][j] - f[i][j]) / b;
				//printf("i: %d, j: %d, resid: %f\n", i, j, resid);

				u[i][j] = uold[i][j] - omega * resid;
				error = error + resid * resid;
			}
		/* Error check */
		if (k % 500 == 0)
		printf("Finished %d iteration with error: %g\n", k, error);
		error = sqrt(error) / (n * m);

		k = k + 1;
	} /*  End iteration loop */
	printf("Total Number of Iterations: %d\n", k);
	printf("Residual: %.15g\n", error);
}

#if defined (DEVICE_NVGPU_SUPPORT)
#include "xomp_cuda_lib_inlined.cu"

#define LOOP_COLLAPSE 1
#if !LOOP_COLLAPSE
__global__ void OUT__2__10550__(long n,long m,REAL *_dev_u,REAL *_dev_uold,long uold_m, int uold_0_offset, int uold_1_offset)
{
  long _p_j;
  long _dev_i ;
  long _dev_lower, _dev_upper;
  XOMP_accelerator_loop_default (0, n-1, 1, &_dev_lower, &_dev_upper);
  for (_dev_i = _dev_lower ; _dev_i <= _dev_upper; _dev_i++) {
    for (_p_j = 0; _p_j < m; _p_j++)
      _dev_uold[(_dev_i+uold_0_offset) * uold_m + _p_j+uold_1_offset] = _dev_u[_dev_i * m + _p_j];
  }
}

__global__ void OUT__1__10550__(long n,long m,REAL omega,REAL ax,REAL ay,REAL b,REAL *_dev_u,REAL *_dev_f, REAL *_dev_uold,
		long uold_m, int uold_0_offset, int uold_1_offset, int start_i, int start_j, REAL *_dev_per_block_error)

{
	long _p_j;
  REAL _p_error;
  _p_error = 0;
  REAL _p_resid;
  long _dev_lower, _dev_upper;
  XOMP_accelerator_loop_default (start_i, n-1, 1, &_dev_lower, &_dev_upper);
  long _dev_i;
  for (_dev_i = _dev_lower; _dev_i<= _dev_upper; _dev_i ++) {
    for (_p_j = 1; _p_j < (m - 1); _p_j++) { /* this only works for dist=1 partition */
      _p_resid = (((((ax * (_dev_uold[(_dev_i - 1 + uold_0_offset) * uold_m + _p_j+uold_1_offset] + _dev_uold[(_dev_i + 1+uold_0_offset) * uold_m + _p_j+uold_1_offset])) +
    		  (ay * (_dev_uold[(_dev_i+uold_0_offset) * uold_m + (_p_j - 1+uold_1_offset)] + _dev_uold[(_dev_i + uold_0_offset) * uold_m + (_p_j + 1+uold_1_offset)]))) + (b * _dev_uold[(_dev_i + uold_0_offset) * uold_m + _p_j+uold_1_offset])) -
    		  _dev_f[(_dev_i + uold_0_offset) * uold_m + _p_j+uold_1_offset]) / b);
      _dev_u[_dev_i * uold_m + _p_j] = (_dev_uold[(_dev_i + uold_0_offset) * uold_m + _p_j + uold_1_offset] - (omega * _p_resid));
      _p_error = (_p_error + (_p_resid * _p_resid));
    }
  }
  xomp_inner_block_reduction_float(_p_error,_dev_per_block_error,6);
}
#else
__global__ void OUT__2__10550__(long n,long m,REAL *_dev_u,REAL *_dev_uold,long uold_m, int uold_0_offset, int uold_1_offset)
{
	long _p_j;
	long ij;
	long _dev_lower, _dev_upper;

	long _dev_i ;

 // variables for adjusted loop info considering both original chunk size and step(strip)
	long _dev_loop_chunk_size;
	long _dev_loop_sched_index;
	long _dev_loop_stride;

// 1-D thread block:
	long _dev_thread_num = gridDim.x * blockDim.x;
	long _dev_thread_id = blockDim.x * blockIdx.x + threadIdx.x;

	long orig_start =0;
	long orig_end = n*m-1; // inclusive upper bound
	long orig_step = 1;
	long orig_chunk_size = 1;

 XOMP_static_sched_init (orig_start, orig_end, orig_step, orig_chunk_size, _dev_thread_num, _dev_thread_id, \
                         & _dev_loop_chunk_size , & _dev_loop_sched_index, & _dev_loop_stride);

 //XOMP_accelerator_loop_default (1, (n-1)*(m-1)-1, 1, &_dev_lower, &_dev_upper);
 while (XOMP_static_sched_next (&_dev_loop_sched_index, orig_end, orig_step,_dev_loop_stride, _dev_loop_chunk_size, _dev_thread_num, _dev_thread_id, & _dev_lower
, & _dev_upper))
 {
   for (ij = _dev_lower ; ij <= _dev_upper; ij ++) {
     //  for (_dev_i = _dev_lower ; _dev_i <= _dev_upper; _dev_i++) {
     //    for (_p_j = 0; _p_j < m; _p_j++)
     _dev_i = ij/m;
     _p_j = ij%m;
     _dev_uold[(_dev_i+uold_0_offset) * uold_m + _p_j+uold_1_offset] = _dev_u[_dev_i * m + _p_j];

   }
  }
 }

__global__ void OUT__1__10550__(long n,long m,REAL omega,REAL ax,REAL ay,REAL b,REAL *_dev_u,REAL *_dev_f, REAL *_dev_uold,
		long uold_m, int uold_0_offset, int uold_1_offset, int start_i, int start_j, REAL *_dev_per_block_error)
{
	long _dev_i;
	long ij;
	long _p_j;
	long _dev_lower, _dev_upper;

  REAL _p_error;
  _p_error = 0;
  REAL _p_resid;

  // variables for adjusted loop info considering both original chunk size and step(strip)
  long _dev_loop_chunk_size;
  long _dev_loop_sched_index;
  long _dev_loop_stride;

  // 1-D thread block:
  long _dev_thread_num = gridDim.x * blockDim.x;
  long _dev_thread_id = blockDim.x * blockIdx.x + threadIdx.x;

  //TODO: adjust bound to be inclusive later
  long orig_start =start_i*m;
  long orig_end = (n - start_i)*m-1; /* Linearized iteration space */
  long orig_step = 1;
  long orig_chunk_size = 1;

  XOMP_static_sched_init (orig_start, orig_end, orig_step, orig_chunk_size, _dev_thread_num, _dev_thread_id, \
      & _dev_loop_chunk_size , & _dev_loop_sched_index, & _dev_loop_stride);

  //XOMP_accelerator_loop_default (1, (n-1)*(m-1)-1, 1, &_dev_lower, &_dev_upper);
  while (XOMP_static_sched_next (&_dev_loop_sched_index, orig_end,orig_step, _dev_loop_stride, _dev_loop_chunk_size, _dev_thread_num, _dev_thread_id, & _dev_lower, & _dev_upper))
  {
    for (ij = _dev_lower; ij <= _dev_upper; ij ++) {
      _dev_i = ij/(m-1);
      _p_j = ij%(m-1);

      if (_dev_i>=start_i && _dev_i< (n) && _p_j>=1 && _p_j< (m-1)) // must preserve the original boudary conditions here!!
      {
    	  _p_resid = (((((ax * (_dev_uold[(_dev_i - 1 + uold_0_offset) * uold_m + _p_j+uold_1_offset] + _dev_uold[(_dev_i + 1+uold_0_offset) * uold_m + _p_j+uold_1_offset])) +
	    		  (ay * (_dev_uold[(_dev_i+uold_0_offset) * uold_m + (_p_j - 1+uold_1_offset)] + _dev_uold[(_dev_i + uold_0_offset) * uold_m + (_p_j + 1+uold_1_offset)]))) + (b * _dev_uold[(_dev_i + uold_0_offset) * uold_m + _p_j+uold_1_offset])) -
	    		  _dev_f[(_dev_i + uold_0_offset) * uold_m + _p_j+uold_1_offset]) / b);
	      _dev_u[_dev_i * uold_m + _p_j] = (_dev_uold[(_dev_i + uold_0_offset) * uold_m + _p_j + uold_1_offset] - (omega * _p_resid));
	      _p_error = (_p_error + (_p_resid * _p_resid));
      }
    }
  }

  xomp_inner_block_reduction_float(_p_error,_dev_per_block_error,6);
}
#endif /* LOOP_CLAPSE */
#endif /* NVGPU support */

struct OUT__2__10550__args {
	long n;
	long m;
	REAL * u;
	REAL * uold;
};

void OUT__2__10550__launcher(omp_offloading_t * off, void *args) {
    struct OUT__2__10550__args * iargs = (struct OUT__2__10550__args*) args;
    long n = iargs->n;
    long m = iargs->m;

    omp_data_map_t * map_u = omp_map_get_map(off, iargs->u, -1); /* 1 means the map u */
    //printf("omp_map_get_map for u: %X, map: %X\n", iargs->u, map_u);
    omp_data_map_t * map_uold = omp_map_get_map(off, iargs->uold, -1); /* 2 means the map uold */
    //printf("omp_map_get_map for uold: %X, map: %X\n", iargs->uold, map_uold);

    /* get the right length for each dimension */
	long start;
	if (dist_dim == 1) {
		omp_loop_get_range(off, 0, &start, &n);
	} else if (dist_dim == 2) {
		omp_loop_get_range(off, 0, &start, &m);
	} else /* vx == 3) */ {
		omp_loop_get_range(off, 0, &start, &n);
		omp_loop_get_range(off, 0, &start, &m);
	}

	/* get the right base address, and offset in each dimension,
	 * since uold has halo region, we need to deal it with carefully.
	 * for u, it is just the map_dev_ptr
	 * */
	long i, j;
    REAL * u_p = (REAL *)map_u->map_dev_ptr;
    REAL (*u)[m] = (REAL(*)[m])u_p;

    /* we need to adjust index offset for those who has halo region because of we use attached halo region memory management */
    REAL * uold_p = (REAL *)map_uold->map_dev_ptr;
    int uold_0_offset;
    int uold_1_offset;
//	printf("dev %d: src ptr: %X, src_wextra_ptr: %X, dev_ptr: %X, dev_wextra_ptr: %X\n", off->dev->id,
//		   map_uold->map_source_ptr, map_uold->map_source_wextra_ptr, map_uold->map_dev_ptr, map_uold->map_dev_wextra_ptr);

    if (omp_data_map_get_halo_left_devseqid(map_uold, 0) >= 0) {
    	uold_0_offset = map_uold->info->halo_info[0].left;
    } else uold_0_offset = 0;

    if (omp_data_map_get_halo_left_devseqid(map_uold, 1) >= 0) {
        uold_1_offset = map_uold->info->halo_info[1].left;
    } else uold_1_offset = 0;

    long uold_0_length = map_uold->map_dist[0].length;
    long uold_1_length = map_uold->map_dist[1].length;

    REAL (*uold)[uold_1_length] = (REAL(*)[uold_1_length])uold_p; /** cast a pointer to a 2-D array */

#if CORRECTNESS_CHECK
    BEGIN_SERIALIZED_PRINTF((off->devseqid));
	printf("devsid: %d, 0-d off: %d, 1-d off: %d\n", off->devseqid, uold_0_offset, uold_1_offset);
	END_SERIALIZED_PRINTF();
#endif

//	printf("dist: %d, dev: %d, n: %d, m: %d\n", dist, off->devseqid, n,m);

	omp_device_type_t devtype = off->dev->type;
#if defined (DEVICE_NVGPU_SUPPORT)
	if (devtype == OMP_DEVICE_NVGPU) {
		int threads_per_team = omp_get_optimal_threads_per_team(off->dev);
		int teams_per_league = omp_get_optimal_teams_per_league(off->dev, threads_per_team, n*m);
		OUT__2__10550__<<<teams_per_league, threads_per_team, 0,off->stream->systream.cudaStream>>>(n, m,(REAL*)u,(REAL*)uold, uold_1_length,uold_0_offset, uold_1_offset);
	} else
#endif
	if (devtype == OMP_DEVICE_THSIM || devtype == OMP_DEVICE_HOSTCPU) {
#pragma omp parallel for private(j,i) shared(m,n,uold,u,uold_0_offset,uold_1_offset)
		for (i=0; i < n; i++)
			for (j=0; j < m; j++) {
				/* since uold has halo region, here we need to adjust index to reflect the new offset */
				uold[i+uold_0_offset][j+uold_1_offset] = u[i][j];
			}
	} else {
		fprintf(stderr, "device type is not supported for this call\n");
	}
#if CORRECTNESS_CHECK
    BEGIN_SERIALIZED_PRINTF(off->devseqid);
	printf("udev: dev: %d, %dX%d\n", off->devseqid, n, m);
	print_array_dev("udev", off->devseqid, "u",(REAL*)u, n, m);
	printf("uolddev: dev: %d, %dX%d\n", off->devseqid, uold_0_length, uold_1_length);
	print_array_dev("uolddev", off->devseqid, "uold",(REAL*)uold, uold_0_length, uold_1_length);
	printf("uold: left_in_ptr: %X, left_out_ptr: %X, right_in_ptr: %X, right_out_ptr: %X\n", uold, &uold[1][0], &uold[uold_0_length-1][0],&uold[uold_0_length-2][0]);
	END_SERIALIZED_PRINTF();
#endif

	//printf("OUT__2__10550__launcher done\n");
}

struct OUT__1__10550__args {
	long n;
	long m;
	REAL ax;
	REAL ay;
	REAL b;
	REAL omega;
	REAL resid;
	REAL *error;

	REAL * u;
	REAL * uold;
	REAL * f;

};

void OUT__1__10550__launcher(omp_offloading_t * off, void *args) {
    struct OUT__1__10550__args * iargs = (struct OUT__1__10550__args*) args;
    long n = iargs->n;
    long m = iargs->m;
    REAL ax = iargs->ax;
    REAL ay = iargs->ay;
    REAL b = iargs->b;
    REAL omega = iargs->omega;
	REAL error = iargs->error[off->devseqid];
	REAL resid = iargs->resid;

    omp_data_map_t * map_f = omp_map_get_map(off, iargs->f, -1); /* 0 is for the map f, here we use -1 so it will search the offloading stack */
    omp_data_map_t * map_u = omp_map_get_map(off, iargs->u, -1); /* 1 is for the map u */
    omp_data_map_t * map_uold = omp_map_get_map(off, iargs->uold, -1); /* 2 is for the map uld */

    REAL * f_p = (REAL *)map_f->map_dev_ptr;
    REAL * u_p = (REAL *)map_u->map_dev_ptr;
    REAL (*f)[m] = (REAL(*)[m])f_p; /* cast pointer to array */
    REAL (*u)[m] = (REAL(*)[m])u_p;

    /* we need to adjust index offset for those who has halo region because of we use attached halo region memory management */
    REAL * uold_p = (REAL *)map_uold->map_dev_ptr;
//	printf("dev %d: src ptr: %X, src_wextra_ptr: %X, dev_ptr: %X, dev_wextra_ptr: %X\n", off->dev->id,
//		   map_uold->map_source_ptr, map_uold->map_source_wextra_ptr, map_uold->map_dev_ptr, map_uold->map_dev_wextra_ptr);
    int uold_0_offset;
    int uold_1_offset;

    if (omp_data_map_get_halo_left_devseqid(map_uold, 0) >= 0) {
    	uold_0_offset = map_uold->info->halo_info[0].left;
    } else uold_0_offset = 0;
    if (omp_data_map_get_halo_left_devseqid(map_uold, 1) >= 0) {
        uold_1_offset = map_uold->info->halo_info[1].left;
    } else uold_1_offset = 0;

	int uold_0_length = map_uold->map_dist[0].length;
    int uold_1_length = map_uold->map_dist[1].length;

    REAL (*uold)[uold_1_length] = (REAL(*)[uold_1_length])uold_p; /** cast a pointer to a 2-D array */

#if CORRECTNESS_CHECK
    printf("kernel launcher: u: %X, uold: %X\n", u, uold);
    print_array("u in device: ", "udev", u, n, m);
    print_array("uold in device: ", "uolddev", uold, n, m);
#endif

	long start;
	if (dist_dim == 1) {
		omp_loop_get_range(off, 0, &start, &n);
	} else if (dist_dim == 2) {
		omp_loop_get_range(off, 0, &start, &m);
	} else /* vx == 3) */ {
		omp_loop_get_range(off, 0, &start, &n);
		omp_loop_get_range(off, 0, &start, &m);
	}

	int i_start, j_start;

    if (omp_data_map_get_halo_left_devseqid(map_uold, 0) >= 0) {
    	i_start = 0;
    } else i_start = 1;

    if (omp_data_map_get_halo_left_devseqid(map_uold, 1) >= 0) {
    	j_start = 0;
    } else j_start = 1;

    if (omp_data_map_get_halo_right_devseqid(map_uold, 0) >= 0) {
    } else n = n - 1;

    if (omp_data_map_get_halo_right_devseqid(map_uold, 1) >= 0) {
    } else m = m - 1;

//	printf("dist: %d, dev: %d, n: %d, m: %d\n", dist, off->devseqid, n,m);

	omp_device_type_t devtype = off->dev->type;
#if defined (DEVICE_NVGPU_SUPPORT)
	if (devtype == OMP_DEVICE_NVGPU) {
		int threads_per_team = omp_get_optimal_threads_per_team(off->dev);
		int teams_per_league = omp_get_optimal_teams_per_league(off->dev, threads_per_team, n*m);

		/* for reduction operation */
		REAL * _dev_per_block_error = (REAL*)omp_map_malloc_dev(off->dev, teams_per_league * sizeof(REAL));
		//printf("dev: %d teams per league, err block mem: %X\n", teams_per_league, _dev_per_block_error);
		REAL _host_per_block_error[teams_per_league];
		//printf("%d device: original offset: %d, mapped_offset: %d, length: %d\n", __i__, offset_n, start_n, length_n);
		/* Launch CUDA kernel ... */
		/** since here we do the same mapping, so will reuse the _threads_per_block and _num_blocks */
		OUT__1__10550__<<<teams_per_league, threads_per_team,(threads_per_team * sizeof(REAL)),
				off->stream->systream.cudaStream>>>(n, m,
				omega, ax, ay, b, (REAL*)u, (REAL*)f, (REAL*)uold,uold_1_length, uold_0_offset, uold_1_offset, i_start, j_start, _dev_per_block_error);

		/* copy back the results of reduction in blocks */
		//printf("copy back reduced error: %X <-- %X\n", _host_per_block_error, _dev_per_block_error);
		omp_map_memcpy_from_async(_host_per_block_error, _dev_per_block_error, off->dev, sizeof(REAL)*teams_per_league, off->stream);
		omp_stream_sync(off->stream);

		iargs->error[off->devseqid] = xomp_beyond_block_reduction_float(_host_per_block_error, teams_per_league, XOMP_REDUCTION_PLUS);
		//cudaStreamAddCallback(__dev_stream__[__i__].systream.cudaStream, xomp_beyond_block_reduction_float_stream_callback, args, 0);
		omp_map_free_dev(off->dev, _dev_per_block_error);

	} else
#endif
	if (devtype == OMP_DEVICE_THSIM || devtype == OMP_DEVICE_HOSTCPU) {
#if CORRECTNESS_CHECK
	    BEGIN_SERIALIZED_PRINTF(off->devseqid);
		printf("udev: dev: %d, %dX%d\n", off->devseqid, n, m);
		print_array_dev("udev", off->devseqid, "u",(REAL*)u, n, m);
		printf("uolddev: dev: %d, %dX%d\n", off->devseqid, uold_0_length, uold_1_length);
		print_array_dev("uolddev", off->devseqid, "uold",(REAL*)uold, uold_0_length, uold_1_length);
		printf("i_start: %d, j_start: %d, n: %d, m: %d, uold_0_offset: %d, uold_1_offset: %d\n", i_start, j_start, n, m, uold_0_offset, uold_1_offset);
		print_array_dev("f", off->devseqid, "f",(REAL*)f, map_f->map_dim[0], map_f->map_dim[1]);

		END_SERIALIZED_PRINTF();
#endif

		int i, j;
#pragma omp parallel for private(resid,j,i) reduction(+:error)
		for (i=i_start; i <n; i++) {
			for (j=j_start; j <m; j++) {
				resid = (ax * (uold[i - 1 + uold_0_offset][j + uold_1_offset] + uold[i + 1 + uold_0_offset][j+uold_1_offset]) + ay * (uold[i+uold_0_offset][j - 1+uold_1_offset] + uold[i+uold_0_offset][j + 1+uold_1_offset]) + b * uold[i+uold_0_offset][j+uold_1_offset] - f[i][j]) / b;

				u[i][j] = uold[i+uold_0_offset][j+uold_1_offset] - omega * resid;
				error = error + resid * resid;
			}
		}
		iargs->error[off->devseqid] = error;

	} else {
		fprintf(stderr, "device type is not supported for this call\n");
	}
}

void jacobi_omp_mdev(long n, long m, REAL dx, REAL dy, REAL alpha, REAL omega, REAL *u_p, REAL *f_p, REAL tol,
                     int mits) {
	double start_time, compl_time;
	start_time = read_timer_ms();
	int i, j, k;
	REAL error;
	REAL ax;
	REAL ay;
	REAL b;
	REAL uold[n][m];
	/*
	 * Initialize coefficients */
	/* X-direction coef */
	ax = (1.0 / (dx * dx));
	/* Y-direction coef */
	ay = (1.0 / (dy * dy));
	/* Central coeff */
	b = (((-2.0 / (dx * dx)) - (2.0 / (dy * dy))) - alpha);
	error = (10.0 * tol);
	k = 1;
#if 0

/* ------------------------------------------------------------------------------------------------------------------------
 * dist_policy 1: BLOCK_BLOCK distribution, i.e. data and loop iteration all block distribution
 */
#pragma omp target data device(*) map(to:n, m, omega, ax, ay, b, f[0:n][0:m] dist_data(BLOCK,DUPLICATE)) \
  map(tofrom:u[0:n][0:m] dist_data(BLOCK,DUPLICATE)) map(alloc:uold[0:n][0:m] dist_data(BLOCK,DUPLICATE) halo(1,))
  while ((k<=mits) &&(error>tol)) {
    error = 0.0;

    /* Copy new solution into old */
#pragma omp target device(*)
#pragma omp parallel for private(i, j) dist_iteration(BLOCK)
      for(i=0;i<n;i++)
        for(j=0;j<m;j++)
          uold[i][j] = u[i][j];
#pragma omp halo exchange(uold)

#pragma omp target device(*)
#pragma omp parallel for private(resid,i,j) reduction(+:error) dist_iteration(BLOCK) // nowait
loop1:for (i=0;i<n;i++) {
         if (i==0||i==n-1) continue;
         for (j=1;j<(m-1);j++) {
            resid = (ax*(uold[i-1][j] + uold[i+1][j])\
              + ay*(uold[i][j-1] + uold[i][j+1])+ b * uold[i][j] - f[i][j])/b;

            u[i][j] = uold[i][j] - omega * resid;
            error = error + resid*resid ;
         }
      }

    /* Error check */
    if (k%500==0) printf("Finished %d iteration with error =%g\n",k, error);
    error = sqrt(error)/(n*m);

    k = k + 1;
  } /*  End iteration loop */

/* ------------------------------------------------------------------------------------------------------------------------
 * dist_policy 2: BLOCK_ALIGN distribution, i.e. Array f, u, uold are BLOCK dist along row, and distribution of the two loops
 * align with the array.
 */
#pragma omp target data device(*) map(to:n, m, omega, ax, ay, b, f[0:n][0:m] dist_data(BLOCK,DUPLICATE)) \
  map(tofrom:u[0:n][0:m] dist_data(BLOCK,DUPLICATE)) map(alloc:uold[0:n][0:m] dist_data(BLOCK,DUPLICATE) halo(1,))
  while ((k<=mits) &&(error>tol)) {
    error = 0.0;

    /* Copy new solution into old */
#pragma omp target device(*)
#pragma omp parallel for private(i, j) dist_iteration(ALIGN(u[*]))
      for(i=0;i<n;i++)
        for(j=0;j<m;j++)
          uold[i][j] = u[i][j];
#pragma omp halo exchange(uold)

#pragma omp target device(*)
#pragma omp parallel for private(resid,i,j) reduction(+:error) dist_iteration(ALIGN(u[*])) // nowait
loop1:for (i=0;i<n;i++) {
         if (i==0||i==n-1) continue;
         for (j=1;j<(m-1);j++) {
            resid = (ax*(uold[i-1][j] + uold[i+1][j])\
              + ay*(uold[i][j-1] + uold[i][j+1])+ b * uold[i][j] - f[i][j])/b;

            u[i][j] = uold[i][j] - omega * resid;
            error = error + resid*resid ;
         }
      }

    /* Error check */
    if (k%500==0) printf("Finished %d iteration with error =%g\n",k, error);
    error = sqrt(error)/(n*m);

    k = k + 1;
  } /*  End iteration loop */

/* ------------------------------------------------------------------------------------------------------------------------
 * dist_policy 3: AUTO_BLOCK distribution, i.e. loop iteration is AUTO-balancing dist, and array f, u, uold all align with loop
 */
#pragma omp target data device(*) map(to:n, m, omega, ax, ay, b, f[0:n][0:m] dist_data(ALIGN(loop1),DUPLICATE)) \
  map(tofrom:u[0:n][0:m] dist_data(ALIGN(loop1),DUPLICATE)) map(alloc:uold[0:n][0:m] dist_data(ALIGN(loop1),DUPLICATE) halo(1,))
  while ((k<=mits) &&(error>tol)) {
    error = 0.0;

    /* Copy new solution into old */
#pragma omp target device(*)
#pragma omp parallel for private(i, j) dist_iteration(ALIGN(loop1))
      for(i=0;i<n;i++)
        for(j=0;j<m;j++)
          uold[i][j] = u[i][j];
#pragma omp halo exchange(uold)

#pragma omp target device(*)
#pragma omp parallel for private(resid,i,j) reduction(+:error) dist_iteration(AUTO) // nowait
loop1:for (i=0;i<n;i++) {
         if (i==0||i==n-1) continue;
         for (j=1;j<(m-1);j++) {
            resid = (ax*(uold[i-1][j] + uold[i+1][j])\
              + ay*(uold[i][j-1] + uold[i][j+1])+ b * uold[i][j] - f[i][j])/b;

            u[i][j] = uold[i][j] - omega * resid;
            error = error + resid*resid ;
         }
      }

    /* Error check */
    if (k%500==0) printf("Finished %d iteration with error =%g\n",k, error);
    error = sqrt(error)/(n*m);

    k = k + 1;
  } /*  End iteration loop */

#endif
//  double ompacc_time = read_timer_ms();
  	/* get number of target devices specified by the programmers */
  	int __num_target_devices__ = omp_get_num_active_devices(); /*XXX: = runtime or compiler generated code */

  	omp_device_t *__target_devices__[__num_target_devices__ ];
  	/**TODO: compiler generated code or runtime call to init the __target_devices__ array */
  	int __i__;
  	for (__i__ = 0; __i__ < __num_target_devices__; __i__++) {
  		__target_devices__[__i__] = &omp_devices[__i__]; /* currently this is simple a copy of the pointer */
  	}

  	/**TODO: compiler generated code or runtime call to init the topology */
  	omp_grid_topology_t __top__;
  	int __top_ndims__;
  	/**************************************** dist-specific *****************************************/
  	if (dist_dim == 1 || dist_dim == 2) __top_ndims__ = 1;
  	else /* dist == 3 */__top_ndims__ = 2;
  	/************************************************************************************************/

  	int __top_dims__[__top_ndims__ ];
  	int __top_periodic__[__top_ndims__ ];
  	int __id_map__[__num_target_devices__ ];
	omp_grid_topology_init_simple(__num_target_devices__, __top_ndims__);

  	int __num_mapped_array__ = 3; /* XXX: need compiler output */
  	omp_data_map_info_t __data_map_infos__[__num_mapped_array__ ];

	omp_offloading_info_t __copy_data_off_info__;
	__copy_data_off_info__.offloadings = (omp_offloading_t *) alloca(sizeof(omp_offloading_t) * __num_target_devices__);
	/* we use universal args and launcher because axpy can do it */
	omp_offloading_init_info("data copy", &__top__, 0, OMP_OFFLOADING_DATA, __num_mapped_array__, NULL, NULL, 0);

	omp_offloading_info_t __uuold_exchange_off_info__;
	omp_offloading_t __offs_1__[__num_target_devices__];
	__uuold_exchange_off_info__.offloadings = __offs_1__;
	/* we use universal args and launcher because axpy can do it */
	struct OUT__2__10550__args args_1;
	args_1.n = n; args_1.m = m;args_1.u = (REAL*)u_p; args_1.uold = (REAL*)uold;
	__uuold_exchange_off_info__.per_iteration_profile.num_fp_operations = 0;
	__uuold_exchange_off_info__.per_iteration_profile.num_load = 1;
	__uuold_exchange_off_info__.per_iteration_profile.num_store = 1;
	omp_dist_info_t __uuold_exchange_loop_dist__[1];
	omp_offloading_init_info("u<->uold exchange kernel", &__top__, 1, OMP_OFFLOADING_CODE, 0, OUT__2__10550__launcher,
							 &args_1, 1);

	omp_offloading_info_t __jacobi_off_info__;
	omp_offloading_t __offs_2__[__num_target_devices__];
	__jacobi_off_info__.offloadings = __offs_2__;	  	/* we use universal args and launcher because axpy can do it */
	struct OUT__1__10550__args args_2;
	args_2.n = n; args_2.m = m; args_2.ax = ax; args_2.ay = ay; args_2.b = b; args_2.omega = omega;args_2.u = (REAL*)u_p; args_2.uold = (REAL*)uold; args_2.f = (REAL*) f_p;
	REAL __reduction_error__[__num_target_devices__]; args_2.error = __reduction_error__;

	__jacobi_off_info__.per_iteration_profile.num_fp_operations = 13*m;
	__jacobi_off_info__.per_iteration_profile.num_load = 7;
	__jacobi_off_info__.per_iteration_profile.num_store = 1;
	omp_dist_info_t __jacobi_off_loop_dist__[1];
	omp_offloading_init_info("jacobi kernel", &__top__, 1, OMP_OFFLOADING_CODE, 0, OUT__1__10550__launcher, &args_2, 1);

  	/* f map info */
  	omp_data_map_info_t * __info__ = &__data_map_infos__[0];
  	long f_dims[2];f_dims[0] = n;f_dims[1] = m;
  	omp_data_map_t f_maps[__num_target_devices__];
  	omp_dist_info_t f_dist[2];
	omp_data_map_init_info("f", __info__, &__copy_data_off_info__, f_p, 2, sizeof(REAL), OMP_DATA_MAP_TO,
						   OMP_DATA_MAP_AUTO);

  	/* u map info */
  	__info__ = &__data_map_infos__[1];
  	long u_dims[2];u_dims[0] = n;u_dims[1] = m;
  	omp_data_map_t u_maps[__num_target_devices__];
  	omp_dist_info_t u_dist[2];
  	//omp_data_map_halo_region_info_t u_halo[2];
	omp_data_map_init_info("u", __info__, &__copy_data_off_info__, u_p, 2, sizeof(REAL), OMP_DATA_MAP_TOFROM,
						   OMP_DATA_MAP_AUTO);

  	/* uold map info */
  	__info__ = &__data_map_infos__[2];
  	long uold_dims[2];uold_dims[0] = n; uold_dims[1] = m;
  	omp_data_map_t uold_maps[__num_target_devices__];
  	omp_dist_info_t uold_dist[2];
  	omp_data_map_halo_region_info_t uold_halo[2];
  	omp_data_map_init_info_with_halo("uold", __info__, &__copy_data_off_info__, uold, 2, uold_dims, sizeof(REAL),uold_maps,OMP_DATA_MAP_ALLOC, OMP_DATA_MAP_AUTO, uold_dist, uold_halo);

	/* halo exchange offloading */
	omp_data_map_halo_exchange_info_t x_halos[1];
	x_halos[0].map_info = __info__; x_halos[0].x_direction = OMP_DATA_MAP_EXCHANGE_FROM_LEFT_RIGHT; /* uold */
	if (dist_dim == 1) {
		x_halos[0].x_dim = 0;
	} else if (dist_dim == 2) {
		x_halos[0].x_dim = 1;
	} else {
		x_halos[0].x_dim = -1; /* means all the dimension */
	}
//#define STANDALONE_DATA_X 1
#if !defined (STANDALONE_DATA_X)
	/* there are two approaches we handle halo exchange, appended data exchange or standalone one */
	/* option 1: appended data exchange */
	omp_offloading_append_data_exchange_info(&__uuold_exchange_off_info__, x_halos, 1);
#else
  	/* option 2: standalone offloading */
  	omp_offloading_info_t uuold_halo_x_off_info;
	omp_offloading_t uuold_halo_x_offs[__num_target_devices__];
	uuold_halo_x_off_info.offloadings = uuold_halo_x_offs;
  	omp_offloading_standalone_data_exchange_init_info("u-uold halo exchange", &uuold_halo_x_off_info, &__top__, __target_devices__, 1, 0, NULL, x_halos, 1);
#endif

  	/**************************************** dist-specific *****************************************/
  	if (dist_dim == 1) {
		if (dist_policy == 1) { /* BLOCK_BLOCK */
			omp_data_map_dist_init_info(&f_dist[0], 0, OMP_DIST_POLICY_BLOCK, 0, n, 0);
			omp_data_map_dist_init_info(&f_dist[1], 0, OMP_DIST_POLICY_DUPLICATE, 0, m, 0);

			omp_data_map_dist_init_info(&u_dist[0], 0, OMP_DIST_POLICY_BLOCK, 0, n, 0);
			omp_data_map_dist_init_info(&u_dist[1], 0, OMP_DIST_POLICY_DUPLICATE, 0, m, 0);

			omp_data_map_dist_init_info(&uold_dist[0], 0, OMP_DIST_POLICY_BLOCK, 0, n, 0);
			omp_data_map_dist_init_info(&uold_dist[1], 0, OMP_DIST_POLICY_DUPLICATE, 0, m, 0);
			omp_map_add_halo_region(&__data_map_infos__[2], 0, 1, 1, OMP_DIST_HALO_EDGING_REFLECTING);

			omp_data_map_dist_init_info(&__uuold_exchange_loop_dist__[0], 0, OMP_DIST_POLICY_BLOCK, 0, n, 0);
			omp_data_map_dist_init_info(&__jacobi_off_loop_dist__[0], 0, OMP_DIST_POLICY_BLOCK, 0, n, 0);
			printf("BLOCK dist policy for arrays and loop dist\n");
		} else if (dist_policy == 2) { /* BLOCK_ALIGN */
			omp_data_map_dist_init_info(&f_dist[0], 0, OMP_DIST_POLICY_BLOCK, 0, n, 0);
			omp_data_map_dist_init_info(&f_dist[1], 0, OMP_DIST_POLICY_DUPLICATE, 0, m, 0);

			omp_align_dist_info(&u_dist[0], OMP_DIST_POLICY_ALIGN, &__data_map_infos__[0],
								OMP_DIST_TARGET_DATA_MAP, 0);
			omp_align_dist_info(&u_dist[1], OMP_DIST_POLICY_ALIGN, &__data_map_infos__[0],
								OMP_DIST_TARGET_DATA_MAP, 1);

			omp_align_dist_info(&uold_dist[0], OMP_DIST_POLICY_ALIGN, &__data_map_infos__[0],
								OMP_DIST_TARGET_DATA_MAP, 0);
			omp_align_dist_info(&uold_dist[1], OMP_DIST_POLICY_ALIGN, &__data_map_infos__[0],
								OMP_DIST_TARGET_DATA_MAP, 1);
			omp_map_add_halo_region(&__data_map_infos__[2], 0, 1, 1, OMP_DIST_HALO_EDGING_REFLECTING);

			omp_align_dist_info(&__uuold_exchange_loop_dist__[0], OMP_DIST_POLICY_ALIGN, &__data_map_infos__[0],
								OMP_DIST_TARGET_DATA_MAP, 0);
			omp_align_dist_info(&__jacobi_off_loop_dist__[0], OMP_DIST_POLICY_ALIGN, &__data_map_infos__[0],
								OMP_DIST_TARGET_DATA_MAP, 0);
			printf("BLOCK dist policy for arrays, and loop dist align with array A row dist\n");
		} else if (dist_policy == 3) { /* AUTO_ALIGN */
			omp_data_map_dist_init_info(&__jacobi_off_loop_dist__[0], 0, OMP_DIST_POLICY_AUTO, 0, n, 0);

			omp_align_dist_info(&f_dist[0], OMP_DIST_POLICY_ALIGN, &__jacobi_off_info__,
								OMP_DIST_TARGET_LOOP_ITERATION, 0);
			omp_data_map_dist_init_info(&f_dist[1], 0, OMP_DIST_POLICY_DUPLICATE, 0, m, 0);

			omp_align_dist_info(&u_dist[0], OMP_DIST_POLICY_ALIGN, &__jacobi_off_info__,
								OMP_DIST_TARGET_LOOP_ITERATION, 0);
			omp_data_map_dist_init_info(&u_dist[1], 0, OMP_DIST_POLICY_DUPLICATE, 0, m, 0);

			omp_align_dist_info(&uold_dist[0], OMP_DIST_POLICY_ALIGN, &__jacobi_off_info__,
								OMP_DIST_TARGET_LOOP_ITERATION, 0);
			omp_data_map_dist_init_info(&uold_dist[1], 0, OMP_DIST_POLICY_DUPLICATE, 0, m, 0);
			omp_map_add_halo_region(&__data_map_infos__[2], 0, 1, 1, OMP_DIST_HALO_EDGING_REFLECTING);

			omp_align_dist_info(&__uuold_exchange_loop_dist__[0], OMP_DIST_POLICY_ALIGN, &__jacobi_off_info__,
								OMP_DIST_TARGET_LOOP_ITERATION, 0);
			printf("AUTO dist policy for loop dist and array align with loops\n");
		}
  	} else if (dist_dim == 2) {
		omp_data_map_dist_init_info(&f_dist[0], 0, OMP_DIST_POLICY_DUPLICATE, 0, n, 0);
		omp_data_map_dist_init_info(&f_dist[1], 0, OMP_DIST_POLICY_BLOCK, 0, m, 0);

		omp_data_map_dist_init_info(&u_dist[0], 0, OMP_DIST_POLICY_DUPLICATE, 0, n, 0);
		omp_data_map_dist_init_info(&u_dist[1], 0, OMP_DIST_POLICY_BLOCK, 0, m, 0);
  		//omp_map_add_halo_region(&__data_map_infos__[1], 1, 1, 1, 0);

		omp_data_map_dist_init_info(&uold_dist[0], 0, OMP_DIST_POLICY_DUPLICATE, 0, n, 0);
		omp_data_map_dist_init_info(&uold_dist[1], 0, OMP_DIST_POLICY_BLOCK, 0, m, 0);
  		omp_map_add_halo_region(&__data_map_infos__[2], 1, 1, 1, OMP_DIST_HALO_EDGING_REFLECTING);
  	} else /* dist == 3 */{
		omp_data_map_dist_init_info(&f_dist[0], 0, OMP_DIST_POLICY_BLOCK, 0, n, 0);
		omp_data_map_dist_init_info(&f_dist[1], 0, OMP_DIST_POLICY_BLOCK, 0, m, 1);

		omp_data_map_dist_init_info(&u_dist[0], 0, OMP_DIST_POLICY_BLOCK, 0, n, 0);
		omp_data_map_dist_init_info(&u_dist[1], 0, OMP_DIST_POLICY_BLOCK, 0, m, 1);
  		//omp_map_add_halo_region(&__data_map_infos__[1], 0, 1, 1, 0);
  		//omp_map_add_halo_region(&__data_map_infos__[1], 1, 1, 1, 0);

		omp_data_map_dist_init_info(&uold_dist[0], 0, OMP_DIST_POLICY_BLOCK, 0, n, 0);
		omp_data_map_dist_init_info(&uold_dist[1], 0, OMP_DIST_POLICY_BLOCK, 0, m, 1);
  		omp_map_add_halo_region(&__data_map_infos__[2], 0, 1, 1, OMP_DIST_HALO_EDGING_REFLECTING);
  		omp_map_add_halo_region(&__data_map_infos__[2], 1, 1, 1, OMP_DIST_HALO_EDGING_REFLECTING);
  	}
  	/************************************************************************************************/

	/*********** NOW notifying helper thread to work on this offload ******************/
#if DEBUG_MSG
	printf("=========================================== offloading to %d targets ==========================================\n", __num_target_devices__);
#endif
	/* here we do not need sync start */
	omp_offloading_start(&__copy_data_off_info__, 0);
	//printf("data copied\n");

	while ((k <= mits) && (error > tol)) {
		error = 0.0;
		/* Copy new solution into old */
		omp_offloading_start(&__uuold_exchange_off_info__, 0);

#if defined (STANDALONE_DATA_X)
		/** option 2 halo exchange */
		//printf("----- u <-> uold halo exchange, k: %d, off_info: %X\n", k, &__uuold_exchange_off_info__);
	  	omp_offloading_start(&uuold_halo_x_off_info);
#endif
		//printf("u->uold exchanged: %d\n", k);
		/* jacobi */
		omp_offloading_start(&__jacobi_off_info__, 0);
		int __i__;
		for (__i__ = 0; __i__ < __num_target_devices__;__i__++) {
			error += __reduction_error__[__i__];
		}

		/* Error check */
#if 0
		if ((k % 500) == 0)
		printf("Parallel: Finished %d iteration with error %g\n", k, error);
#endif
		error = (sqrt(error) / (n * m));
		k = (k + 1);
		/*  End iteration loop */
	}
	/* copy back u from each device and free others */
	omp_offloading_start(&__copy_data_off_info__, 1);
	compl_time = read_timer_ms();
	omp_print_map_info(&__data_map_infos__[0]);
	omp_print_map_info(&__data_map_infos__[1]);
	omp_print_map_info(&__data_map_infos__[2]);

#if defined (STANDALONE_DATA_X)
	omp_offloading_fini_info(&uuold_halo_x_off_info);
#endif

#if defined (OMP_BREAKDOWN_TIMING)
	int num_infos = 3;
	omp_offloading_info_report_profile(&__copy_data_off_info__);
	omp_offloading_info_report_profile(&__uuold_exchange_off_info__);
	omp_offloading_info_report_profile(&__jacobi_off_info__);
#if defined (STANDALONE_DATA_X)
	num_infos = 4;
	omp_offloading_info_report_profile(&uuold_halo_x_off_info);
#endif

	omp_offloading_info_t *infos[num_infos];
	infos[0] = &__copy_data_off_info__;
	infos[1] = &__uuold_exchange_off_info__;
	infos[2] = &__jacobi_off_info__;
#if defined (STANDALONE_DATA_X)
	infos[3] = &uuold_halo_x_off_inf;
#endif
	omp_offloading_info_sum_profile(infos, num_infos, start_time, compl_time);
	omp_offloading_info_report_profile(&__copy_data_off_info__);
#endif
	omp_offloading_fini_info(&__uuold_exchange_off_info__);
	omp_offloading_fini_info(&__jacobi_off_info__);
	omp_offloading_fini_info(&__copy_data_off_info__);

	printf("Total Number of Iterations:%d\n", k);
	printf("Residual:%E\n", error);

	/*
	ompacc_time = read_timer_ms() - ompacc_time;
	double cpu_total = ompacc_time;
	*/
}
