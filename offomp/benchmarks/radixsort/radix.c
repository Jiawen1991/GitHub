#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<string.h>
#include<math.h>
#include "homp.h"

#define FILENAME "a480.dat"
#define NUMBERS 480
#define DEBUG 1
#define REAL float

#if 0
/* v2: explicit distribution of both data and loop:
 * the y[0:n], and x[0:n] will be evenly distributed among the ndev devices,
 * scalars such as a and n will each have a mapped copy in all the devices, loop will also be evenly distributed */
void axpy_mdev_v2(REAL* x, REAL* y, long n, REAL a) {
#pragma omp target device (*) map(tofrom: y[0:n] dist_data(BLOCK)) map(to: x[0:n] dist_data(BLOCK),a,n)
#pragma omp parallel for shared(x, y, n, a) dist_iteration(BLOCK)
	for (i = 0; i < n; ++i)
	y[i] += a * x[i];
}

/* v3: block distribute array x and y and let the loop distribution aligh with x
 */
void axpy_mdev_v3(REAL* x, REAL* y, long n, REAL a) {
#pragma omp target device (*) map(tofrom: y[0:n] dist_data(BLOCK)) map(to: x[0:n] dist_data(BLOCK),a,n)
#pragma omp parallel for shared(x, y, n, a) dist_iteration(ALIGN(x))
	for (i = 0; i < n; ++i)
	y[i] += a * x[i];
}

/* v4: AUTO-distribute the loop iteration and let the distribution of array x and y to be aligned with loop distribution.
 */
void axpy_mdev_v4(REAL* x, REAL* y, long n, REAL a) {
#pragma omp target device (*) map(tofrom: y[0:n] dist_data(ALIGN)) map(to: x[0:n] dist_data(ALIGN),a,n)
#pragma omp parallel for shared(x, y, n, a) dist_iteration(AUTO)
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

extern int sort_mdev_v = 3;





//#define FILENAME "a480.dat"
//#define NUMBERS 480

//#define FILENAME "a4095.dat"
//#define NUMBERS 4095



REAL sort_ompacc_mdev(int *bufferInt);

int *r_values;
int *d_values;
int *t_values;

int *d_split;
int *d_e;
int *d_f;
int *d_t;
struct sort_args {
    int *bufferInt;
};

#if defined (DEVICE_NVGPU_SUPPORT)
#include "xomp_cuda_lib_inlined.cu"
__global__ void sort(long start_n, long length_n, int *bufferInt) {
	printf("Inside kernel \n");
	int _p_i;
	long _dev_lower;
	long _dev_upper;
	long _dev_loop_chunk_size;
	long _dev_loop_sched_index;
	long _dev_loop_stride;
	long _dev_thread_num = getCUDABlockThreadCount(1);
	long _dev_thread_id = getLoopInexFromCUDAVariables(1);
	//int x = (blockIdx.x * blockDim.x) + threadIdx.x;

	XOMP_static_sched_init(start_n, start_n + length_n - 1, 1, 1,
			_dev_thread_num, _dev_thread_id,
			&_dev_loop_chunk_size, &_dev_loop_sched_index,
			&_device_loop_stride);

	while (XOMP_static_sched_next
			(&_dev_loop_sched_index, start_n + length_n - 1, 1,
					_dev_loop_stride, _dev_loop_chunk_size, _dev_thread_num,
					_dev_thread_id, &_dev_lower, &_dev_upper))

	int b[NUMBERS], m = 0, exp = 1;
	for (_p_i = _dev_lower, _p_i <= _dev_upper; _p_i += 1) {
		for (x = 0; x < NUMBERS; x++) {
			{
				if (bufferInt[x] > m)
				m = bufferInt[x];
			}
			while (m / exp > 0) {
				int bucket[NUMBERS] = {0};

				for (x = 0; x < NUMBERS; x++)
				bucket[bufferInt[x] / exp % 10]++;
				__syncthreads();
				for (x = 1; x < NUMBERS; x++)
				bucket[x] += bucket[x - 1];
				__syncthreads();
				for (x = NUMBERS - 1; x >= 0; x--)
				b[--bucket[bufferInt[x] / exp % 10]] = bufferInt[x];

				__syncthreads();
				for (x = 0; x < NUMBERS; x++)
				bufferInt[x] = b[x];

				__syncthreads();
				exp *= 10;
			}
		}
	}
}
#endif


//Launcher function
void sort_launcher(omp_offloading_t *off, void *args) {
    printf("Inside launcher function \n");
    struct sort_args *argss = (struct sort_args *) args;
    long start_n, length_n;

    omp_data_map_t *map_bufferInt = omp_map_get_map(off, argss->bufferInt, -1);

    //LatLong *locations=(LatLong *)map_distances->map_dev_ptr;
    int *bufferInt = (int *) map_bufferInt->map_dev_ptr;

    omp_loop_get_range(off, 0, &start_n, &length_n);

    omp_device_type_t devtype = off->dev->type;
#if defined (DEVICE_NVGPU_SUPPORT)
	if (devtype == OMP_DEVICE_NVGPU) {
	int threads_per_team =
		omp_get_optimal_threads_per_team(off->dev);
		int teams_per_league = omp_get_optimal_teams_per_league(off->dev, threads_per_team,	length_n);

		sort <<< teams_per_league, threads_per_team, 0,	off->stream->systream.cudaStream >>> (start_n, length_n,bufferInt);
		fprintf("device type is GPU \n");
	}
	else
#endif

    if (devtype == OMP_DEVICE_THSIM) {
        int i;
//#pragma omp parallel for shared (bufferInt)private(i)
//serial code starts here:
        printf("Inside serial sort function \n");

        // int *bufferInt = NULL;
        char tmpLine[20];        //to store the contents of the line we read from the file
        FILE *handler = fopen(FILENAME, "r");
        if (handler == NULL) {
            printf("Unable to open file\n");
            exit(-1);
        }
        bufferInt = (int *) malloc(sizeof(int) * (NUMBERS));

        for (i = 0; i < NUMBERS; i++) {
            fgets(tmpLine, 20, handler);
            bufferInt[i] = atoi(tmpLine);
        }

#if DEBUG == 1
        printf("Printing output\n");
        //printing output to verify
        for (i = 0; i < NUMBERS; i++) {
            //      printf("%d \n ", bufferInt[i]);
        }
        printf("\n");
#endif
        //sorting starts here
        int b[NUMBERS], m = 0, exp = 1;

        for (i = 0; i < NUMBERS; i++) {
            if (bufferInt[i] > m)
                m = bufferInt[i];
        }
        while (m / exp > 0) {
            int bucket[NUMBERS] = {0};

            for (i = 0; i < NUMBERS; i++)
                bucket[bufferInt[i] / exp % 10]++; /* count the # elements in each bucket */
            for (i = 1; i < NUMBERS; i++)
                bucket[i] += bucket[i - 1]; /* find their starting position */
            for (i = NUMBERS - 1; i >= 0; i--)
                b[--bucket[bufferInt[i] / exp % 10]] = bufferInt[i]; /* sorting */
            for (i = 0; i < NUMBERS; i++)
                bufferInt[i] = b[i]; /* swap */
            exp *= 10;
        }
        for (i = 0; i < NUMBERS; i++) {
            //    printf("After sorting is %d \n",bufferInt[i]);
            if (i == 0) {
                printf("Minimum value of array is %d \n", bufferInt[i]);
            }
            if (i == NUMBERS - 1) {
                printf("Maximum value of array is %d \n", bufferInt[i]);
            }
            //Median Value calculation
            if (i == NUMBERS / 2) {
                if (i % 2 == 0) {
                    float temp = bufferInt[i] + bufferInt[i - 1];
                    printf("median value is %f \n", temp / 2);
                }
                else {
                    int temp1 = bufferInt[i];
                    printf("Median value is %d \n", temp1);
                }
            }
        }

        //serial code ends here


    }
    else {
        printf("device type is not supported for this call \n");
        abort();
    }

    printf("ending launcher function \n");
}


//sort_ompacc_mdev function
REAL sort_ompacc_mdev(int *bufferInt) {
    printf("Inside sort_ompacc_mdev function \n");
    double ompacc_init_time = read_timer_ms();

    printf("Initializing grid \n");
    /* use all the devices */
    int __num_targets__ = omp_get_num_active_devices(); /*XXX: = runtime or compiler generated code */
    omp_grid_topology_t *__top__ = omp_grid_topology_init_simple(__num_targets__, 1);
    /* init other infos (dims, periodic, idmaps) of top if needed */

    int __num_maps__ = 1; /* XXX: need compiler output */

    printf("Before mapping array \n");
    struct sort_args args;
    args.bufferInt = bufferInt;

    omp_offloading_info_t *__off_info__ = omp_offloading_init_info("Sorting Kernel", __top__, 1,
                                                                   OMP_OFFLOADING_DATA_CODE, __num_maps__,
                                                                   sort_launcher, &args, 1);
    /*TODO this profiles are incorrect */
    omp_offloading_append_profile_per_iteration(__off_info__, 1, 2, 1);

    printf("Befor calling the launcher function \n");
    //a for loop for multiple arrays
    omp_data_map_info_t *__bufferInt_map_info__ = &__off_info__->data_map_info[0];
    omp_data_map_init_info("bufferInt", __bufferInt_map_info__, __off_info__, bufferInt, 1, sizeof(int),
                           OMP_DATA_MAP_TOFROM, OMP_DATA_MAP_AUTO);
    omp_data_map_info_set_dims_1d(__bufferInt_map_info__, NUMBERS);

    printf("Before defining mdev types \n");
    if (sort_mdev_v == 3) {
        printf("Entering sort_mdev_v \n");
        omp_data_map_dist_init_info(__bufferInt_map_info__, 0, OMP_DIST_POLICY_BLOCK, 0, NUMBERS, 0);
        omp_loop_dist_align_with_data_map(__off_info__, 0, 0, __bufferInt_map_info__, 0);
        printf("VERSION 3: BLOCK dist policy for distances and location\n");
        printf("Exited sort_mdev_v version 3 \n");
    } else if (sort_mdev_v == 4) {
        omp_loop_dist_init_info(__off_info__, 0, OMP_DIST_POLICY_AUTO, 0, NUMBERS, 0);
        omp_data_map_dist_align_with_loop(__bufferInt_map_info__, 0, 0, __off_info__, 0);
        printf("Version 4: Auto policy for loop and distances and location with loop dist \n");
    } else {
        omp_data_map_dist_init_info(__bufferInt_map_info__, 0, OMP_DIST_POLICY_BLOCK, 0, NUMBERS, 0);
        omp_loop_dist_init_info(__off_info__, 0, OMP_DIST_POLICY_BLOCK, 0, NUMBERS, 0);
    }
#if DEBUG_MSG
	 printf("=========================================== offloading to %d targets ==========================================\n", __num_target_devices__);
#endif
    /* here we do not need sync start */
    printf("before omp_offloading_start function \n");
    ompacc_init_time = read_timer_ms() - ompacc_init_time;
    double off_total = read_timer_ms();
    int it;
    int total_its = 1;
    for (it = 0; it < total_its; it++)
        omp_offloading_start(__off_info__, it == total_its - 1);
    off_total = (read_timer_ms() - off_total) / total_its;
#if defined (OMP_BREAKDOWN_TIMING)
    printf("Before profiling thing \n");
    omp_print_map_info(__bufferInt_map_info__);
	omp_offloading_info_report_profile(__off_info__);
#endif

    omp_offloading_fini_info(__off_info__);
    omp_grid_topology_fini(__top__);
    off_total += ompacc_init_time;
    printf("After calculating cpu_total \n");
    return off_total;
}

void radix_serial(int *bufferInt) {
    int buffer[NUMBERS];
    int *b = &buffer[0];
    int m = 0, exp = 1;
    int i;
    for (i = 0; i < NUMBERS; i++) {
        if (bufferInt[i] > m)
            m = bufferInt[i];
    }
    int count = 0;
    while (m / exp > 0) {
        int bucket[NUMBERS] = {0};

        for (i = 0; i < NUMBERS; i++)
            bucket[bufferInt[i] / exp % 10]++; /* count the # elements in each bucket */
        for (i = 1; i < NUMBERS; i++)
            bucket[i] += bucket[i - 1]; /* find their starting position */
        for (i = NUMBERS - 1; i >= 0; i--)
            b[--bucket[bufferInt[i] / exp % 10]] = bufferInt[i]; /* sorting */
        /* a pointer swap */
        int *tmp = b;
        b = bufferInt;
        bufferInt = tmp;
        count++;
        exp *= 10;
    }

    if (count % 2) {/* swap back */
        memcpy(b, bufferInt, sizeof(int) * NUMBERS);
    }
}

void radix_omp(int *bufferInt) {
    int buffer[NUMBERS];
    int *b = &buffer[0];
    int m = 0, exp = 1;
    int i;
#pragma omp parallel for shared(bufferInt) reduction(max : m)
    for (i = 0; i < NUMBERS; i++) {
        if (bufferInt[i] > m)
            m = bufferInt[i];
    }
    int count = 0;
#pragma omp parallel shared()
    while (m / exp > 0) {
        int bucket[NUMBERS] = {0};

        for (i = 0; i < NUMBERS; i++)
            bucket[bufferInt[i] / exp % 10]++; /* count the # elements in each bucket */
        for (i = 1; i < NUMBERS; i++)
            bucket[i] += bucket[i - 1]; /* find their starting position */
        for (i = NUMBERS - 1; i >= 0; i--)
            b[--bucket[bufferInt[i] / exp % 10]] = bufferInt[i]; /* sorting */
        /* a pointer swap */
        int *tmp = b;
        b = bufferInt;
        bufferInt = tmp;
        count++;
        exp *= 10;
    }

    if (count % 2) {/* swap back */
        memcpy(b, bufferInt, sizeof(int) * NUMBERS);
    }
}

int main(int argc, char *argv[]) {

    printf("Inside main function \n");
    int *bufferInt = NULL;
    char tmpLine[20];        //to store the contents of the line we read     from the file
    int i;

    FILE *handler = fopen(FILENAME, "r");
    if (handler == NULL) {
        printf("Unable to open file\n");
        exit(-1);
    }
    bufferInt = (int *) malloc(sizeof(int) * (NUMBERS));
    int *bufferInt_omp = (int *) malloc(sizeof(int) * (NUMBERS));

    for (i = 0; i < NUMBERS; i++) {
        fgets(tmpLine, 20, handler);
        bufferInt[i] = atoi(tmpLine);
    }
    memcpy(bufferInt_omp, bufferInt, sizeof(int) * NUMBERS);

    radix_serial(bufferInt);
    for (i = 0; i < NUMBERS; i++) {
        printf("%d\n", bufferInt[i]);
    }

    omp_init_devices();
    REAL omp_time = read_timer_ms();
    //REAL sort_kernel_time = sort_ompacc_mdev(bufferInt_omp);
    omp_time = (read_timer_ms() - omp_time);
    omp_fini_devices();

    printf("ending main function \n");
    return (0);

}
