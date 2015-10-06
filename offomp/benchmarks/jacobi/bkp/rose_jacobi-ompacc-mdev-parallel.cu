#include <stdio.h>
#include <math.h>
#include <omp.h>
// Add timing support
#include <sys/time.h>
#include "libxomp.h" 
#include "xomp_cuda_lib_inlined.cu" 
#include "homp.h"
/* in second */
#define read_timer() omp_get_wtime()
/* in ms */
#define read_timer_ms() (omp_get_wtime()*1000.0)

double time_stamp() {
	struct timeval t;
	double time;
	gettimeofday(&t, 0);
	time = (t.tv_sec + (1.0e-6 * t.tv_usec));
	return time;
}
double time1;
double time2;
void driver();
void initialize();
void jacobi_v1();
void error_check();
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
#ifndef MSIZE
#warning "MSIZE default to be 512"
#define MSIZE 512
#endif

int n;
int m;
int mits;
#define REAL float // flexible between float and double
float tol;
float relax = 1.0;
float alpha = 0.0543;
float u[MSIZE][MSIZE];
float f[MSIZE][MSIZE];
float uold[MSIZE][MSIZE];
float dx;
float dy;

int main() {
//  float toler;
	/*      printf("Input n,m (< %d) - grid dimension in x,y direction:\n",MSIZE);
	 scanf ("%d",&n);
	 scanf ("%d",&m);
	 printf("Input tol - error tolerance for iterative solver\n");
	 scanf("%f",&toler);
	 tol=(double)toler;
	 printf("Input mits - Maximum iterations for solver\n");
	 scanf("%d",&mits);
	 */
	omp_init_devices();
	n = MSIZE;
	m = MSIZE;
	printf("Input n,m (< %d) - grid dimension in x,y direction.\n", MSIZE);
	tol = 0.0000000001;
	mits = 5000;
#if 0 // Not yet support concurrent CPU and GPU threads  
#ifdef _OPENMP
#endif
#endif  
	driver();
	return 0;
}
/*************************************************************
 * Subroutine driver ()
 * This is where the arrays are allocated and initialzed.
 *
 * Working varaibles/arrays
 *     dx  - grid spacing in x direction
 *     dy  - grid spacing in y direction
 *************************************************************/

void driver() {
	initialize();
	time1 = time_stamp();
	/* Solve Helmholtz equation */
	jacobi_v1();
	time2 = time_stamp();
	printf("------------------------\n");
	printf("Execution time = %f\n", (time2 - time1));
	/* error_check (n,m,alpha,dx,dy,u,f)*/
	error_check();
}
/*      subroutine initialize (n,m,alpha,dx,dy,u,f) 
 ******************************************************
 * Initializes data
 * Assumes exact solution is u(x,y) = (1-x^2)*(1-y^2)
 *
 ******************************************************/

void initialize() {
	int i;
	int j;
	int xx;
	int yy;
//double PI=3.1415926;
	dx = (2.0 / (n - 1));
	dy = (2.0 / (m - 1));
	/* Initialize initial condition and RHS */
#pragma omp parallel for private(xx,yy,j,i)
	for (i = 0; i < n; i++)
		for (j = 0; j < m; j++) {
			xx = ((int) (-1.0 + (dx * (i - 1))));
			yy = ((int) (-1.0 + (dy * (j - 1))));
			u[i][j] = 0.0;
			f[i][j] = (((((-1.0 * alpha) * (1.0 - (xx * xx)))
					* (1.0 - (yy * yy))) - (2.0 * (1.0 - (xx * xx))))
					- (2.0 * (1.0 - (yy * yy))));
		}
}
/*      subroutine jacobi (n,m,dx,dy,alpha,omega,u,f,tol,maxit)
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
 *         maxit  Maximum number of iterations
 *
 * Output : u(n,m) - Solution
 *****************************************************************/
#define LOOP_COLLAPSE 1

#if !LOOP_COLLAPSE
__global__ void OUT__1__10550__(int start_n, int n,int m,float omega,float ax,float ay,float b,float *_dev_per_block_error,float *_dev_u,float *_dev_f,float *_dev_uold)
{
  int _p_j;
  float _p_error;
  _p_error = 0;
  float _p_resid;
  long _dev_lower, _dev_upper;
  XOMP_accelerator_loop_default (start_n, n-1, 1, &_dev_lower, &_dev_upper);
  int _dev_i;
  for (_dev_i = _dev_lower; _dev_i<= _dev_upper; _dev_i ++) {
    for (_p_j = 1; _p_j < (m - 1); _p_j++) {
      _p_resid = (((((ax * (_dev_uold[(_dev_i - 1) * MSIZE + _p_j] + _dev_uold[(_dev_i + 1) * MSIZE + _p_j])) + (ay * (_dev_uold[_dev_i * MSIZE + (_p_j - 1)] + _dev_uold[_dev_i * MSIZE + (_p_j + 1)]))) + (b * _dev_uold[_dev_i * MSIZE + _p_j])) - _dev_f[_dev_i * MSIZE + _p_j]) / b);
      _dev_u[_dev_i * MSIZE + _p_j] = (_dev_uold[_dev_i * MSIZE + _p_j] - (omega * _p_resid));
      _p_error = (_p_error + (_p_resid * _p_resid));
    }
  }
  xomp_inner_block_reduction_float(_p_error,_dev_per_block_error,6);
}

#else
__global__ void OUT__1__10550__(int start_n, int n,int m,float omega,float ax,float ay,float b,float *_dev_per_block_error,float *_dev_u,float *_dev_f,float *_dev_uold)
{
  int _dev_i;
  int ij;
  int _p_j;
  int _dev_lower, _dev_upper;

  float _p_error;
  _p_error = 0;
  float _p_resid;

  // variables for adjusted loop info considering both original chunk size and step(strip)
  int _dev_loop_chunk_size;
  int _dev_loop_sched_index;
  int _dev_loop_stride;

  // 1-D thread block:
  int _dev_thread_num = gridDim.x * blockDim.x;
  int _dev_thread_id = blockDim.x * blockIdx.x + threadIdx.x;

  //TODO: adjust bound to be inclusive later
  int orig_start =start_n;
  int orig_end = (n)*m-1;
  int orig_step = 1;
  int orig_chunk_size = 1;

  XOMP_static_sched_init (orig_start, orig_end, orig_step, orig_chunk_size, _dev_thread_num, _dev_thread_id, \
      & _dev_loop_chunk_size , & _dev_loop_sched_index, & _dev_loop_stride);

  //XOMP_accelerator_loop_default (1, (n-1)*(m-1)-1, 1, &_dev_lower, &_dev_upper);
  while (XOMP_static_sched_next (&_dev_loop_sched_index, orig_end,orig_step, _dev_loop_stride, _dev_loop_chunk_size, _dev_thread_num, _dev_thread_id, & _dev_lower, & _dev_upper))
  {
    for (ij = _dev_lower; ij <= _dev_upper; ij ++) {
      _dev_i = ij/(m-1);
      _p_j = ij%(m-1);

      if (_dev_i>=start_n && _dev_i< (n) && _p_j>=1 && _p_j< (m-1)) // must preserve the original boudary conditions here!!
      {
	_p_resid = (((((ax * (_dev_uold[(_dev_i - 1) * MSIZE + _p_j] + _dev_uold[(_dev_i + 1) * MSIZE + _p_j])) + (ay * (_dev_uold[_dev_i * MSIZE + (_p_j - 1)] + _dev_uold[_dev_i * MSIZE + (_p_j + 1)]))) + (b * _dev_uold[_dev_i * MSIZE + _p_j])) - _dev_f[_dev_i * MSIZE + _p_j]) / b);
	_dev_u[_dev_i * MSIZE + _p_j] = (_dev_uold[_dev_i * MSIZE + _p_j] - (omega * _p_resid));
	_p_error = (_p_error + (_p_resid * _p_resid));
      }
    }
  }

  xomp_inner_block_reduction_float(_p_error,_dev_per_block_error,6);
}

#endif

#if !LOOP_COLLAPSE
__global__ void OUT__2__10550__(int n,int m,float *_dev_u,float *_dev_uold)
{
  int _p_j;
  int _dev_i ;
  long _dev_lower, _dev_upper;
  XOMP_accelerator_loop_default (0, n-1, 1, &_dev_lower, &_dev_upper);
  for (_dev_i = _dev_lower ; _dev_i <= _dev_upper; _dev_i++) {
    for (_p_j = 0; _p_j < m; _p_j++)
      _dev_uold[_dev_i * MSIZE + _p_j] = _dev_u[_dev_i * MSIZE + _p_j];
  }
}

#else
__global__ void OUT__2__10550__(int n,int m,float *_dev_u,float *_dev_uold)
{
  int _p_j;
  int ij;
  int _dev_lower, _dev_upper;

  int _dev_i ;

 // variables for adjusted loop info considering both original chunk size and step(strip)
 int _dev_loop_chunk_size;
 int _dev_loop_sched_index;
 int _dev_loop_stride;

// 1-D thread block:
int _dev_thread_num = gridDim.x * blockDim.x;
int _dev_thread_id = blockDim.x * blockIdx.x + threadIdx.x;

int orig_start =0;
int orig_end = n*m-1; // inclusive upper bound
int orig_step = 1;
int orig_chunk_size = 1;

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
     _dev_uold[_dev_i * MSIZE + _p_j] = _dev_u[_dev_i * MSIZE + _p_j];
   }
  }
 }
#endif

/**
 * array dist according to row halo region
 */
void jacobi_v1() {
	float omega;
	int k;
	float error;
	float ax;
	float ay;
	float b;
	omega = relax;
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
#pragma omp target data device(*) map(to:n, m, omega, ax, ay, b, f{0:n|1}[0:m]>>{:}) map(tofrom:u{0:n}[0:m]>>{:}) map(alloc:uold{0:n|1}[0:m]>>{:})
  while ((k<=mits)&&(error>tol))
  {
    error = 0.0;

    /* Copy new solution into old */
//#pragma omp parallel
//    {
#pragma omp target device(*)//map(in:n, m, u[0:n][0:m]) map(out:uold[0:n][0:m])
#pragma omp parallel for private(j,i) dist_iteration match_range u[]{}
/* #pragma omp parallel for private(j,i) dist_iteration >>(*) */
      for(i=0;i<n;i++)
        for(j=0;j<m;j++)
          uold[i][j] = u[i][j];
#pragma omp halo exchange uold[:]{:}

#pragma omp target device(*)//map(in:n, m, omega, ax, ay, b, f[0:n][0:m], uold[0:n][0:m]) map(out:u[0:n][0:m])
#pragma omp parallel for private(resid,j,i) reduction(+:error) dist_iteration match_range u[]{}// nowait
      for (i=1;i<(n-1);i++)
        for (j=1;j<(m-1);j++)
        {
          resid = (ax*(uold[i-1][j] + uold[i+1][j])\
              + ay*(uold[i][j-1] + uold[i][j+1])+ b * uold[i][j] - f[i][j])/b;

          u[i][j] = uold[i][j] - omega * resid;
          error = error + resid*resid ;
        }

//    }
    /*  omp end parallel */

    /* Error check */

    if (k%500==0)
      printf("Finished %d iteration with error =%f\n",k, error);
    error = sqrt(error)/(n*m);

    k = k + 1;
  }          /*  End iteration loop */

#endif
	double cpu_total = omp_get_wtime()*1000;
	/* there are three mapped array variables (f, u, and uold). all scalar variables will be as parameters */
	int __num_target_devices__ = omp_get_num_active_devices(); /*XXX: = runtime or compiler generated code */

	omp_device_t *__target_devices__[__num_target_devices__];
	/**TODO: compiler generated code or runtime call to init the topology */
	int __top_ndims__ = 1;
	int __top_dims__[__top_ndims__];
	omp_factor(__num_target_devices__, __top_dims__, __top_ndims__);
	int __top_periodic__[__top_ndims__]; __top_periodic__[0] = 0;
	omp_grid_topology_t __topology__={__num_target_devices__, __top_ndims__, __top_dims__, __top_periodic__};
	omp_grid_topology_t *__topp__ = &__topology__;
//	omp_topology_print(__topp__);
	int __num_mapped_variables__ = 3; /* XXX: need compiler output */

	omp_stream_t __dev_stream__[__num_target_devices__]; /* need to change later one for omp_stream_t struct */
	omp_data_map_info_t __data_map_infos__[__num_mapped_variables__];

	omp_data_map_info_t * __info__ = &__data_map_infos__[0];
	omp_data_map_init_info(__info__, __topp__, &f[0][0], sizeof(float), OMP_MAP_TO, n, m, 1);
	__info__->maps = (omp_data_map_t **)alloca(sizeof(omp_data_map_t *) * __num_target_devices__);

	__info__ = &__data_map_infos__[1];
	omp_data_map_init_info(__info__, __topp__, &u[0][0], sizeof(float), OMP_MAP_TOFROM, n, m, 1);
	__info__->maps = (omp_data_map_t **)alloca(sizeof(omp_data_map_t *) * __num_target_devices__);
	omp_map_add_halo_region(__info__, 0, 1, 1, 0, 0);

	__info__ = &__data_map_infos__[2];
	omp_data_map_init_info(__info__, __topp__, (void*)NULL, sizeof(float), OMP_MAP_ALLOC, n, m, 1);
	__info__->maps = (omp_data_map_t **)alloca(sizeof(omp_data_map_t *) * __num_target_devices__);
	/* fill in halo region info here for uold */
	omp_map_add_halo_region(__info__, 0, 1, 1, 0, 0);

	omp_data_map_t __data_maps__[__num_target_devices__][__num_mapped_variables__];

	/* for reduction */
	float *_dev_per_block_error[__num_target_devices__];
	float *_host_per_block_error[__num_target_devices__];
	omp_reduction_float_t *reduction_callback_args[__num_target_devices__];
	double streamCreate_elapsed[__num_target_devices__];

        omp_set_num_threads(__num_target_devices__);
#pragma omp parallel default(shared) 
{
	int __i__ = omp_get_thread_num();
	/**TODO: compiler generated code or runtime call to init the __target_devices__ array */
	__target_devices__[__i__] = &omp_devices[__i__]; /* currently this is simple a copy of the pointer */

#if DEBUG_MSG
	    	printf("=========================================== device %d ==========================================\n", __i__);
#endif
		omp_device_t * __dev__ = __target_devices__[__i__];
		omp_set_current_device(__dev__);
		streamCreate_elapsed[__i__] = read_timer_ms();
		omp_init_stream(__dev__, &__dev_stream__[__i__]);
		streamCreate_elapsed[__i__] = read_timer_ms() - streamCreate_elapsed[__i__];
#pragma omp barrier
		/***************** for each mapped variable has to and tofrom, if it has region mapped to this __ndev_i__ id, we need code here *******************************/
		omp_data_map_t * __dev_map_f__ = &__data_maps__[__i__][0]; /* 0 is given by compiler here */
		omp_data_map_init_map(__dev_map_f__, &__data_map_infos__[0], __i__, __dev__, &__dev_stream__[__i__]);
		omp_data_map_do_even_map(__dev_map_f__, 0, __topp__, 0, __i__);

		omp_data_map_t * __dev_map_u__ = &__data_maps__[__i__][1]; /* 1 is given by compiler here */
		omp_data_map_init_map(__dev_map_u__, &__data_map_infos__[1], __i__, __dev__, &__dev_stream__[__i__]);
		omp_data_map_do_even_map(__dev_map_u__, 0, __topp__, 0, __i__);

		omp_data_map_t * __dev_map_uold__ = &__data_maps__[__i__][2]; /* 2 is given by compiler here */
		omp_data_map_init_map(__dev_map_uold__, &__data_map_infos__[2], __i__, __dev__, &__dev_stream__[__i__]);
		omp_data_map_do_even_map(__dev_map_uold__, 0, __topp__, 0, __i__);
#pragma omp barrier
		/***************** for each mapped variable has to and tofrom, if it has region mapped to this __ndev_i__ id, we need code here *******************************/
		omp_map_buffer_malloc(__dev_map_f__);

		omp_stream_start_event_record(&__dev_stream__[__i__], 0);
		omp_memcpyHostToDeviceAsync(__dev_map_f__);
		omp_stream_stop_event_record(&__dev_stream__[__i__], 0);
		omp_print_data_map(__dev_map_f__);
		/***************************************************************** for u *********************************************************************/
		omp_map_buffer_malloc(__dev_map_u__);

		omp_stream_start_event_record(&__dev_stream__[__i__], 1);
		omp_memcpyHostToDeviceAsync(__dev_map_u__);
		omp_stream_stop_event_record(&__dev_stream__[__i__], 1);
		omp_print_data_map(__dev_map_u__);

		/******************************************** for uold ******************************************************************************/
		omp_map_buffer_malloc(__dev_map_uold__);
		omp_print_data_map(__dev_map_uold__);

		/* for reduction operation */
		long start_n, length_n;
		omp_loop_map_range(__dev_map_u__, 0, -1, -1, &start_n, &length_n);
		int _threads_per_block_ = xomp_get_maxThreadsPerBlock();
		int _num_blocks_ = xomp_get_max1DBlock(length_n*m);
		cudaMalloc(&_dev_per_block_error[__i__], _num_blocks_ * sizeof(float));
		_host_per_block_error[__i__] = (float*)(malloc(_num_blocks_*sizeof(float)));
		reduction_callback_args[__i__] = (omp_reduction_float_t*)malloc(sizeof(omp_reduction_float_t));
#pragma omp barrier
}
	while ((k <= mits)/* && (error > tol)*/) {
		error = 0.0;
#pragma omp parallel default(shared) 
{
	int __i__ = omp_get_thread_num();
			omp_device_t * __dev__ = __target_devices__[__i__];
			omp_set_current_device(__dev__);
		/* Copy new solution into old */
		/* Launch CUDA kernel ... */
			omp_data_map_t * __dev_map_u__ = &__data_maps__[__i__][1];
			omp_data_map_t * __dev_map_uold__ = &__data_maps__[__i__][2]; /* 2 is given by compiler here */
			long start_n, length_n, offset_n;
			omp_loop_map_range(__dev_map_u__, 0, -1, -1, &start_n, &length_n);
			int _threads_per_block_ = xomp_get_maxThreadsPerBlock();
			int _num_blocks_ = xomp_get_max1DBlock(length_n*m);
			omp_stream_start_event_record(&__dev_stream__[__i__], 2);
			OUT__2__10550__<<<_num_blocks_, _threads_per_block_, 0,__dev_stream__[__i__].systream.cudaStream>>>
					(length_n, m,(float*)__dev_map_u__->map_dev_ptr, (float*)__dev_map_uold__->map_dev_ptr);
			omp_stream_stop_event_record(&__dev_stream__[__i__], 2);
		 /* TODO: here we have to make sure that the remote are finish computation before halo exchange */
#pragma omp barrier 
			/* halo exchange here, we do a pull protocol, thus the receiver move data from the source */
			omp_stream_start_event_record(&__dev_stream__[__i__], 3);
			omp_halo_region_pull_async(__dev_map_uold__, 0, 0);
			omp_stream_stop_event_record(&__dev_stream__[__i__], 3);
#pragma omp barrier 
			omp_data_map_t * __dev_map_f__ = &__data_maps__[__i__][0];
			if (__i__== 0) offset_n = omp_loop_map_range(__dev_map_u__, 0, 1, -1, &start_n, &length_n);
			else if (__i__ == __num_target_devices__-1) offset_n = omp_loop_map_range(__dev_map_u__, 0, -1, __dev_map_u__->map_dim[0]-1, &start_n, &length_n);
			else offset_n = omp_loop_map_range(__dev_map_u__, 0, -1, -1, &start_n, &length_n);

			omp_reduction_float_t * args = reduction_callback_args[__i__];
			args->input = _host_per_block_error[__i__];
			args->num = _num_blocks_;
			args->opers = 6;
			//printf("%d device: original offset: %d, mapped_offset: %d, length: %d\n", __i__, offset_n, start_n, length_n);

			/* Launch CUDA kernel ... */
			/** since here we do the same mapping, so will reuse the _threads_per_block and _num_blocks */
			omp_stream_start_event_record(&__dev_stream__[__i__], 4);
			OUT__1__10550__<<<_num_blocks_, _threads_per_block_,(_threads_per_block_ * sizeof(float)),
					__dev_stream__[__i__].systream.cudaStream>>>(start_n, length_n, m,
					omega, ax, ay, b, _dev_per_block_error[__i__],
					(float*)__dev_map_u__->map_dev_ptr, (float*)__dev_map_f__->map_dev_ptr,(float*)__dev_map_uold__->map_dev_ptr);
			/* copy back the results of reduction in blocks */
			cudaMemcpyAsync(_host_per_block_error[__i__], _dev_per_block_error[__i__], sizeof(float)*_num_blocks_, cudaMemcpyDeviceToHost, __dev_stream__[__i__].systream.cudaStream);
			cudaStreamAddCallback(__dev_stream__[__i__].systream.cudaStream, xomp_beyond_block_reduction_float_stream_callback, args, 0);
			omp_stream_stop_event_record(&__dev_stream__[__i__], 4);
			/* xomp_beyond_block_reduction_float(_dev_per_block_error, _num_blocks_, 6); */
			//xomp_freeDevice(_dev_per_block_error);
#pragma omp barrier
#pragma omp critical 
			error += args->result;

			omp_stream_event_elapsed_accumulate_ms(&__dev_stream__[__i__], 2); /* kernel u2uold */
			omp_stream_event_elapsed_accumulate_ms(&__dev_stream__[__i__], 3); /* halo exchange */
			omp_stream_event_elapsed_accumulate_ms(&__dev_stream__[__i__], 4); /* jacobi (including reduction) */
}
		error = (sqrt(error) / (n * m));
		k = (k + 1);
		/*  End iteration loop */
}



		/* Error check */
/*
		if ((k % 500) == 0)
			printf("Finished %d iteration with error =%f\n", k, error);
*/



#pragma omp parallel default(shared) 
{
	int __i__ = omp_get_thread_num();
	/* copy back u from each device and free others */
		omp_device_t * __dev__ = __target_devices__[__i__];
		omp_set_current_device(__dev__);
		omp_data_map_t * __dev_map_u__ = &__data_maps__[__i__][1]; /* 1 is given by compiler here */
		omp_stream_start_event_record(&__dev_stream__[__i__], 5);
                omp_memcpyDeviceToHostAsync(__dev_map_u__);
		omp_stream_stop_event_record(&__dev_stream__[__i__], 5);
		cudaFree(_dev_per_block_error[__i__]);
		omp_reduction_float_t * args = reduction_callback_args[__i__];
		free(args);
		free(_host_per_block_error[__i__]);
}

    omp_sync_cleanup(__num_target_devices__, __num_mapped_variables__, __dev_stream__, &__data_maps__[0][0]);
    cpu_total = omp_get_wtime()*1000 - cpu_total;
	printf("Total Number of Iterations:%d\n", k);
//	printf("Residual:%E\n", error);

    /* for profiling */
	float f_map_to_elapsed[__num_target_devices__]; /* event 0 */
	float u_map_to_elapsed[__num_target_devices__]; /* event 1 */
	float kernel_u2uold_elapsed[__num_target_devices__]; /* event 2 */
	float halo_exchange_elapsed[__num_target_devices__]; /* event 3 */
	float kernel_jacobi_elapsed[__num_target_devices__]; /* event 4, also including the reduction */
	float u_map_from_elapsed[__num_target_devices__]; /* event 5 */

	printf("==============================================================================================================================================================================\n");
	printf("=========================== GPU Results (%d GPUs) for jacobi: u[][](tofrom), f[][](to), uold[][](alloc) size: [%d][%d], time in ms (s/1000) ===============================\n", __num_target_devices__, n, m);
	float f_map_to_accumulated = 0.0;
	float u_map_to_accumulated = 0.0;
	float kernel_u2uold_accumulated = 0.0;
	float halo_exchange_accumulated = 0.0;
	float kernel_jacobi_accumulated = 0.0;
	float u_map_from_accumulated = 0.0;
	float streamCreate_accumulated = 0.0;
        printf("deviceID\t exectime\t stremcreate\t f_u_map_to\t u2old_kernel\t halo_exchange\t kernel_jacobi\t u_map_from\t\n");
	for (int __i__ = 0; __i__ < __num_target_devices__; __i__++) {
		f_map_to_elapsed[__i__] = omp_stream_event_elapsed_ms(&__dev_stream__[__i__], 0);
		u_map_to_elapsed[__i__] = omp_stream_event_elapsed_ms(&__dev_stream__[__i__], 1);
		kernel_u2uold_elapsed[__i__] = __dev_stream__[__i__].elapsed[2]; /* event 2 */
		halo_exchange_elapsed[__i__] = __dev_stream__[__i__].elapsed[3]; /* event 3 */
		kernel_jacobi_elapsed[__i__] = __dev_stream__[__i__].elapsed[4]; /* event 4, also including the reduction */
		u_map_from_elapsed[__i__] = omp_stream_event_elapsed_ms(&__dev_stream__[__i__], 5);
		float total = f_map_to_elapsed[__i__] + u_map_to_elapsed[__i__] + kernel_u2uold_elapsed[__i__] + halo_exchange_elapsed[__i__] + kernel_jacobi_elapsed[__i__]  + u_map_from_elapsed[__i__];
//		printf("device: %d, total: %4f\n", __i__, total);
//		printf("\t\tstreamCreate overhead: %4f\n", streamCreate_elapsed[__i__]);
//		printf("\t\tbreakdown: f map_to: %4f; u map_to: %4f; u2uold kernel: %4f; halo_exchange: %4f; kernel_jacobi: %4f, u map_from: %f\n", f_map_to_elapsed[__i__], u_map_to_elapsed[__i__], kernel_u2uold_elapsed[__i__], halo_exchange_elapsed[__i__], kernel_jacobi_elapsed[__i__], u_map_from_elapsed[__i__]);
//		printf("\t\tbreakdown: f map_to (u and f): %4f; u2uold kernel: %4f; halo_exchange: %4f; kernel_jacobi: %4f, map_from (u): %f\n", f_map_to_elapsed[__i__] + u_map_to_elapsed[__i__], kernel_u2uold_elapsed[__i__], halo_exchange_elapsed[__i__], kernel_jacobi_elapsed[__i__], u_map_from_elapsed[__i__]);
                printf("%d\t %4f\t %4f\t %4f\t %4f\t %4f\t %4f\t %4f\t\n",__i__, total, streamCreate_elapsed[__i__], f_map_to_elapsed[__i__] + u_map_to_elapsed[__i__], kernel_u2uold_elapsed[__i__], halo_exchange_elapsed[__i__], kernel_jacobi_elapsed[__i__], u_map_from_elapsed[__i__]);
		f_map_to_accumulated += f_map_to_elapsed[__i__];
		u_map_to_accumulated += u_map_to_elapsed[__i__];
		kernel_u2uold_accumulated += kernel_u2uold_elapsed[__i__];
		halo_exchange_accumulated += halo_exchange_elapsed[__i__];
		kernel_jacobi_accumulated += kernel_jacobi_elapsed[__i__];
		u_map_from_accumulated += u_map_from_elapsed[__i__];
		streamCreate_accumulated += streamCreate_elapsed[__i__];
	}
	float gpu_total = f_map_to_accumulated + u_map_to_accumulated + kernel_u2uold_accumulated + halo_exchange_accumulated + kernel_jacobi_accumulated + u_map_from_accumulated;
//	printf("ACCUMULATED GPU time (%d GPUs): %4f\n", __num_target_devices__ , gpu_total);
//	printf("\t\tstreamCreate overhead: %4f\n",streamCreate_accumulated);
//	printf("\t\tbreakdown: f map_to: %4f; u map_to: %4f; u2uold kernel: %4f; halo_exchange: %4f; kernel_jacobi: %4f, u map_from: %f\n", f_map_to_accumulated , u_map_to_accumulated , kernel_u2uold_accumulated , halo_exchange_accumulated , kernel_jacobi_accumulated , u_map_from_accumulated);
//	printf("\t\tbreakdown: f map_to (u and f): %4f; u2uold kernel: %4f; halo_exchange: %4f; kernel_jacobi: %4f, map_from (u): %f\n", f_map_to_accumulated + u_map_to_accumulated, kernel_u2uold_accumulated , halo_exchange_accumulated , kernel_jacobi_accumulated , u_map_from_accumulated);
        printf("Total:\t exectime\t stremcreate\t f_u_map_to\t u2old_kernel\t halo_exchange\t kernel_jacobi\t u_map_from\t\n");
        printf("\t %4f\t %4f\t %4f\t %4f\t %4f\t %4f\t %4f\t\n", gpu_total, streamCreate_accumulated,f_map_to_accumulated + u_map_to_accumulated, kernel_u2uold_accumulated , halo_exchange_accumulated , kernel_jacobi_accumulated , u_map_from_accumulated); 

//	printf("AVERAGE GPU time (per GPU): %4f\n", gpu_total/__num_target_devices__);
//	printf("\t\tbreakdown: f map_to: %4f; u map_to: %4f; u2uold kernel: %4f; halo_exchange: %4f; kernel_jacobi: %4f, u map_from: %f\n", f_map_to_accumulated/__num_target_devices__, u_map_to_accumulated/__num_target_devices__, kernel_u2uold_accumulated/__num_target_devices__, halo_exchange_accumulated/__num_target_devices__, kernel_jacobi_accumulated/__num_target_devices__, u_map_from_accumulated);
//	printf("\t\tbreakdown: f map_to (u and f): %4f; u2uold kernel: %4f; halo_exchange: %4f; kernel_jacobi: %4f, map_from (u): %f\n", f_map_to_accumulated/__num_target_devices__ + u_map_to_accumulated/__num_target_devices__, kernel_u2uold_accumulated/__num_target_devices__, halo_exchange_accumulated/__num_target_devices__, kernel_jacobi_accumulated/__num_target_devices__, u_map_from_accumulated);
        printf("AVG:\t exectime\t stremcreate\t f_u_map_to\t u2old_kernel\t halo_exchange\t kernel_jacobi\t u_map_from\t\n");
        printf("\t %4f\t %4f\t %4f\t %4f\t %4f\t %4f\t\n", gpu_total/__num_target_devices__, f_map_to_accumulated/__num_target_devices__ + u_map_to_accumulated/__num_target_devices__, kernel_u2uold_accumulated/__num_target_devices__, halo_exchange_accumulated/__num_target_devices__, kernel_jacobi_accumulated/__num_target_devices__, u_map_from_accumulated);

	printf("----------------------------------------------------------------\n");
	printf("Total time measured from CPU:\t %4f\n", cpu_total);
	printf("Total time measured without streamCreate:\t %4f\n", (cpu_total-streamCreate_accumulated));
	printf("AVERAGE total (CPU cost+GPU) per GPU:\t %4f\n", cpu_total/__num_target_devices__);
	printf("Total CPU cost:\t %4f\n", cpu_total - gpu_total);
	printf("AVERAGE CPU cost per GPU:\t %4f\n", (cpu_total-gpu_total)/__num_target_devices__);
	printf("==========================================================================================================================================\n");

}

#if 0
void jacobi_v3() {
	float omega;
	int i;
	int j;
	int k;
	float error;
	float resid;
	float ax;
	float ay;
	float b;
	omega = relax;
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
	/*
	 #pragma omp target data device(*)=>(*)(*) map(to:n, m, omega, ax, ay, b, f[0:n][0:m]>>(:)(:)) map(tofrom:u[0:n][0:m]>>(:)(:)) map(alloc:uold[0:n|1][0:m]>>(:)(:))
	 */
	/* there are three mapped array variables (f, u, and uold). all scalar variables will be as parameters */
	int __num_target_devices__ = 4; /*XXX: = runtime call or compiler generated number */
	omp_device_t *__target_devices__[__num_target_devices__];
	/**TODO: compiler generated code or runtime call to init the __target_devices__ array */
	int __i__;
	for (__i__ = 0; __i__ < __num_target_devices__; __i__++) {
		__target_devices__[__i__] = &omp_devices[__i__]; /* currently this is simple a copy of the pointer */
	}
	/**TODO: compiler generated code or runtime call to init the topology */
	int __top_ndims__ = 2;
	int __top_dims__[__top_ndims__];
	omp_factor(__num_target_devices__, __top_dims__, __top_ndims__);
	int __top_periodic__[__top_ndims__]; __top_periodic__[0] = 0;__top_periodic__[1] = 0; /* this is not used at all */
	omp_grid_topology_t __topology__={__num_target_devices__, __top_ndims__, __top_dims__, __top_periodic__};
	omp_grid_topology_t *__topp__ = &__topology__;

	int __num_mapped_variables__ = 3; /* XXX: need compiler output */

	omp_stream_t __dev_stream__[__num_target_devices__]; /* need to change later one for omp_stream_t struct */
	omp_data_map_info_t __data_map_infos__[__num_mapped_variables__];

	omp_data_map_info_t * __info__ = &__data_map_infos__[0];
	omp_data_map_init_info(__info__, __topp__, &f[0][0], sizeof(float), OMP_MAP_TO, n, m, 1);
	__info__->maps = alloca(sizeof(omp_data_map_t *) * __num_target_devices__);

	omp_data_map_info_t * __info__ = &__data_map_infos__[1];
	omp_data_map_init_info(__info__, __topp__, &u[0][0], sizeof(float), OMP_MAP_TOFROM, n, m, 1);
	__info__->maps = alloca(sizeof(omp_data_map_t *) * __num_target_devices__);

	omp_data_map_info_t * __info__ = &__data_map_infos__[2];
	omp_data_map_init_info(__info__, __topp__, &uold[0][0], sizeof(float), OMP_MAP_ALLOC, n, m, 1);
	__info__->maps = alloca(sizeof(omp_data_map_t *) * __num_target_devices__);
	/* fill in halo region info here for uold */
	omp_map_add_halo_region(__info__, 0, 1, 1, 0);
	omp_map_add_halo_region(__info__, 1, 1, 1, 0);

	omp_data_map_t __data_maps__[__num_target_devices__][__num_mapped_variables__];
	for (__i__ = 0; __i__ < __num_target_devices__; __i__++) {
		omp_device_t * __dev__ = __target_devices__[__i__];
		omp_set_current_device(__dev__);
		omp_init_stream(__dev__, &__dev_stream__[__i__]);

		/***************** for each mapped variable has to and tofrom, if it has region mapped to this __ndev_i__ id, we need code here *******************************/
		omp_data_map_t * __dev_map_f__ = &__data_maps__[__i__][0]; /* 0 is given by compiler here */
		omp_data_map_init_map(__dev_map_f__, &__data_map_infos__[0], __i__, __dev__, &__dev_stream__[__i__]);
		omp_data_map_do_even_map(__dev_map_f__, 0, __topp__, 0, __i__);
		omp_data_map_do_even_map(__dev_map_f__, 1, __topp__, 1, __i__);

		omp_map_buffer(__dev_map_f__, 1); /* even a 2-d array, but since we are doing row-major partition, no need to marshalled data */

		omp_memcpyHostToDeviceAsync(__dev_map_f__);
		omp_print_data_map(__dev_map_f__);
		/*************************************************************************************************************************************************************/

		/***************************************************************** for u *********************************************************************/
		omp_data_map_t * __dev_map_u__ = &__data_maps__[__i__][1]; /* 1 is given by compiler here */
		omp_data_map_init_map(__dev_map_u__, &__data_map_infos__[1], __i__, __dev__, &__dev_stream__[__i__]);

		omp_data_map_do_even_map(__dev_map_u__, 0, __topp__, 0, __i__);
		omp_data_map_do_even_map(__dev_map_u__, 1, __topp__, 1, __i__);

		omp_map_buffer(__dev_map_u__, 1); /* column major, marshalling needed */

		omp_memcpyHostToDeviceAsync(__dev_map_u__);
		omp_print_data_map(__dev_map_u__);

		/******************************************** for uold ******************************************************************************/

		omp_data_map_t * __dev_map_uold__ = &__data_maps__[__i__][2]; /* 2 is given by compiler here */
		omp_data_map_init_map(__dev_map_uold__, &__data_map_infos__[2], __i__, __dev__, &__dev_stream__[__i__]);

		omp_data_map_do_even_map(__dev_map_uold__, 0, __topp__, 0, __i__);
		omp_data_map_do_even_map(__dev_map_uold__, 1, __topp__, 1, __i__);

		omp_map_buffer(__dev_map_uold__, 0);

		omp_print_data_map(__dev_map_uold__);
	}

	while ((k <= mits) && (error > tol)) {
		error = 0.0;
		/* Copy new solution into old */
		/* Launch CUDA kernel ... */
		for (__i__ = 0; __i__ < __num_target_devices__;__i__++) {
			omp_device_t * __dev__ = __target_devices__[__i__];
			omp_set_current_device(__dev__);
			omp_data_map_t * __dev_map_f__ = &__data_maps__[__i__][0];
			omp_data_map_t * __dev_map_u__ = &__data_maps__[__i__][1];
			omp_data_map_t * __dev_map_uold__ = &__data_maps__[__i__][2]; /* 2 is given by compiler here */
			int _threads_per_block_ = xomp_get_maxThreadsPerBlock();
			int _num_blocks_ = xomp_get_max1DBlock(n / __num_target_devices__ - 1 - 0 + 1);
			OUT__2__10550__<<<_num_blocks_, _threads_per_block_, 0,
					__dev_stream__[__i__]>>>(n / __num_target_devices__, m,
					__dev_map_u__->map_dev_ptr, __dev_map_uold__->map_dev_ptr);

			/* halo exchange here, we do a pull protocol, thus the receiver move data from the source */
			omp_halo_region_pull_async(__i__, NULL, __dev_map_uold__,NULL);

			/* Launch CUDA kernel ... */
//			_threads_per_block_ = xomp_get_maxThreadsPerBlock();
			_num_blocks_ = xomp_get_max1DBlock((n / __num_target_devices__ - 1) - 1 - 1 + 1);
			float *_dev_per_block_error = (float *) (xomp_deviceMalloc(	_num_blocks_ * sizeof(float)));
			OUT__1__10550__<<<_num_blocks_, _threads_per_block_,(_threads_per_block_ * sizeof(float)),
					__dev_stream__[__i__]>>>(n / __num_target_devices__, m,
					omega, ax, ay, b, _dev_per_block_error,
					__dev_map_u__->map_dev_ptr, __dev_map_f__->map_dev_ptr,__dev_map_uold__->map_dev_ptr);
			/* copy back the results of reduction in blocks */
			float * _host_per_block_error = (float*)(malloc(_num_blocks_*sizeof(float)));
			cudaMemcpyAsync(_host_per_block_error, _dev_per_block_error, sizeof(float)*_num_blocks_, __dev_stream__[__i__]);
			omp_reduction_t beyond_block_reduction = {_host_per_block_error, _num_blocks_, sizeof(float), 6};
			cudaStreamAddCallback (__dev_stream__[__i__], xomp_beyond_block_reduction_float, &beyond_block_reduction, 0);
			/* error = xomp_beyond_block_reduction_float(_dev_per_block_error, _num_blocks_, 6); */
			//xomp_freeDevice(_dev_per_block_error);
		}
		/* here we sync the stream and make sure all are complete (including the per-device reduction)
		 */
		omp_sync_stream(__num_target_devices__, __dev_stream__, 0);
		/* then, we need the reduction from multi-devices */

		/* Error check */
		if ((k % 500) == 0)
			printf("Finished %d iteration with error =%f\n", k, error);
		error = (sqrt(error) / (n * m));
		k = (k + 1);
		/*  End iteration loop */
	}
	xomp_memcpyDeviceToHost(((void *) u), ((const void *) _dev_u), _dev_u_size);
	xomp_freeDevice (_dev_u);
	xomp_freeDevice (_dev_f);
	xomp_freeDevice (_dev_uold);
	printf("Total Number of Iterations:%d\n", k);
	printf("Residual:%E\n", error);
}
#endif

/*      subroutine error_check (n,m,alpha,dx,dy,u,f)
 implicit none
 ************************************************************
 * Checks error between numerical and exact solution
 *
 ************************************************************/

void error_check() {
	int i;
	int j;
	float xx;
	float yy;
	float temp;
	float error;
	dx = (2.0 / (n - 1));
	dy = (2.0 / (m - 1));
	error = 0.0;
//#pragma omp parallel for private(xx,yy,temp,j,i) reduction(+:error)
	for (i = 0; i < n; i++)
		for (j = 0; j < m; j++) {
			xx = (-1.0 + (dx * (i - 1)));
			yy = (-1.0 + (dy * (j - 1)));
			temp = (u[i][j] - ((1.0 - (xx * xx)) * (1.0 - (yy * yy))));
			error = (error + (temp * temp));
		}
	error = (sqrt(error) / (n * m));
//	printf("Solution Error :%E \n", error);
}
