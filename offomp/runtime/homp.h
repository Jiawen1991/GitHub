/*
 * homp.h
 *
 *  Created on: Sep 16, 2013
 *      Author: yy8
 */

#ifndef __HOMP_H__
#define __HOMP_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#if defined (DEVICE_NVGPU_SUPPORT)
#define CUDA_API_PER_THREAD_DEFAULT_STREAM 1
#include <cuda.h>
#include <cuda_runtime.h>
#endif

#define OMP_BREAKDOWN_TIMING 1
/**************************** OpenMP 4.0 standard support *******************************/
extern int default_device_var; /* -1 means no device, the runtime should be initialized this to be 0 if there is at least one device */
/* the OMP_DEFAULT_DEVICE env variable is also defined in 4.0, which set the default-device-var icv */
extern void omp_set_default_device(int device_num );
extern int omp_get_default_device(void);
extern int omp_get_num_devices();

typedef struct omp_device omp_device_t;
typedef struct omp_data_map omp_data_map_t;
typedef struct omp_data_map_info omp_data_map_info_t;
typedef struct omp_offloading_info omp_offloading_info_t;
typedef struct omp_offloading omp_offloading_t;

/* the max number of dimensions runtime support now for array and cart topology */
#define OMP_MAX_NUM_DIMENSIONS 3
#define OMP_ALL_DIMENSIONS -1
/* the LONG_MIN */
#define OMP_ALIGNEE_START -2147483647

/**
 * multiple device support
 * the following should be a list of name agreed with vendors

 * NOTES: should we also have a device version number, e.g. a type of device
 * may have mulitple generations, thus versions.
 */
typedef enum omp_device_type {
	OMP_DEVICE_HOSTCPU, /* the host cpu */
	OMP_DEVICE_NVGPU, /* NVIDIA GPGPUs */
	OMP_DEVICE_ITLMIC, /* Intel MIC */
	OMP_DEVICE_TIDSP, /* TI DSP */
	OMP_DEVICE_AMDAPU, /* AMD APUs */
	OMP_DEVICE_REMOTE, /* a remote node */
	OMP_DEVICE_THSIM, /* a new thread of the same process, e.g. pthread */
	OMP_DEVICE_LOCALPS, /* a new process in the same node, e.g. a new process created using fork) */
	OMP_NUM_DEVICE_TYPES,  /* the total number of types of supported devices */
} omp_device_type_t;

extern char * omp_device_type_name[];
typedef struct omp_device_type_info {
	omp_device_type_t type;
	char name[32];
	char shortname[8];
	int num_devs;
} omp_device_type_info_t;
extern omp_device_type_info_t omp_device_types[];

/**
 * each stream also provide a limited number of event objects for collecting timing
 * information.
 */
typedef struct omp_dev_stream {
	omp_device_t * dev;
	union {
#if defined (DEVICE_NVGPU_SUPPORT)
		cudaStream_t cudaStream;
#endif
		void * myStream;
	} systream;
} omp_dev_stream_t;

typedef enum omp_device_mem_type { /* the mem type related to the host mem */
	OMP_DEVICE_MEM_SHARED_CC_UMA, /* CC UMA */
	OMP_DEVICE_MEM_SHARED_CC_NUMA, /* CC NUMA */
	OMP_DEVICE_MEM_SHARED_NCC_UMA,
	OMP_DEVICE_MEM_SHARED_NCC_NUMA,
	OMP_DEVICE_MEM_SHARED,
	OMP_DEVICE_MEM_VIRTUAL_AS, /* uniformed virtual address space */
	OMP_DEVICE_MEM_DISCRETE, /* different memory and different memory space */
}omp_device_mem_type_t;

#define omp_device_mem_shared(mem_type) (mem_type <= OMP_DEVICE_MEM_SHARED)
#define omp_device_mem_vas(mem_type) (mem_type == OMP_DEVICE_MEM_VIRTUAL_AS)
#define omp_device_mem_discrete(mem_type) (mem_type == OMP_DEVICE_MEM_DISCRETE)

/**
 ********************* Runtime notes ***********************************************
 * runtime may want to have internal array to supports the programming APIs for multiple devices, e.g.
 *
 * We have a pthread managing a accelerator device, i.e. responsible of set up the GPU,
 * launching calls for memory allocation, kernel launching as well as timing.
 *
 * The helper thread use the offload_queue to keep track of a list of
 * offloading request to this device, see struct_offloading_dev_t struct definition
 */
struct omp_device {
	int id; /* the id from omp view */
	long sysid; /* the handle from the system view, e.g.
			 device id for NVGPU cudaSetDevice(sysid), 
			 or pthread_t for THSIM. Need type casting to become device-specific id */
	char name[64]; /* a short name for the sake of things */
	omp_device_type_t type;
	void * dev_properties; /* a pointer to the device-specific properties object */
	omp_device_mem_type_t mem_type; /* the mem access pattern relative to the host, e.g. shared or discrete */

	/* performance factor */
	unsigned long mem_size;
	unsigned long max_teams;
	unsigned long max_threads;

	unsigned long num_cores;
	unsigned long num_chips; /* SM for GPU, core pack for host */
	unsigned long core_frequency;
	unsigned int arch_factor; /* means to capture micro arch factor that impact performance, e.g. superscalar, deeper pipeline, etc */

	double bandwidth; /* between host memory and dev memory for profile data movement cost MB/s */
	double latency; /* us (10e-6 seconds) */

	double total_real_flopss; /* the sustained flops/s after testing, GFLOPs/s */
	double flopss_percore; /* per core performance GFLOPs/s */

	int status;
	volatile omp_offloading_info_t * offload_request; /* this is the notification flag that the helper thread will pick up the offloading request */

	omp_offloading_t * offload_stack[4];
	/* the stack for keeping the nested but unfinished offloading request, we actually only need 2 so far.
	 * However, if we know the current offload (being processed) is one that the device will run to completion, we will not put into the stack
	 * for the purpose of saving the cost of push/pop. Thus the stack only keep those pending offload (e.g. inside a "target data" offload, we have
	 * a "target" offload, the "target data" offload will be pushed to the stack. The purpose of this stack is to help data mapping inheritance, i.e.
	 * reuse the data map created in the upper-level enclosing offloading operations (typically target data).
	 */
	int offload_stack_top;

	omp_dev_stream_t devstream; /* per dev stream */

	omp_data_map_t ** resident_data_maps; /* a link-list or an array for resident data maps (data maps cross multiple offloading region */

	pthread_t helperth;
};

/**
 * we organize record and timing as a sequence of event, recording could be done by host side or by device-specific approach
 */
typedef enum omp_event_record_method {
	OMP_EVENT_DEV_NONE = 0,
	OMP_EVENT_DEV_RECORD,
	OMP_EVENT_HOST_RECORD,
	OMP_EVENT_HOST_DEV_RECORD,
} omp_event_record_method_t;

#define OMP_EVENT_MSG_LENGTH 96
#define OMP_EVENT_NAME_LENGTH 12

/*omp_event is for the record and timing caclulation*/
typedef struct omp_event {
	omp_device_t * dev;
	omp_dev_stream_t * stream;
	omp_event_record_method_t record_method;
	const char *event_name;
	char event_description[OMP_EVENT_MSG_LENGTH];
	int count; /* a counter for accumulating recurring event */
	int recorded; /* everytime stop_record is called, this flag is set, and when a elapsed is calculated, this flag is reset */


#if defined (DEVICE_NVGPU_SUPPORT)
	cudaEvent_t start_event_dev;
	cudaEvent_t stop_event_dev;
#endif
	double start_time_dev;
	double stop_time_dev;
	double start_time_host;
	double stop_time_host;
	double elapsed_dev;
	double elapsed_host;
} omp_event_t;

/* tracing
 * The event tracing are performed by each helper thread that writes traces to a file.
 * The master thread (who starts the offloading) will write a master trace file.
 * Traces are stored in multiple files (1 index + #targets trace files), the trace_name is:
 * <offloading_name>_<uid>_<recur_id>
 *
 * The master index file is named as <trace_name>.index
 * Each trace file is named as <trace_name>_vdevid_pdevid.txt
 *
 * The content of master index:
 * trace_name
 * vdevid pdevid (one per line)
 * ...
 * starter time_stamp
 * end time_stamp
 *
 * Trace file content:
 * event_name start_timestamp stop_timestamp
 * ....
 *
 */
//#define OMP_BREAKDOWN_TIMING 1
#if defined (OMP_BREAKDOWN_TIMING)

extern int total_event_index;       		/* host event */
extern int timing_init_event_index; 		/* host event */
extern int map_init_event_index;  			/* host event */

extern int acc_mapto_event_index; 			/* dev event */
extern int acc_kernel_exe_event_index;		/* dev event */
extern int acc_ex_pre_barrier_event_index; 	/* host event */
extern int acc_ex_event_index;  			/* host event for data exchange such as halo xchange */
extern int acc_ex_post_barrier_event_index;	/* host event */
extern int acc_mapfrom_event_index;			/* dev event */

extern int sync_cleanup_event_index;		/* host event */
extern int barrier_wait_event_index;		/* host event */

extern int misc_event_index_start;      	/* other events, e.g. mapto/from for each array, start with 9*/
extern void omp_offloading_info_sum_profile(omp_offloading_info_t ** infos, int count, double start_time, double compl_time);

#endif
/**
 ********************** Compiler notes *********************************************
 * The recommended compiler flag name to output the
 * list of supported device is "-omp-device-types". The use of "-omp-device-types"
 * could be also as follows to restrict the compiler to only support
 * compilation for the listed device types: -omp-device-types="TYPE1,TYPE2,...,TYPE3"
 */

/* APIs to support multiple devices: */
extern char * omp_supported_device_types(); /* return a list of devices supported by the compiler in the format of TYPE1:TYPE2 */
extern omp_device_type_t omp_get_device_type(int devid);
extern char * omp_get_device_type_as_string(int devid);
extern int omp_get_num_devices_of_type(omp_device_type_t type); /* current omp has omp_get_num_devices(); */
extern int omp_get_devices(omp_device_type_t type, int *devnum_array, int ndev); /* return a list of devices of the specified type */
extern omp_device_t * omp_get_device(int id);
extern int omp_num_devices;
extern omp_device_t * omp_devices; /* an array of all device objects */
extern pthread_barrier_t all_dev_sync_barrier; /* this barrier sync with all device threads and the init thread, when needed */
extern volatile int omp_device_complete;
extern volatile int omp_printf_turn;
#define BEGIN_SERIALIZED_PRINTF(myturn) while (omp_printf_turn != myturn);
#define END_SERIALIZED_PRINTF() omp_printf_turn = (omp_printf_turn + 1)%omp_num_devices;

/* a topology of devices, or threads or teams */
typedef struct omp_grid_topology {
	 int nnodes;     /* Product of dims[*], gives the size of the topology */
	 int ndims;
	 int dims[OMP_MAX_NUM_DIMENSIONS];
	 int periodic[OMP_MAX_NUM_DIMENSIONS];
	 int * idmap; /* the seqid is the array index, each element is the mapped devid */
} omp_grid_topology_t;

/* APIs to support data/array mapping and distribution */
typedef enum omp_data_map_direction {
	OMP_DATA_MAP_TO,
	OMP_DATA_MAP_FROM,
	OMP_DATA_MAP_TOFROM,
	OMP_DATA_MAP_ALLOC,
} omp_data_map_direction_t;

typedef enum omp_data_map_type {
	OMP_DATA_MAP_AUTO, /* system choose, it is shared, but system will use either copy or shared depending on whether real sharing  is possible or not */
	OMP_DATA_MAP_SHARED,
	OMP_DATA_MAP_COPY,
} omp_data_map_type_t;

typedef enum omp_dist_policy {
	OMP_DIST_POLICY_BLOCK,
	OMP_DIST_POLICY_DUPLICATE,
	OMP_DIST_POLICY_AUTO, /* the balanced data distribution so computation is balanced distributed, ideally */
	OMP_DIST_POLICY_ALIGN,
	OMP_DIST_POLICY_CYCLIC, /* user defined */
	OMP_DIST_POLICY_FIX, /* fixed dist */
} omp_dist_policy_t;

typedef enum omp_dist_target_type {
	OMP_DIST_TARGET_DATA_MAP,
	OMP_DIST_TARGET_LOOP_ITERATION,
} omp_dist_target_type_t;
/**
 * Info object for dist (array and iteration)
 */
typedef struct omp_dist_info {
	omp_dist_policy_t policy; /* the dist policy */
	long start; /* the start index for the dim of the original array */
	long length;   /* the length (total # element to be distributed) */
	long stride; /* stride between ele, default 1 of course */
	long chunk_size;
	int dim_index; /* the index of top dim to apply dist, for block, duplicate, auto. For ALIGN, this is dim at the alignee*/

	omp_dist_target_type_t alignee_type; /* a data map or a loop iteration */
	union alignee_t { /* The dist this dist_info aligns with, it could be a loop iteration or a data map. It uses dim_index to reference which dim */
		omp_offloading_info_t * loop_iteration;
		omp_data_map_info_t * data_map_info;
	} alignee;

	/* the following are the container so we know which array/loop uses this dist, a backtrack pointer of a dist_info */
	omp_dist_target_type_t target_type;
	int target_dim;
	void * target; /* opaque, see below union */
	/*
	union dist_target_t {
		omp_offloading_info_t * loop_iteration;
		omp_data_map_info_t * data_map_info;
	};
	*/
} omp_dist_info_t;

typedef enum omp_dist_halo_edging_type {
	OMP_DIST_HALO_EDGING_NOHALO = 1,
	OMP_DIST_HALO_EDGING_REFLECTING,
	OMP_DIST_HALO_EDGING_PERIODIC,
} omp_dist_halo_edging_type_t;

/**
 * in each dimension, halo region have left and right halo region and also a flag for cyclic halo or not,
 */
typedef struct omp_data_map_halo_region {
	/* the info */
	int left; /* element size, it is the elements needed from left (not to provide) */
	int right; /* element size */
	omp_dist_halo_edging_type_t edging;
	int topdim; /* which dimension of the device topology this halo region is related to, it is the same as the dim_index of dist object of the same dimension */
} omp_data_map_halo_region_info_t;

/* for each mapped host array, we have one such object */
struct omp_data_map_info {
    omp_offloading_info_t *off_info;
    const char * symbol; /* the user symbol */
	char * source_ptr;
	int num_dims;
	long dims[OMP_MAX_NUM_DIMENSIONS];

	int sizeof_element;
	omp_data_map_direction_t map_direction; /* the map type, to, from, or tofrom */
	omp_data_map_type_t map_type;
	omp_data_map_t * maps; /* a list of data maps of this array */

	/*
	 * The dist array maintains info on how the array should be distributed among dev topology, not including
	 * the halo region info.
	 */
	omp_dist_info_t dist_info[OMP_MAX_NUM_DIMENSIONS];

	 /* the halo region: halo region is considered out-of-bound access of the main array region,
	  * thus the index could be -1, -2, or larger than the dimensions. Our memory allocation and pointer
	  * arithmetic will make sure we do not go out of memory bound
	  */
	int num_halo_dims; /* a simple flag for checking whether this map has halo or not */
	omp_data_map_halo_region_info_t halo_info[OMP_MAX_NUM_DIMENSIONS]; /* it is an num_dims array */
};

/** a data map can only be changed by the shepherd thread of the device that map is belong to, but
 * other shepherds can read when the info is ready. The access level is used to control when others can read
 * on what info
 */
typedef enum omp_data_map_access_level {
	OMP_DATA_MAP_ACCESS_LEVEL_0, /* garbage value */
	OMP_DATA_MAP_ACCESS_LEVEL_1, /* basic info, such as dev, info, is there */
	OMP_DATA_MAP_ACCESS_LEVEL_2, /* map_dim, offset, etc info is there */
	OMP_DATA_MAP_ACCESS_LEVEL_3, /* if halo, halo neightbors are set up, and halo buffers are allocated */
	OMP_DATA_MAP_ACCESS_LEVEL_4, /* dev mem buffer is allocated */

} omp_data_map_access_level_t;

typedef struct omp_data_map_halo_region_mem {
	/* the mem for halo management */
	/* the in/out pointer is the buffer for the halo regions.
	 * The in ptr is the buffer for halo region that will be copied in,
	 * and the out is for those that will be copied out.
	 * In implementation, we put the in and out buffer into one mem space for each left and right halo region
	 */
	int left_dev_seqid; /* left devseqid, can be used to access left_map and left_dev */
	int right_dev_seqid;

	char * left_in_ptr; /* the halo region this needs */
	long left_in_size; /* for pull update, == right_out_size if push protocol is used */
	char * left_out_ptr; /* the halo region this will privide to the left */
	long left_out_size;

	char * right_in_ptr;
	long right_in_size; /* for pull update, == left_out_size if push protocol is used */
	char * right_out_ptr;
	long right_out_size;

	/* if p2p communication is not available, we will need buffer at host to relay the halo exchange.
	 * Each data map only maintains the relay pointers for halo that they need, i.e. a pull
	 * protocol should be applied for halo exchange.
	 */
	char * left_in_host_relay_ptr;
	volatile int left_in_data_in_relay_pushed;
	volatile int left_in_data_in_relay_pulled;
	/* the push flag is set when the data is pushed by the source to the host relay so the receiver side can pull,
	 * a simple busy-wait is used to wait for the data to arrive
	 */
	char * right_in_host_relay_ptr;
	volatile int right_in_data_in_relay_pushed;
	volatile int right_in_data_in_relay_pulled;
} omp_data_map_halo_region_mem_t;

/**
 * dist object, a subregion of the whole region defined in info object
 */
typedef struct omp_dist {
	omp_dist_info_t * info; /* not yet used so far */
	long offset;
	long length;
} omp_dist_t;

/* for each device, we maintain a list such objects, each for one mapped array */
struct omp_data_map {
	omp_data_map_access_level_t access_level;
	omp_data_map_info_t * info;
    omp_device_t * dev;
	omp_data_map_type_t map_type;

	/* the subarray for the mapped region, including the offset and length */
	omp_dist_t map_dist[OMP_MAX_NUM_DIMENSIONS];
	/* the sizes in bytes */
	long map_size; // = map_dim[0] * map_dim[1] * map_dim[2] * sizeof_element;
	long map_wextra_size; /* include extras, e.g. halo */

	/* source side */
	char * map_source_ptr; /* the mapped buffer on host. This pointer is either the	offset pointer from the source_ptr */
	char * map_source_wextra_ptr;

	/* dev side */
	char * map_dev_ptr; /* the mapped buffer on device, only for the mapped array region (not including halo region) */
	char * map_dev_wextra_ptr; /* the mapped buffer on device, include extras, such as halo region */

	omp_data_map_halo_region_mem_t halo_mem [OMP_MAX_NUM_DIMENSIONS];

	int mem_noncontiguous;
	//omp_dev_stream_t * stream; /* the stream operations of this data map are registered with, mostly it will be the stream created for an offloading */
};

/**
 * the data exchange direction, FROM is for pull, TO is for push
 */
typedef enum omp_data_map_exchange_direction {
	OMP_DATA_MAP_EXCHANGE_FROM_LEFT_RIGHT,
	OMP_DATA_MAP_EXCHANGE_FROM_LEFT_ONLY,
	OMP_DATA_MAP_EXCHANGE_FROM_RIGHT_ONLY,
	OMP_DATA_MAP_EXCHANGE_TO_LEFT_RIGHT,
	OMP_DATA_MAP_EXCHANGE_TO_LEFT_ONLY,
	OMP_DATA_MAP_EXCHANGE_TO_RIGHT_ONLY,
} omp_data_map_exchange_direction_t;

/**
 * the data exchange info, used for forwarding a request to shepherd thread to perform
 * parallel data exchange between devices, e.g. halo region exchange
 */
typedef struct omp_data_map_halo_exchange_info {
	omp_data_map_info_t * map_info; /* the map info the exchange needs to perform */
	int x_dim;
	omp_data_map_exchange_direction_t x_direction;
} omp_data_map_halo_exchange_info_t;

typedef enum omp_offloading_type {
	OMP_OFFLOADING_DATA, /* e.g. omp target data, i.e. only offloading data */
	OMP_OFFLOADING_DATA_CODE, /* e.g. omp target, i.e. offloading both data and code, and all the data used by the code are specified in this one */
	OMP_OFFLOADING_CODE, /* e.g. omp target, i.e. offloading code and partial data only, for other data, inherent data offloaded by the enclosing omp target data */

	/* data exchange offloading, while a regular offloading can carry data exchange that will
	 * be performed after finishing the offloading tasks, this type of offloading is a standalone data exchange offloading */
	OMP_OFFLOADING_STANDALONE_DATA_EXCHANGE,
} omp_offloading_type_t;

/**
 * A sequence of calls in one offloading involve.
 * set up stream and event for queuing and sync purpose, we currently only one stream per
 * device at a time, i.e. multiple concurrent streams are not supported.
 * 1. compute data mapping region for each variables, allocate device memory to store mapped data
 * 2. copy data from host to device (could happen while allocating device memory)
 * 3. launch the kernel that will work on the data
 * 4. if any, do data exchange between host and devices, and between devices
 * 5. more kernel launch and data exchange
 * 6. copy data from device to host, deallocate memory that will not be used
 *
 * Between each of the above steps, a barrier may be needed.
 * The thread will can be used to a accelerator
 */
typedef enum omp_offloading_stage {
	OMP_OFFLOADING_INIT,      /* initialization of device, e.g. stream, host barrier, etc */
	OMP_OFFLOADING_MAPMEM,    /* compute data map and allocate memory/buffer on host and device */
	OMP_OFFLOADING_COPYTO,    /* copy data from host to device */
	OMP_OFFLOADING_KERNEL,    /* kernel execution */
	OMP_OFFLOADING_EXCHANGE,  /* p2p (dev2dev) and h2d (host2dev) data exchange */
	OMP_OFFLOADING_COPYFROM,  /* copy data from dev to host */
	OMP_OFFLOADING_SYNC, 		/* sync*/
	OMP_OFFLOADING_SYNC_CLEANUP, /* sync and clean up */
	OMP_OFFLOADING_MDEV_BARRIER, /* mdev barrier wait to sync all participating devices */
	OMP_OFFLOADING_COMPLETE,  /* anything else if there is */
	OMP_OFFLOADING_NUM_STEPS, /* total number of steps */
} omp_offloading_stage_t;

/* a kernel profile keep track of info such as # of iterations, # nest loop, # load per iteration, # store per iteration, # FP per operations
 * data access pattern that has locality/cache access impact, etc
 */
typedef struct omp_kernel_profile_info {
	unsigned long num_load;
	unsigned long num_store;
	unsigned long num_fp_operations;
} omp_kernel_profile_info_t;

/**
  * info per kernel/data offloading
  *
  * For each offloading to one or multiple devices, we will maintain an object of omp_offloading_info
  * that keeps track of the topology of target devices, the mapped variables and other info.
  *
  * The barrier is used for syncing target devices
  *
  * Also each device maintains its own offloading stack, the nest offloading operations to the device, see omp_device object.
  *
  * when an off_info being forwarded to dev helper thread, it should be read-only
  */
struct omp_offloading_info {
	/************** per-offloading var, shared by all target devices ******/
	omp_grid_topology_t * top; /* target devices */
	const char * name; /* kernel name */

	int free_after_completion; /* a flag to notify off thread whether to clean up memory after offloading */

	volatile int count; /* if an offload is within a loop (while/for, etc and with goto) of a function, it is a recurring */
	double start_time;
	double compl_time;
	double elapsed;

	omp_offloading_type_t type; /* what to offload: data, code, or both*/
	int num_mapped_vars;
	omp_data_map_info_t * data_map_info; /* an entry for each mapped variable */
	omp_offloading_t * offloadings; /* a list of dev-specific offloading objects, num of this is num of targets from top */
	omp_kernel_profile_info_t full_kernel_profile;
	omp_kernel_profile_info_t per_iteration_profile;

	/* max three level of loop nest */
	omp_dist_info_t loop_dist_info[OMP_MAX_NUM_DIMENSIONS];
	int loop_depth; /* max 3 so far */

	/* the universal kernel launcher and args, the helper thread will use this one if no dev-specific one is provided */
	/* the helper thread will first check these two field, if they are not null, it will use them, otherwise, it will use the dev-specific one */
	void * args;
	void (*kernel_launcher)(omp_offloading_t *, void *); /* the same kernel to be called by each of the target device, if kernel == NULL, we are just offloading data */

	/* there are two approaches we handle halo exchange, 1) halo exchange as part of a previous regular kernel offloading and halo exchange
	 * will be executed right after finishing the kernel. 2) halo exchange as a standalone offloading operations.
	 *
	 * For standalone data exchange offloading, the type is OMP_OFFLOADING_STANDALONE_DATA_EXCHANGE, for the first option, runtime will
	 * check whether halo_x_info pointer is NULL or not to see whether there is appended data exchange or not after a regular kernel
	 */
	omp_data_map_halo_exchange_info_t * halo_x_info;
	int num_maps_halo_x;

	/* the participating barrier */
	pthread_barrier_t barrier; /* this barrier sync with offloading thread */
	pthread_barrier_t inter_dev_barrier; /* this barrier sync between devices only */

};

#define OFF_MAP_CACHE_SIZE 64
/**
 * info for per device
 *
 * For each offloading, we have this object to keep track per-device info, e.g. the stream used, mapped region of
 * a variable (array or scalar) and kernel info.
 *
 * The next pointer is used to form the offloading queue (see omp_device struct)
 */
struct omp_offloading {
	/* per-offloading info */
	omp_offloading_info_t * off_info;

	/************** per device var ***************/	
	omp_dev_stream_t mystream;
	omp_dev_stream_t *stream;
	int devseqid; /* device seqid in the top */
	omp_device_t * dev; /* the dev object, as cached info */
	omp_offloading_stage_t stage;

	/* we will use a simple fix-sized array for simplicity and performance (than a linked list) */
	/* an offload can has as many as OFFLOADING_MAP_CACHE_SIZE mapped variable */
	struct {
		omp_data_map_t * map;
		int inherited; /* flag to mark whether this is an inherited map or not */
	} map_cache[OFF_MAP_CACHE_SIZE];
	int num_maps;

	omp_dist_t loop_dist[3];
	omp_kernel_profile_info_t kernel_profile;
	omp_kernel_profile_info_t per_iteration_profile;
	int loop_dist_done; /* a flag */

	/* for profiling purpose */
	omp_event_t *events;
	int num_events;

	/* the auto model purpose */
	double Ar;
	double Br;

	/* kernel info */
	long X1, Y1, Z1; /* the first level kernel thread configuration, e.g. CUDA blockDim */
	long X2, Y2, Z2; /* the second level kernel thread config, e.g. CUDA gridDim */
	void *args;
	void (*kernel_launcher)(omp_offloading_t *, void *); /* device specific kernel, if any */
};

/* init the device objects, num_of_devices, helper threads, default_device_var ICV etc 
    return # of devices initialized 
*/
extern int omp_init_devices(); 

/* terminate helper threads
 */
extern void omp_fini_devices();
extern char * omp_get_device_typename(omp_device_t * dev);
extern int omp_get_num_active_devices();
extern int omp_set_current_device_dev(omp_device_t * d); /* return the current device id */
extern int omp_set_current_device(int id); /* return the current device id */
extern void helper_thread_main(void * arg);
extern void omp_warmup_device(omp_device_t * dev);

extern omp_offloading_info_t * omp_offloading_init_info(const char *name, omp_grid_topology_t *top, int recurring,
														omp_offloading_type_t off_type, int num_maps,
														void (*kernel_launcher)(omp_offloading_t *, void *), void *args,
														int loop_nest_depth);
extern void omp_offloading_append_profile_per_iteration(omp_offloading_info_t *info, long num_fp_operations,
														long num_loads, long num_stores);
extern void omp_offloading_fini_info(omp_offloading_info_t * info);
extern void omp_offloading_info_report_profile(omp_offloading_info_t * info);

extern void omp_offloading_append_data_exchange_info (omp_offloading_info_t * info, omp_data_map_halo_exchange_info_t * halo_x_info, int num_maps_halo_x);
extern omp_offloading_info_t * omp_offloading_standalone_data_exchange_init_info(const char *name, omp_grid_topology_t *top, int recurring,
																				 omp_data_map_halo_exchange_info_t *halo_x_info, int num_maps_halo_x);
extern void omp_offloading_start(omp_offloading_info_t *off_info, int free_after_completion);

extern void omp_stream_create(omp_device_t *d, omp_dev_stream_t *stream);
extern void omp_stream_destroy(omp_dev_stream_t * st);
extern void omp_stream_sync(omp_dev_stream_t *st);
extern void omp_map_free(omp_data_map_t *map, omp_offloading_t *off);

extern void omp_event_init(omp_event_t * ev, omp_device_t * dev, omp_event_record_method_t record_method);
extern void omp_event_print(omp_event_t * ev);
extern void omp_event_record_start(omp_event_t * ev, omp_dev_stream_t * stream, const char * event_name, const char * event_msg, ...);
extern void omp_event_record_stop(omp_event_t * ev);
extern void omp_event_print_profile_header();
extern void omp_event_print_elapsed(omp_event_t *ev, double reference, double *start_time, double *elapsed);
extern void omp_event_elapsed_ms(omp_event_t * ev);
extern void omp_event_accumulate_elapsed_ms(omp_event_t * ev);
extern void omp_offloading_clear_report_info(omp_offloading_info_t * info);

extern omp_grid_topology_t * omp_grid_topology_init_simple(int nnodes, int ndims);
/*  factor input n into dims number of numbers (store into factor[]) whose multiplication equals to n */
extern void omp_grid_topology_fini(omp_grid_topology_t * top);
extern void omp_factor(int n, int factor[], int dims);
extern void omp_topology_print(omp_grid_topology_t * top);
extern int omp_grid_topology_get_seqid(omp_grid_topology_t * top, int devid);

extern void omp_data_map_init_info(const char *symbol, omp_data_map_info_t *info, omp_offloading_info_t *off_info,
                            void *source_ptr, int num_dims, int sizeof_element, omp_data_map_direction_t map_direction,
                            omp_data_map_type_t map_type);
extern void omp_data_map_info_set_dims_1d(omp_data_map_info_t * info, long dim0);
extern void omp_data_map_info_set_dims_2d(omp_data_map_info_t * info, long dim0, long dim1);
extern void omp_data_map_info_set_dims_3d(omp_data_map_info_t * info, long dim0, long dim1, long dim2);

extern void omp_print_map_info(omp_data_map_info_t * info);

extern void omp_data_map_dist_init_info(omp_data_map_info_t *map_info, int dim, omp_dist_policy_t dist_policy,
										long start, long length, int topdim);
extern void omp_loop_dist_init_info(omp_offloading_info_t *off_info, int level, omp_dist_policy_t dist_policy,
									long start,
									long length, int topdim);
extern void omp_data_map_dist_align_with_data_map_with_halo(omp_data_map_info_t *map_info, int dim, long start,
                                                     omp_data_map_info_t *alignee, int alignee_dim);
extern void omp_data_map_dist_align_with_data_map(omp_data_map_info_t *map_info, int dim, long start,
                                           omp_data_map_info_t *alignee, int alignee_dim);
extern void omp_data_map_dist_align_with_loop(omp_data_map_info_t *map_info, int dim, long start,
                                       omp_offloading_info_t *alignee, int alignee_level);
extern void omp_loop_dist_align_with_data_map(omp_offloading_info_t *loop_off_info, int level, long start,
                                       omp_data_map_info_t *alignee, int alignee_dim);
extern void omp_loop_dist_align_with_loop(omp_offloading_info_t *loop_off_info, int level, long start,
                                   omp_offloading_info_t *alignee, int alignee_level);
extern void omp_data_map_init_map(omp_data_map_t *map, omp_data_map_info_t *info, omp_device_t *dev);
extern void omp_data_map_dist(omp_data_map_t *map, int seqid);
extern void omp_loop_iteration_dist(omp_offloading_t * off);
extern void omp_map_add_halo_region(omp_data_map_info_t *info, int dim, int left, int right,
									omp_dist_halo_edging_type_t edging);
//extern int omp_data_map_has_halo(omp_data_map_info_t * info, int dim);
//extern int omp_data_map_get_halo_left_devseqid(omp_data_map_t * map, int dim);
//extern int omp_data_map_get_halo_right_devseqid(omp_data_map_t * map, int dim);

extern omp_data_map_t *omp_map_offcache_iterator(omp_offloading_t *off, int index, int * inherited);
extern void omp_map_append_map_to_offcache(omp_offloading_t *off, omp_data_map_t *map, int inherited);
extern int omp_map_is_map_inherited(omp_offloading_t *off, omp_data_map_t *map);
extern omp_data_map_t * omp_map_get_map_inheritance (omp_device_t * dev, void * host_ptr);
extern omp_data_map_t * omp_map_get_map(omp_offloading_t *off, void * host_ptr, int map_index);
extern void omp_print_data_map(omp_data_map_t * map);
extern void omp_map_malloc(omp_data_map_t *map, omp_offloading_t *off);
extern void * omp_map_marshal(omp_data_map_t *map);

extern void omp_map_unmarshal(omp_data_map_t * map);
extern void omp_map_free_dev(omp_device_t * dev, void * ptr);
extern void * omp_map_malloc_dev(omp_device_t * dev, long size);
extern void * omp_unified_malloc(long size);
extern void omp_unified_free(void *ptr);
extern void omp_map_mapto(omp_data_map_t * map);
extern void omp_map_mapto_async(omp_data_map_t * map, omp_dev_stream_t * stream);
extern void omp_map_mapfrom(omp_data_map_t * map);
extern void omp_map_mapfrom_async(omp_data_map_t * map, omp_dev_stream_t * stream);
extern void omp_map_memcpy_to(void * dst, omp_device_t * dstdev, const void * src, long size);
extern void omp_map_memcpy_to_async(void * dst, omp_device_t * dstdev, const void * src, long size, omp_dev_stream_t * stream);
extern void omp_map_memcpy_from(void * dst, const void * src, omp_device_t * srcdev, long size);
extern void omp_map_memcpy_from_async(void * dst, const void * src, omp_device_t * srcdev, long size, omp_dev_stream_t * stream);
extern int omp_map_enable_memcpy_DeviceToDevice(omp_device_t * dstdev, omp_device_t * srcdev);
extern void omp_map_memcpy_DeviceToDevice(void * dst, omp_device_t * dstdev, void * src, omp_device_t * srcdev, int size) ;
extern void omp_map_memcpy_DeviceToDeviceAsync(void * dst, omp_device_t * dstdev, void * src, omp_device_t * srcdev, int size, omp_dev_stream_t * srcstream);

extern void omp_halo_region_pull(omp_data_map_t * map, int dim, omp_data_map_exchange_direction_t from_left_right);
extern void omp_halo_region_pull_async(omp_data_map_t * map, int dim, int from_left_right);

extern int omp_get_max_threads_per_team(omp_device_t * dev);
extern int omp_get_optimal_threads_per_team(omp_device_t * dev);
extern int omp_get_max_teams_per_league(omp_device_t * dev);
extern int omp_get_optimal_teams_per_league(omp_device_t * dev, int threads_per_team, int total);

#if defined (DEVICE_NVGPU_SUPPORT)
extern void xomp_beyond_block_reduction_float_stream_callback(cudaStream_t stream,  cudaError_t status, void* userData );
#endif

/**
 * return the mapped range index from the iteration range of the original array
 * e.g. A[128], when being mapped to a device for A[64:64] (from 64 to 128), then, the range 100 to 128 in the original A will be
 * mapped to 36 to 64 in the mapped region of A
 *
 * @param: omp_data_map_t * map: the mapped variable, we should use the original pointer and let the runtime retrieve the map
 * @param: int dim: which dimension to retrieve the range
 * @param: int start: the start index from the original array, if start is -1, use the map_offset_<dim>, which will simply cause
 * 					the function return 0 for obvious reasons
 * @param: int length: the length of the range, if -1, use the mapped dim from the start
 * @param: int * map_start: the mapped start index in the mapped range, if return <0 value, wrong input
 * @param: int * map_length: normally just the length, if lenght == -1, use the map_dim[dim]
 *
 * NOTE: the mapped range must be a subset of the range of the specified map in the specified dim
 *
 */
extern long omp_loop_get_range(omp_offloading_t *off, int loop_level, long *start, long *length);

/* util */
extern double read_timer_ms();
extern double read_timer();
extern void devcall_errchk(int code, char *file, int line, int abort);
#define devcall_assert(ecode) { devcall_errchk((ecode), __FILE__, __LINE__, 1); }

#ifdef __cplusplus
 }
#endif

#endif /* __HOMP_H__ */
