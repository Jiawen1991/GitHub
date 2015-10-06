#include "axpy.h"

/* standard one-dev support */
void axpy_ompacc(REAL* x, REAL* y, int n, REAL a) {
  int i;
   /* this one defines both the target device name and data environment to map to,
      I think here we need mechanism to tell the compiler the device type (could be multiple) so that compiler can generate the codes of different versions; 
      we also need to let the runtime know what the target device is so the runtime will chose the right function to call if the code are generated 
      #pragma omp target device (gpu0) map(x, y) 
   */
  #pragma omp target device (gpu0) map(inout: y[0:n]) map(in: x[0:n],a,n)
  #pragma omp parallel for shared(x, y, n, a) private(i)
  for (i = 0; i < n; ++i)
    y[i] += a * x[i];
}

/* version 1: use omp parallel, i.e. each host thread responsible for one dev */
void axpy_ompacc_mdev_1(REAL* x, REAL* y, int n, REAL a) {
  int ndev = omp_get_num_devices(); /* standard omp call, see ticket 167 */
  printf("There are %d devices available\n", ndev);
  REAL *lx;
  REAL *ly;
  #pragma omp parallel num_threads(ndev) private(lx, ly) private(lx, ly)
  {
	int i;
    	/* chunking it for each device */
	int devid = omp_get_thread_num();
	cudaSetDevice(devid);
    	int remain = n % ndev;
        int esize = n / ndev;
        int partsize, starti, endi;
	if (devid < remain) { /* each of the first remain dev has one more element */
		partsize = esize+1;
		starti = partsize*devid;
	} else {
		partsize = esize;
		starti = esize*devid+remain;
	}
	endi=starti + partsize;
	printf("dev %d range: %d-%d\n", devid, starti, endi);
	lx = &x[starti];
	ly = &y[starti];
#pragma omp target device (devid) map(inout: ly[0:partsize]) map(in: lx[0:partsize],a,partsize)
#pragma omp parallel for shared(lx, ly, partsize, a)  private(i)
	for (i = 0; i < partsize; ++i)
	  ly[i] += a * lx[i];
  }
}

void axpy_mdev_v2(REAL* x, REAL* y, int n, REAL a) {

#pragma omp target device (:) map(tofrom: y[0:n]>>(:)) map(to: x[0:n]>>(:),a,n)
#pragma omp parallel for shared(x, y, n, a) private(i) dist_iteration match_range x[:]
/* in this example, the y[0:n], and x[0:n] will be evenly distributed among the ndev devices, scalars such as a and n will each have a mapped copy in all the devices */
  for (i = 0; i < n; ++i)
    y[i] += a * x[i];
}

#if 0
/* version 3: we should allow the following data mapping */
void axpy_mdev_v3(REAL* x, REAL* y, int n, REAL a) {
  int ndev = omp_get_num_devices();

#pragma omp parallel for
/* here we use devices, not device, map to three devices explicitly listed, and the data mapping is explicitly listed too */
#pragma omp target devices (0,1,ndev-1) map(inout: y[0:n/2]>>0, y[n/2:n]>>1,y[0:n]>>ndev-1) map(in: x[0:n]>>0, x[0:n]>>1,a,n)
/* if no explicitly-specified mapping deviice, they are mapping to all the devices, e.g. scalars such as a and n in this example will each have a mapped copy in all the devices */
  for (i = 0; i < n; ++i)
    y[i] += a * x[i];
}

/* version 4: the following data mapping is more dynamic */
void axpy_mdev_v3(REAL* x, REAL* y, int n, REAL a) {
  int ndev = omp_get_num_devices();
  int devid;

#pragma omp parallel for
/* here we use devices, not device, map to three devices explicitly listed, and the data mapping is explicitly listed too */
#pragma omp target devices (did:[0:ndev]) map(inout: y[n/did-1:n/did]>>did) map(in: x[0:n]>>did,a,n)
/* if no explicitly-specified mapping deviice, they are mapping to all the devices, e.g. scalars such as a and n in this example will each have a mapped copy in all the devices */
  for (i = 0; i < n; ++i)
    y[i] += a * x[i];
}

#endif
