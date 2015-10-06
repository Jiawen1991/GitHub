#include "axpy.h"
/* standard one-dev support */
#include "libxomp.h" 
#include "xomp_cuda_lib_inlined.cu"
#include "homp.h"

#if 0
void axpy_mdev_v2(REAL* x, REAL* y,  long n, REAL a) {

#pragma omp target device (:) map(tofrom: y[0:n]>>(:)) map(to: x[0:n]>>(:),a,n)
#pragma omp parallel for shared(x, y, n, a) private(i) dist_iteration match_range x[:]
/* in this example, the y[0:n], and x[0:n] will be evenly distributed among the ndev devices, scalars such as a and n will each have a mapped copy in all the devices */
  for (i = 0; i < n; ++i)
    y[i] += a * x[i];
}

/* NOTE: the compiler needs to do the analysis for multiple pragma(s) and loop nest. The x[:] in the mapped_range x[:] should
 * be in the previous pragma's map clause
 *
 * Thus this requires the runtime to keep track of the mapped variables and all the details. In some examples, those information could
 * be provided by code-generation of compiler. but in other cases, e.g. the orphaned directive, one need to retrieve from the runtime
 * to get those information. Thus in current implementation, we simply keep all the information in the runtime, regardless of using it
 * or not.
 */
#endif

#if defined (DEVICE_NVGPU_SUPPORT)
__global__ void OUT__3__5904__( long start_n,  long len_n,REAL a,REAL *_dev_x,REAL *_dev_y)
{
   long _dev_i = blockDim.x * blockIdx.x + threadIdx.x;
  if (_dev_i >= start_n && _dev_i <= start_n + len_n  - 1) {
    _dev_y[_dev_i] += (a * _dev_x[_dev_i]);
  }
}
#endif

struct OUT__3__5904__other_args {
	REAL a;
	long n;
};

/* called by the helper thread */
void OUT__3__5904__launcher (omp_offloading_t * off, void *args) {
    struct OUT__3__5904__other_args * iargs = (struct OUT__3__5904__other_args*) args; 
    long start_n, length_n;
    REAL a = iargs->a;
    REAL n = iargs->n;
    omp_offloading_info_t * off_info = off->off_info;
//    printf("off: %X, off_info: %X, devseqid: %d\n", off, off_info, off->devseqid);
    omp_data_map_t * map_x = &off_info->data_map_info[0].maps[off->devseqid]; /* 0 means the map X */
    omp_data_map_t * map_y = &off_info->data_map_info[1].maps[off->devseqid]; /* 1 means the map Y */
    REAL * x = (REAL *)map_x->map_dev_ptr;
    REAL * y = (REAL *)map_y->map_dev_ptr;
    
    omp_loop_map_range(map_x, 0, -1, -1, &start_n, &length_n);
//    printf("devseqid: %d, start_n: %d, length_n: %d, x: %X, y: %X\n", off->devseqid, start_n, length_n, x, y);
    
	omp_device_type_t devtype = off_info->targets[off->devseqid]->type;
#if defined (DEVICE_NVGPU_SUPPORT)
	if (devtype == OMP_DEVICE_NVGPU) {
        /* Launch CUDA kernel ... */
        /* the argu for this function should be the original pointer (x in this example) and the runtime should search and retrieve the
         * device map object
         */
        int _threads_per_block_ = xomp_get_maxThreadsPerBlock();
        int _num_blocks_ = xomp_get_max1DBlock(length_n);
//        printf("device: %d, range: %d:%d\n", __i__, start_n, length_n);

        OUT__3__5904__<<<_num_blocks_,_threads_per_block_, 0, off->stream.systream.cudaStream>>>(start_n, length_n,a,x,y);
	} else
#endif
	if (devtype == OMP_DEVICE_THSIM) {
		int i;
		for (i=start_n; i<start_n + length_n; i++) {
			y[i] += a*x[i];
	//		printf("x[%d]: %f, y[%d]: %f\n", i, x[i], i, y[i]);
		}
	} else {
		fprintf(stderr, "device type is not supported for this call\n");
	}
}

REAL axpy_ompacc_mdev_v2(REAL *x, REAL *y,  long n,REAL a)
{
	double ompacc_time = read_timer_ms(); //read_timer_ms();
	
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
	int __top_ndims__ = 1;
	int __top_dims__[__top_ndims__];
	int __top_periodic__[__top_ndims__]; 
	int __id_map__[__num_target_devices__];
	omp_grid_topology_init_simple (&__top__, __target_devices__, __num_target_devices__, __top_ndims__, __top_dims__, __top_periodic__, __id_map__);

	int __num_mapped_variables__ = 2; /* XXX: need compiler output */

	omp_data_map_info_t __data_map_infos__[__num_mapped_variables__];
		
	omp_data_map_info_t * __info__ = &__data_map_infos__[0];
	long x_dims[1]; x_dims[0] = n;
	omp_data_map_dist_t x_dist[1];
	omp_data_map_init_info_dist_straight(__info__, &__top__, x, 1, x_dims, sizeof(REAL), OMP_DATA_MAP_TO, x_dist, OMP_DATA_MAP_DIST_EVEN);
	__info__->maps = (omp_data_map_t *)alloca(sizeof(omp_data_map_t) * __num_target_devices__);

	__info__ = &__data_map_infos__[1];
	long y_dims[1]; y_dims[0] = n;
	omp_data_map_dist_t y_dist[1];
	omp_data_map_init_info_dist_straight(__info__, &__top__, y, 1, y_dims, sizeof(REAL), OMP_DATA_MAP_TOFROM, y_dist, OMP_DATA_MAP_DIST_EVEN);
	__info__->maps = (omp_data_map_t *)alloca(sizeof(omp_data_map_t) * __num_target_devices__);
	
	struct OUT__3__5904__other_args args;
	args.a = a;
	args.n = n;
	omp_offloading_info_t __offloading_info__;
	__offloading_info__.offloadings = (omp_offloading_t *) alloca(sizeof(omp_offloading_t) * __num_target_devices__);
	/* we use universal args and launcher because axpy can do it */
	omp_offloading_init_info (&__offloading_info__, &__top__, __target_devices__, __num_mapped_variables__, __data_map_infos__, OUT__3__5904__launcher, &args);

#if 0
	/* we could specify dev-specific args and kernel_launcher */
	struct OUT__3__5904__other_args args[__num_target_devices__];
	for (__i__ = 0; __i__ < __num_target_devices__; __i__++) {
		args[i].a = a;
		args[i].n = n;
		__offloading_info__.offloadings[i].args = &args[i];
		__offloading_info__.offloadings[i].kernel_launcher = OUT__3__5904__launcher;
	}
#endif
	
	/*********** NOW notifying helper thread to work on this offload ******************/
#if DEBUG_MSG
	 printf("=========================================== offloading to %d targets ==========================================\n", __num_target_devices__);
#endif
	/* here we do not need sync start */
	omp_offloading_notify_and_wait_completion(__target_devices__, __num_target_devices__, &__offloading_info__);
	ompacc_time = read_timer_ms() - ompacc_time;
	double cpu_total = ompacc_time;

#if 0
	float x_map_to_elapsed[__num_target_devices__];
	float y_map_to_elapsed[__num_target_devices__];
	float kernel_elapsed[__num_target_devices__];
	float y_map_from_elapsed[__num_target_devices__];
	printf("=============================================================================================================================================\n");
	printf("=========================== GPU Results (%d GPUs) for y[] = a*x[] + y[], x|y size: %d, time in ms (s/1000) ===============================\n", __num_target_devices__, n);
	float x_map_to_accumulated = 0.0;
	float y_map_to_accumulated = 0.0;
	float kernel_accumulated = 0.0;
	float y_map_from_accumulated = 0.0;
	float streamCreate_accumulated = 0.0;
	for (__i__ = 0; __i__ < __num_target_devices__; __i__++) {
		x_map_to_elapsed[__i__] = omp_stream_event_elapsed_ms(&__dev_stream__[__i__], 0);
		y_map_to_elapsed[__i__] = omp_stream_event_elapsed_ms(&__dev_stream__[__i__], 1);
		kernel_elapsed[__i__] = omp_stream_event_elapsed_ms(&__dev_stream__[__i__], 2);
		y_map_from_elapsed[__i__] = omp_stream_event_elapsed_ms(&__dev_stream__[__i__], 3);
		float total = x_map_to_elapsed[__i__] + y_map_to_elapsed[__i__] + kernel_elapsed[__i__] + y_map_from_elapsed[__i__];
		printf("device: %d, total: %4f\n", __i__, total);
		printf("\t\tbreakdown: x map_to: %4f; y map_to: %4f; kernel: %4f; y map_from: %f\n", x_map_to_elapsed[__i__], y_map_to_elapsed[__i__], kernel_elapsed[__i__], y_map_from_elapsed[__i__]);
		printf("\t\tbreakdown: map_to (x and y): %4f; kernel: %4f; map_from (y): %f\n", x_map_to_elapsed[__i__] + y_map_to_elapsed[__i__], kernel_elapsed[__i__], y_map_from_elapsed[__i__]);
		x_map_to_accumulated += x_map_to_elapsed[__i__];
		y_map_to_accumulated += y_map_to_elapsed[__i__];
		kernel_accumulated += kernel_elapsed[__i__];
		y_map_from_accumulated += y_map_from_elapsed[__i__];
		//streamCreate_accumulated += streamCreate_elapsed[__i__];
	}
	float total = x_map_to_accumulated + y_map_to_accumulated + kernel_accumulated + y_map_from_accumulated;
	printf("ACCUMULATED GPU time (%d GPUs): %4f\n", __num_target_devices__ , total);
	printf("\t\tstreamCreate overhead: %4f\n",streamCreate_accumulated);
	printf("\t\tbreakdown: x map_to: %4f, y map_to: %4f, kernel: %4f, y map_from %f\n", x_map_to_accumulated, y_map_to_accumulated, kernel_accumulated, y_map_from_accumulated);
	printf("\t\tbreakdown: map_to(x and y): %4f, kernel: %4f, map_from (y): %f\n", x_map_to_accumulated + y_map_to_accumulated, kernel_accumulated, y_map_from_accumulated);
	printf("AVERAGE GPU time (per GPU): %4f\n", total/__num_target_devices__);
	printf("\t\tbreakdown: x map_to: %4f, y map_to: %4f, kernel: %4f, y map_from %f\n", x_map_to_accumulated/__num_target_devices__, y_map_to_accumulated/__num_target_devices__, kernel_accumulated/__num_target_devices__, y_map_from_accumulated/__num_target_devices__);
	printf("\t\tbreakdown: map_to (x and y): %4f, kernel: %4f, map_from (y): %f\n", x_map_to_accumulated/__num_target_devices__ + y_map_to_accumulated/__num_target_devices__, kernel_accumulated/__num_target_devices__, y_map_from_accumulated/__num_target_devices__);

	printf("----------------------------------------------------------------\n");
	printf("Total time measured from CPU: %4f\n", cpu_total);
	printf("Total time measured without streamCreate %4f\n", (cpu_total-streamCreate_accumulated));
	printf("AVERAGE total (CPU cost+GPU) per GPU: %4f\n", cpu_total/__num_target_devices__);
	printf("Total CPU cost: %4f\n", cpu_total - total/__num_target_devices__);
	printf("AVERAGE CPU cost per GPU: %4f\n", (cpu_total-total/__num_target_devices__)/__num_target_devices__);
	printf("==========================================================================================================================================\n");
#endif
	return cpu_total;
}
