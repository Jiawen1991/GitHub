#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <sys/time.h>
#include "homp.h"
#include "stencil2d.h"

#if defined (DEVICE_NVGPU_SUPPORT)
extern __global__ void stencil2d_nvgpu_kernel(int start_n, int len_n, long n, long m, int u_dimX, int u_dimY, REAL *u, REAL *uold, int radius, int coeff_dimX, REAL *coeff);
#endif

void stencil2d_omp_mdev_off_launcher(omp_offloading_t *off, void *args) {
    struct stencil2d_off_args * iargs = (struct stencil2d_off_args*) args;
    long n = iargs->n;
    long m = iargs->m;
    int radius = iargs->radius;
    int num_its = iargs->num_its;
    long u_dimX = iargs->u_dimX;
    long u_dimY = iargs->u_dimY;
    int coeff_dimX = iargs->coeff_dimX;

    omp_data_map_t * map_u = omp_map_get_map(off, iargs->u, -1); /* 1 is for the map u */
    omp_data_map_t * map_uold = omp_map_get_map(off, iargs->uold, -1); /* 2 is for the map uld */
    omp_data_map_t * map_coeff = omp_map_get_map(off, iargs->coeff, -1); /* 2 is for the map uld */

    REAL * u = (REAL*) map_u->map_dev_wextra_ptr;
    REAL * uold = (REAL*) map_uold->map_dev_wextra_ptr;
    REAL *coeff = (REAL*) map_coeff->map_dev_wextra_ptr;
    coeff = coeff + (2*radius+1) * radius + radius; /* TODO this should be a call to map a host-side address to dev-side address*/
    int count = 4*radius+1;
#ifdef SQUARE_SETNCIL
	count = coeff_dimX * coeff_dimX;
#endif

    long it; /* iteration */
#if CORRECTNESS_CHECK
    printf("kernel launcher: u: %X, uold: %X\n", u, uold);
    print_array("u in device: ", "udev", u, n, m);
    print_array("uold in device: ", "uolddev", uold, n, m);
#endif

    long offset;
    long start;
    long len;
    if (dist_dim == 1) {
        offset = omp_loop_get_range(off, 0, &start, &len);
    } else if (dist_dim == 2) {
        omp_loop_get_range(off, 0, &start, &len);
    } else /* vx == 3) */ {
        omp_loop_get_range(off, 0, &start, &len); /* todo */
        omp_loop_get_range(off, 0, &start, &len); /* todo */
    }
    omp_device_type_t devtype = off->dev->type;
    //printf("dev: %d, offset: %d, length: %d, local start: %d, u: %X, uold: %X, coeff-center: %X\n", off->devseqid, offset, len, start, u, uold, coeff);

//#pragma omp parallel shared(n, m, radius, coeff, num_its, u_dimX, u_dimY, coeff_dimX) private(it) firstprivate(u, uold)
    for (it = 0; it < num_its; it++) {
#if defined (DEVICE_NVGPU_SUPPORT)
		if (devtype == OMP_DEVICE_NVGPU) {
			dim3 threads_per_team(16, 16);
			dim3 teams_per_league((len+threads_per_team.x-1)/threads_per_team.x, (m+threads_per_team.y-1)/threads_per_team.y); /* we assume dividable */
            stencil2d_nvgpu_kernel<<<teams_per_league, threads_per_team, 0, off->stream->systream.cudaStream>>>
                (start, len, n, m, u_dimX, u_dimY, u, uold, radius, coeff_dimX, coeff);
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
			END_SERIALIZED_PRINTF();
#endif
//#pragma omp for private(ix, iy, ir)
            int ix, iy, ir;
            for (ix = start; ix < start+len; ix++) {
                REAL * temp_u = &u[(ix+radius)*u_dimY+radius];
                REAL * temp_uold = &uold[(ix+radius)*u_dimY+radius];
                for (iy = 0; iy < m; iy++) {
//                    if (off->devseqid == 0)printf("dev: %d, [%d][%d]:%f\n", off->devseqid, ix, iy, temp_u[0]);
                    REAL result = temp_uold[0] * coeff[0];
                    /* 2/4 way loop unrolling */
                    for (ir = 1; ir <= radius; ir++) {
                        result += coeff[ir] * temp_uold[ir];           		//horizontal right
                        result += coeff[-ir]* temp_uold[-ir];                  // horizontal left
                        result += coeff[-ir*coeff_dimX] * temp_uold[-ir * u_dimY]; //vertical up
                        result += coeff[ir*coeff_dimX] * temp_uold[ir * u_dimY]; // vertical bottom
#ifdef SQUARE_SETNCIL
						result += coeff[-ir*coeff_dimX-ir] * temp_uold[-ir * u_dimY-ir] // left upper corner
						result += coeff[-ir*coeff_dimX+ir] * temp_uold[-ir * u_dimY+ir] // right upper corner
						result += coeff[ir*coeff_dimX-ir] * temp_uold[ir * u_dimY]-ir] // left bottom corner
						result += coeff[ir*coeff_dimX+ir] * temp_uold[ir * u_dimY]+ir] // right bottom corner
#endif
                    }
                    *temp_u = result/count;
                    temp_u++;
                    temp_uold++;
                }
            }
        } else {
            fprintf(stderr, "device type is not supported for this call\n");
        }

        pthread_barrier_wait(&off->off_info->inter_dev_barrier);
        if (it % 2 == 0) omp_halo_region_pull(map_u, 0, OMP_DATA_MAP_EXCHANGE_FROM_LEFT_RIGHT);
        else omp_halo_region_pull(map_uold, 0, OMP_DATA_MAP_EXCHANGE_FROM_LEFT_RIGHT);

        REAL * tmp = uold;
        uold = u;
        u = tmp;
    }
}

void stencil2d_omp_mdev_iterate_off_launcher(omp_offloading_t * off, void *args) {
    struct stencil2d_off_args * iargs = (struct stencil2d_off_args*) args;
    long n = iargs->n;
    long m = iargs->m;
    int radius = iargs->radius;
    int num_its = iargs->num_its;
    long u_dimX = iargs->u_dimX;
    long u_dimY = iargs->u_dimY;
    int coeff_dimX = iargs->coeff_dimX;

    omp_data_map_t * map_u = omp_map_get_map(off, iargs->u, -1); /* 1 is for the map u */
    omp_data_map_t * map_uold = omp_map_get_map(off, iargs->uold, -1); /* 2 is for the map uld */
    omp_data_map_t * map_coeff = omp_map_get_map(off, iargs->coeff, -1); /* 2 is for the map uld */

    REAL * u = (REAL*) map_u->map_dev_wextra_ptr;
    REAL * uold = (REAL*) map_uold->map_dev_wextra_ptr;
    REAL *coeff = (REAL*) map_coeff->map_dev_wextra_ptr;
    coeff = coeff + (2*radius+1) * radius + radius; /* TODO this should be a call to map a host-side address to dev-side address*/
    int count = 4*radius+1;
#ifdef SQUARE_SETNCIL
	count = coeff_dimX * coeff_dimX;
#endif

    long it; /* iteration */
#if CORRECTNESS_CHECK
    printf("kernel launcher: u: %X, uold: %X\n", u, uold);
    print_array("u in device: ", "udev", u, n, m);
    print_array("uold in device: ", "uolddev", uold, n, m);
#endif

    long offset;
    long start;
    long len;
    if (dist_dim == 1) {
        offset = omp_loop_get_range(off, 0, &start, &len);
    } else if (dist_dim == 2) {
        omp_loop_get_range(off, 0, &start, &len);
    } else /* vx == 3) */ {
        omp_loop_get_range(off, 0, &start, &len); /* todo */
        omp_loop_get_range(off, 0, &start, &len); /* todo */
    }
    omp_device_type_t devtype = off->dev->type;
    //printf("dev: %d, offset: %d, length: %d, local start: %d, u: %X, uold: %X, coeff-center: %X\n", off->devseqid, offset, len, start, u, uold, coeff);

//#pragma omp parallel shared(n, m, radius, coeff, num_its, u_dimX, u_dimY, coeff_dimX) private(it) firstprivate(u, uold)
#if defined (DEVICE_NVGPU_SUPPORT)
	if (devtype == OMP_DEVICE_NVGPU) {
		dim3 threads_per_team(16, 16);
		dim3 teams_per_league((len+threads_per_team.x-1)/threads_per_team.x, (m+threads_per_team.y-1)/threads_per_team.y); /* we assume dividable */
           stencil2d_nvgpu_kernel<<<teams_per_league, threads_per_team, 0, off->stream->systream.cudaStream>>>
               (start, len, n, m, u_dimX, u_dimY, u, uold, radius, coeff_dimX, coeff);
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
		END_SERIALIZED_PRINTF();
#endif
//#pragma omp for private(ix, iy, ir)
        int ix, iy, ir;
        for (ix = start; ix < start + len; ix++) {
            REAL *temp_u = &u[(ix + radius) * u_dimY + radius];
            REAL *temp_uold = &uold[(ix + radius) * u_dimY + radius];
            for (iy = 0; iy < m; iy++) {
//                    if (off->devseqid == 0)printf("dev: %d, [%d][%d]:%f\n", off->devseqid, ix, iy, temp_u[0]);
                REAL result = temp_uold[0] * coeff[0];
                /* 2/4 way loop unrolling */
                for (ir = 1; ir <= radius; ir++) {
                    result += coeff[ir] * temp_uold[ir];                //horizontal right
                    result += coeff[-ir] * temp_uold[-ir];                  // horizontal left
                    result += coeff[-ir * coeff_dimX] * temp_uold[-ir * u_dimY]; //vertical up
                    result += coeff[ir * coeff_dimX] * temp_uold[ir * u_dimY]; // vertical bottom
#ifdef SQUARE_SETNCIL
						result += coeff[-ir*coeff_dimX-ir] * temp_uold[-ir * u_dimY-ir] // left upper corner
						result += coeff[-ir*coeff_dimX+ir] * temp_uold[-ir * u_dimY+ir] // right upper corner
						result += coeff[ir*coeff_dimX-ir] * temp_uold[ir * u_dimY]-ir] // left bottom corner
						result += coeff[ir*coeff_dimX+ir] * temp_uold[ir * u_dimY]+ir] // right bottom corner
#endif
                }
                *temp_u = result / count;
                temp_u++;
                temp_uold++;
            }
        }
    } else {
        fprintf(stderr, "device type is not supported for this call\n");
    }

    /*
    pthread_barrier_wait(&off->off_info->inter_dev_barrier);
    omp_halo_region_pull(map_u, 0, OMP_DATA_MAP_EXCHANGE_FROM_LEFT_RIGHT);
     */
}

