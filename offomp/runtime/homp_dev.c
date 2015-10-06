/*
 * homp_dev.c
 *
 * contains dev-specific implementation of homp.h functions, mainly those with
 *
 *  Created on: Oct 4, 2014
 *      Author: yy8
 */
/**
 * an easy way for defining dev-specific code:
#if defined (DEVICE_NVGPU_SUPPORT)

#elif defined (DEVICE_THSIM)

#else

#endif
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <math.h>
#include "homp.h"
#include "../util/iniparser.h"

inline void devcall_errchk(int code, char *file, int line, int ab) {
#if defined (DEVICE_NVGPU_SUPPORT)
	if (code != cudaSuccess) {
		fprintf(stderr,"GPUassert: %s %s %d\n", cudaGetErrorString(code), file, line);
		if (ab) { abort();}
	}
#elif defined (DEVICE_THSIM)
	if (code != 0) {
		fprintf(stderr, "devcal_assert: %d %s %d\n", code, file, line);
		if (ab) { abort();}
	}
#endif
}

double addmul(double add, double mul, int ops){
	// need to initialise differently otherwise compiler might optimise away
	double sum1=0.1, sum2=-0.1, sum3=0.2, sum4=-0.2, sum5=0.0;
	double mul1=1.0, mul2= 1.1, mul3=1.2, mul4= 1.3, mul5=1.4;
	int loops=ops/10;          // we have 10 floating point ops inside the loop
	double expected = 5.0*add*loops + (sum1+sum2+sum3+sum4+sum5)
			+ pow(mul,loops)*(mul1+mul2+mul3+mul4+mul5);

	int i;
	for(i=0; i<loops; i++) {
		mul1*=mul; mul2*=mul; mul3*=mul; mul4*=mul; mul5*=mul;
		sum1+=add; sum2+=add; sum3+=add; sum4+=add; sum5+=add;
	}
	return  sum1+sum2+sum3+sum4+sum5+mul1+mul2+mul3+mul4+mul5 - expected;
}

double cpu_sustain_gflopss (double * flopss) {
	double x=M_PI;
	double y=1.0+1e-8;
	int n = 1000000;
	double timer = read_timer();
	x=addmul(x,y,n);
	timer = read_timer() - timer;
	*flopss = n/timer/1e9;
}


/* OpenMP 4.0 support */
int default_device_var = -1;
void omp_set_default_device(int device_num ) {
	default_device_var = device_num;
}
int omp_get_default_device(void) {
	return default_device_var;
}

int omp_get_num_devices() {
	return omp_num_devices;
}

int omp_num_devices;
omp_device_t * omp_devices;
pthread_barrier_t all_dev_sync_barrier;
volatile int omp_device_complete = 0;

volatile int omp_printf_turn = 0; /* a simple mechanism to allow multiple dev shepherd threads to print in turn so the output do not scramble together */
omp_device_type_info_t omp_device_types[OMP_NUM_DEVICE_TYPES] = {
		{OMP_DEVICE_HOSTCPU, "OMP_DEVICE_HOSTCPU", "HOSTCPU", 1},
		{OMP_DEVICE_NVGPU, "OMP_DEVICE_NVGPU", "NVGPU", 0},
		{OMP_DEVICE_ITLMIC, "OMP_DEVICE_ITLMIC", "ITLMIC", 0},
		{OMP_DEVICE_TIDSP, "OMP_DEVICE_TIDSP", "TIDSP", 0},
		{OMP_DEVICE_AMDAPU, "OMP_DEVICE_AMDAPU", "AMDAPU", 0},
		{OMP_DEVICE_THSIM, "OMP_DEVICE_THSIM", "THSIM", 0},
		{OMP_DEVICE_REMOTE, "OMP_DEVICE_REMOTE", "REMOTE", 0},
		{OMP_DEVICE_LOCALPS, "OMP_DEVICE_LOCALPS", "LOCALPS", 0}
};

/* APIs to support multiple devices: */
char * omp_supported_device_types() { /* return a list of devices supported by the compiler in the format of TYPE1:TYPE2 */
	/* FIXME */
	return "OMP_DEVICE_HOSTCPU";
}
omp_device_type_t omp_get_device_type(int devid) {
	return omp_devices[devid].type;
}

char * omp_get_device_type_as_string(int devid) {
	return omp_device_types[omp_devices[devid].type].name;
}

int omp_get_num_devices_of_type(omp_device_type_t type) { /* current omp has omp_get_num_devices(); */
	return omp_device_types[type].num_devs;
}
/*
 * return the first ndev device IDs of the specified type, the function returns the actual number of devices
 * in the array (devnum_array)
 *
 * before calling this function, the caller should allocate the devnum_array[ndev]
 */
int omp_get_devices(omp_device_type_t type, int *devnum_array, int ndev) { /* return a list of devices of the specified type */
	int i;
	int num = 0;
	for (i=0; i<omp_num_devices; i++)
		if (omp_devices[i].type == type && num <= ndev) {
			devnum_array[num] = omp_devices[i].id;
			num ++;
		}
	return num;
}
omp_device_t * omp_get_device(int id) {
	return &omp_devices[id];
}

int omp_get_num_active_devices() {
	int num_dev;
	char * ndev = getenv("OMP_NUM_ACTIVE_DEVICES");
	if (ndev != NULL) {
		num_dev = atoi(ndev);
		if (num_dev == 0 || num_dev > omp_num_devices) num_dev = omp_num_devices;
	} else {
		num_dev = omp_num_devices;
	}
	return num_dev;
}

void omp_init_hostcpu_device(omp_device_t *dev, int id, int sysid, int num_cores) {
	dev->type = OMP_DEVICE_HOSTCPU;
	dev->id = id;
	dev->sysid = sysid;
	dev->devstream.dev = dev;
	dev->devstream.systream.myStream = NULL;
	dev->mem_type = OMP_DEVICE_MEM_SHARED_CC_NUMA;
	dev->dev_properties = &dev->helperth; /* make it point to the thread id */
	dev->num_cores = num_cores;
	//dev->num_cores = sysconf( _SC_NPROCESSORS_ONLN );
	double dummy = cpu_sustain_gflopss(&dev->flopss_percore);
	dev->total_real_flopss = dev->num_cores * dev->flopss_percore;
	dev->bandwidth = 600*1000; /* GB/s */
	dev->latency = 0.02; /* us, i.e. 20 ns */
}

void * omp_init_thsim_device(omp_device_t * dev, int id, int sysid, int num_cores) {
	dev->type = OMP_DEVICE_THSIM;
	dev->id = id;
	dev->sysid = sysid;
	dev->devstream.dev = dev;
	dev->devstream.systream.myStream = NULL;
	dev->mem_type = OMP_DEVICE_MEM_DISCRETE;
	dev->dev_properties = &dev->helperth; /* make it point to the thread id */
	dev->num_cores = num_cores;
	/*
 	dev->num_cores = omp_host_dev->num_cores;
	dev->flopss_percore = omp_host_dev->flopss_percore;
	dev->total_real_flopss = omp_host_dev->total_real_flopss*(1+dev->id);
	dev->bandwidth = (2*(1+dev->id))*omp_host_dev->bandwidth / 100;
	dev->latency = (1+dev->id)*omp_host_dev->latency * 1000;
	*/
}

void omp_init_nvgpu_device(omp_device_t * dev, int id, int sysid) {
	dev->type = OMP_DEVICE_NVGPU;
	dev->id = id;
	dev->sysid = sysid;
	dev->devstream.dev = dev;
	dev->devstream.systream.myStream = NULL;
	dev->mem_type = OMP_DEVICE_MEM_DISCRETE;
}

void omp_util_copy_device_object(omp_device_t* new, omp_device_t* src, int newid, int newsysid) {
	memcpy(new, src, sizeof(omp_device_t));
	new->id = newid;
	new->sysid = newsysid;
	new->devstream.dev = new;
	new->devstream.systream.myStream = NULL;
	new->dev_properties = &new->helperth;
}

int num_hostcpu_dev = 0;
int num_thsim_dev = 0; /* the thsim actually */
/* for NVDIA GPU devices */
int num_nvgpu_dev = 0;

/* we allow the same type of device to be specified using a simple form, e.g.
 *
 *
[cpu]
num = 10;
id = 0
type = cpu
ncores = 8
FLOPss = 2 # GFLOPS/s
Bandwidth = 600000 # MB/s
Latency = 0 #us
 */
void omp_read_device_spec(char * dev_spec_file) {
	dictionary *ini;
	ini = iniparser_load(dev_spec_file);
	if (ini == NULL) {
		fprintf(stderr, "cannot parse file: %s\n", dev_spec_file);
		abort();
	}
	//iniparser_dump(ini, stderr);
	int num_sections = iniparser_getnsec(ini);
	int i;
	/* count total number of devices */
	omp_num_devices = 0;
	char devname[32];
	char keyname[48];
	for (i = 0; i < num_sections; i++) {
		sprintf(devname, "%s", iniparser_getsecname(ini, i));
		sprintf(keyname, "%s:%s", devname, "num");
		int num_devs = iniparser_getint(ini, keyname, 1);
		if (num_devs > 0) omp_num_devices += num_devs;
	}
	omp_devices = malloc(sizeof(omp_device_t) * (omp_num_devices));

	int devid = 0;
	for (i = 0; i < num_sections; i++) {
		sprintf(devname, "%s", iniparser_getsecname(ini, i));
		sprintf(keyname, "%s:%s", devname, "num");
		int num_devs = iniparser_getint(ini, keyname, 1);
		if (num_devs <= 0) continue;

		omp_device_t *dev = &omp_devices[devid];

		sprintf(devname, "%s", iniparser_getsecname(ini, i));
		sprintf(keyname, "%s:%s", devname, "sysid");
		int devsysid = iniparser_getint(ini, keyname, -1);
		sprintf(dev->name, "%s:%d", devname, devsysid);
		sprintf(keyname, "%s:%s", devname, "ncores");
		int num_cores = iniparser_getint(ini, keyname, 1);
		char * devtype;
		sprintf(keyname, "%s:%s", devname, "type");
		devtype = iniparser_getstring(ini, keyname, "NULL");

		if (strcasecmp(devtype, "cpu") == 0 || strcasecmp(devtype, "hostcpu") == 0 ) {
			omp_init_hostcpu_device(dev, devid, devsysid, num_cores);
			num_hostcpu_dev += num_devs;
		} else if (strcasecmp(devtype, "gpu") == 0) {
			omp_init_nvgpu_device(dev, devid, devsysid);
			num_nvgpu_dev += num_devs;
		} else if (strcasecmp(devtype, "thsim") == 0) {
			omp_init_thsim_device(dev, devid, devsysid, num_cores);
			num_thsim_dev += num_devs;
		} else {
			printf("unknow device type error: %s \n, default to be hostcpu\n", devtype);
			/* unknow device type error */
		}

		sprintf(keyname, "%s:%s", devname, "flopss");
		dev->total_real_flopss = iniparser_getdouble(ini, keyname, -1);

		sprintf(keyname, "%s:%s", devname, "Bandwidth");
		dev->bandwidth = iniparser_getdouble(ini, keyname, -1);

		sprintf(keyname, "%s:%s", devname, "Latency");
		dev->latency = iniparser_getdouble(ini, keyname, 0.00000000001);

		sprintf(keyname, "%s:%s", devname, "Memory");
		char * mem = iniparser_getstring(ini, keyname, "default"); /* or shared */
		if (strcasecmp(mem, "shared") == 0) {
			dev->mem_type = OMP_DEVICE_MEM_SHARED;
		} else if (strcasecmp(mem, "discrete") == 0) {
			dev->mem_type = OMP_DEVICE_MEM_DISCRETE;
		} else {
			/* using default, already done in init_*_device call */
		}

		devid++;
		/* repeating the same type of devices */
		int j;
		for (j=1; j<num_devs; j++) {
			omp_device_t * new = &omp_devices[devid];
			omp_util_copy_device_object(new, dev, devid, devsysid+j);
			sprintf(new->name, "%s:%d", devname, devsysid+j);
			devid++;
		}
	}

	iniparser_freedict(ini);
}

void omp_probe_devices() {
	/* query hardware device */

	/* OMP_HOSTCPU_AS_DEVICE=true|true:4|false */
	char true_false[6];
	char * host_as_dev_str = getenv("OMP_HOSTCPU_AS_DEVICE");
	if (host_as_dev_str != NULL ) {
		sscanf(host_as_dev_str, "%s", &true_false);
		if (strncasecmp(host_as_dev_str, "false", 5) == 0) {
//			printf("host as device: false\n");
		} else if (strncasecmp(host_as_dev_str, "true", 4) == 0) {
			if (host_as_dev_str[4] == ':') {
				sscanf(host_as_dev_str+5, "%d", &num_hostcpu_dev);
				if (num_hostcpu_dev < 0) num_hostcpu_dev = 1;
			}
//			printf("host as device: true, #cores: %d\n", num_cores_host_dev);
		} else {
			printf("Unrecognized OMP_HOSTCPU_AS_DEVICE value(%s), use default: false\n", host_as_dev_str);
		}
	} else {
//		printf("default: false\n");
	}
	omp_num_devices += num_hostcpu_dev;

	char * num_thsim_dev_str = getenv("OMP_NUM_THSIM_DEVICES");
	if (num_thsim_dev_str != NULL ) {
		sscanf(num_thsim_dev_str, "%d", &num_thsim_dev);
		if (num_thsim_dev < 0) num_thsim_dev = 0;
	} else num_thsim_dev = 0;

	omp_num_devices += num_thsim_dev;

	/* for NVDIA GPU devices */
	int total_gpudevs = 0;

#if defined (DEVICE_NVGPU_SUPPORT)
	cudaError_t result = cudaGetDeviceCount(&total_gpudevs);
	devcall_assert(result);
#endif

	int gpu_selection[total_gpudevs];
	int i;
	for (i=0; i<total_gpudevs;i++) gpu_selection[i] = 0;

#if defined (DEVICE_NVGPU_SUPPORT)
	if (total_gpudevs > 0) {
		char * nvgpu_dev_str = getenv("OMP_NVGPU_DEVICES");
		if (nvgpu_dev_str != NULL ) {
			char * token = strtok(nvgpu_dev_str, ",");
			while(token != NULL) {
				int gpuid;
				sscanf(token, "%d", &gpuid);
				gpu_selection[gpuid] = 1;
				num_nvgpu_dev ++;
				token = strtok(NULL, ",");
			}
		} else {
			char * num_nvgpu_dev_str = getenv("OMP_NUM_NVGPU_DEVICES");
			if (num_nvgpu_dev_str != NULL ) {
				sscanf(num_nvgpu_dev_str, "%d", &num_nvgpu_dev);
				if (num_nvgpu_dev > total_gpudevs || num_nvgpu_dev < 0) num_nvgpu_dev = total_gpudevs;
			} else num_nvgpu_dev = total_gpudevs;
			for (i=0; i<num_nvgpu_dev;i++) gpu_selection[i] = 1;
		}

		omp_num_devices += num_nvgpu_dev;
		omp_device_types[OMP_DEVICE_NVGPU].num_devs = num_nvgpu_dev;
	}
#endif

	omp_devices = malloc(sizeof(omp_device_t) * (omp_num_devices));

	int host_dev_sysid = 0;
	int thsim_dev_sysid = 0;
	int gpu_dev_sysid = 0;

	for (i=0; i<omp_num_devices; i++) {
		omp_device_t * dev = &omp_devices[i];

		if (i < num_hostcpu_dev) {
			omp_init_hostcpu_device(dev, i, host_dev_sysid, 1);
			sprintf(dev->name, "%s:%d", omp_get_device_typename(dev), host_dev_sysid);
			host_dev_sysid++;
		} else if (i < num_thsim_dev+ num_hostcpu_dev) {
			omp_init_thsim_device(dev, i, thsim_dev_sysid, 1);
			sprintf(dev->name, "%s:%d", omp_get_device_typename(dev), thsim_dev_sysid);
			thsim_dev_sysid++;
		} else if (i < num_nvgpu_dev + num_hostcpu_dev + num_thsim_dev) {
			for (; gpu_dev_sysid <total_gpudevs; gpu_dev_sysid++) {
				if (gpu_selection[gpu_dev_sysid]) {
					break;
				}
			}
			omp_init_nvgpu_device(dev, i, gpu_dev_sysid);
			sprintf(dev->name, "%s:%d", omp_get_device_typename(dev), gpu_dev_sysid);
			gpu_dev_sysid++;
		} else {
			/* TODO: unknown device type error */
		}


	}
}
/* init the device objects, num_of_devices, helper threads, default_device_var ICV etc
 *
 */
int omp_init_devices() {
	/* query hardware device */
	omp_num_devices = 0; /* we always have at least host device */

	int i;

	char * dev_spec_file = getenv("OMP_DEV_SPEC_FILE");
	if (dev_spec_file != NULL) {
		omp_read_device_spec(dev_spec_file);
	} else {
		omp_probe_devices();
	}
	if (omp_num_devices) {
		default_device_var = 0;
	} else {
		default_device_var = -1;
	}
	omp_device_types[OMP_DEVICE_HOSTCPU].num_devs = num_hostcpu_dev;
	omp_device_types[OMP_DEVICE_THSIM].num_devs = num_thsim_dev;
	omp_device_types[OMP_DEVICE_NVGPU].num_devs = num_nvgpu_dev;

	/* create the helper thread for each device */
	/* the helper thread setup */
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	/* initialize attr with default attributes */
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_setconcurrency(omp_num_devices+1);
	pthread_barrier_init(&all_dev_sync_barrier, NULL, omp_num_devices+1);

	for (i=0; i<omp_num_devices; i++) {
		omp_device_t * dev = &omp_devices[i];

		dev->status = 1;
		dev->resident_data_maps = NULL;
		dev->offload_request = NULL;
		dev->offload_stack_top = -1;

		int rt = pthread_create(&dev->helperth, &attr, (void *(*)(void *))helper_thread_main, (void *) dev);
		if (rt) {fprintf(stderr, "cannot create helper threads for devices.\n"); exit(1); }
	}

	printf("=====================================================================================================================\n");
	printf("System has total %d devices, %d HOSTCPU (one CPU is one dev), %d GPU and %d THSIM; default dev: %d.\n", omp_num_devices,
		   num_hostcpu_dev, num_nvgpu_dev, num_thsim_dev, default_device_var);
	for (i=0; i<omp_num_devices; i++) {
		omp_device_t * dev = &omp_devices[i];
		char * mem_type = "SHARED";
		if (dev->mem_type == OMP_DEVICE_MEM_DISCRETE) {
			mem_type = "DISCRETE";
		}
		printf("\t%d|sysid: %d, type: %s, name: %s, ncores: %d, mem: %s, flops: %0.2fGFLOPS/s, bandwidth: %.2fMB/s, latency: %.2fus\n",
			   dev->id, dev->sysid, omp_get_device_typename(dev), dev->name, dev->num_cores, mem_type, dev->total_real_flopss, dev->bandwidth,
			   dev->latency);
		//printf("\t\tstream dev: %s\n", dev->devstream.dev->name);
		if (dev->type == OMP_DEVICE_NVGPU) {
			if (dev->mem_type == OMP_DEVICE_MEM_DISCRETE) {
#if defined(DEVICE_NVGPU_VSHAREDM)
			printf("\t\tUnified Memory is supported in the runtime, but this device is not set to use it. To use it, enable shared mem in the dev spec(Memory=shared)\n");
#endif
			}
			if (dev->mem_type == OMP_DEVICE_MEM_SHARED) {
#if defined(DEVICE_NVGPU_VSHAREDM)
#else
				printf("\t\tUnified Memory is NOT supported in the runtime, fall back to discrete memory for this device. To enable shared mem support in runtime, set the DEVICE_NVGPU_VSHAREDM macro.\n");
				dev->mem_type = OMP_DEVICE_MEM_DISCRETE;
#endif
			}
		}
	}
	printf("The device specifications can be provided by a spec file, or through system probing:\n");
	printf("\tTo provide dev spec file, use OMP_DEV_SPEC_FILE variable\n");
	printf("\tTo help system probing and customize configuration, using the following environment variable\n");
	printf("\t\tOMP_HOSTCPU_AS_DEVICE for enabling hostcpu as devices, e.g. true|TRUE:4|false, default false.\n");
	printf("\t\t\tTRUE:4, means 4 hostcpu to be used as devices\n");
	printf("\t\tOMP_NUM_THSIM_DEVICES for selecting a number of THSIM devices (default 0)\n");
	printf("\t\tOMP_NUM_NVGPU_DEVICES for selecting a number of NVIDIA GPU devices from dev 0 (default, total available).\n");
	printf("\t\t\tThis variable is overwritten by OMP_NVGPU_DEVICES).\n");
	printf("\t\tOMP_NVGPU_DEVICES for selecting specific NVGPU devices (e.g., \"0,2,3\",no spaces)\n");
	printf("=====================================================================================================================\n");

	pthread_barrier_wait(&all_dev_sync_barrier);
	return omp_num_devices;
}

void omp_warmup_device(omp_device_t * dev) {
	omp_device_type_t devtype = dev->type;
	if (devtype == OMP_DEVICE_NVGPU) {
#if defined (DEVICE_NVGPU_SUPPORT)
		dev->dev_properties = (struct cudaDeviceProp*)malloc(sizeof(struct cudaDeviceProp));
		cudaSetDevice(dev->sysid);
		cudaGetDeviceProperties(dev->dev_properties, dev->sysid);

		/* warm up the device */
		void * dummy_dev;
		char dummy_host[1024];
		cudaMalloc(&dummy_dev, 1024);
		cudaMemcpy(dummy_dev, dummy_host, 1024, cudaMemcpyHostToDevice);
		cudaMemcpy(dummy_host, dummy_dev, 1024, cudaMemcpyDeviceToHost);
		cudaFree(dummy_dev);
#endif
	} else if (devtype == OMP_DEVICE_THSIM || devtype == OMP_DEVICE_HOSTCPU) {
		/* warm up the OpenMP environment */
		/*
		int i;
		int dummy_size = dev->num_cores * 100;
		float dummy_array[dummy_size];
#pragma omp parallel for shared(dummy_size, dummy_array) private (i)
		for (i = 0; i < dummy_size; i++) {
			dummy_array[i] *= i * dummy_array[(i + dev->num_cores) % dummy_size];
		}
		 */
	} else {
		/* abort(); unknow device type */
	}
	omp_stream_sync(&dev->devstream);
}

// terminate helper threads
void omp_fini_devices() {
	int i;

	omp_device_complete = 1;
	for (i=0; i<omp_num_devices; i++) {
		omp_device_t * dev = &omp_devices[i];
		int rt = pthread_join(dev->helperth, NULL);
		omp_device_type_t devtype = dev->type;
#if defined (DEVICE_NVGPU_SUPPORT)
		if (devtype == OMP_DEVICE_NVGPU) {
			free(dev->dev_properties);
		}
#endif
	}

	pthread_barrier_destroy(&all_dev_sync_barrier);
	free(omp_devices);
}

int omp_set_current_device_dev(omp_device_t * d) {
#if defined (DEVICE_NVGPU_SUPPORT)
    int result;
	if (d->type == OMP_DEVICE_NVGPU) {
		result = cudaSetDevice(d->sysid);
		devcall_assert (result);
	}
#endif
	return d->id;
}

void omp_map_mapto(omp_data_map_t * map) {
	if (map->map_type == OMP_DATA_MAP_COPY) omp_map_memcpy_to((void*)map->map_dev_wextra_ptr, map->dev, (void*)map->map_source_wextra_ptr, map->map_wextra_size);
}

void omp_map_mapto_async(omp_data_map_t * map, omp_dev_stream_t * stream) {
	if (map->map_type == OMP_DATA_MAP_COPY) {
		omp_map_memcpy_to_async((void*)map->map_dev_wextra_ptr, map->dev, (void*)map->map_source_wextra_ptr, map->map_wextra_size, stream);
	//	printf("%s, dev: %d, mapto: %X <--- %X\n", map->info->symbol, map->dev->id, map->map_dev_ptr, map->map_source_ptr);
	//	printf("%s, dev: %d, mapto: %X <--- %X of extra\n",  map->info->symbol, map->dev->id, map->map_dev_wextra_ptr, map->map_source_wextra_ptr);
	}
}

void omp_map_mapfrom(omp_data_map_t * map) {
	if (map->map_type == OMP_DATA_MAP_COPY)
		omp_map_memcpy_from((void*)map->map_source_wextra_ptr, (void*)map->map_dev_wextra_ptr, map->dev, map->map_wextra_size); /* memcpy from host to device */
}

void omp_map_mapfrom_async(omp_data_map_t * map, omp_dev_stream_t * stream) {
	if (map->map_type == OMP_DATA_MAP_COPY) {
	//	omp_map_memcpy_from_async((void*)map->map_source_ptr, (void*)map->map_dev_ptr, map->dev, map->map_size, stream); /* memcpy from host to device */
		omp_map_memcpy_from_async((void*)map->map_source_wextra_ptr, (void*)map->map_dev_wextra_ptr, map->dev, map->map_wextra_size, stream); /* memcpy from host to device */
	//	printf("%s, dev: %d, mapfrom: %X <--- %X\n", map->info->symbol, map->dev->id, map->map_source_ptr, map->map_dev_ptr);
	//	printf("%s, dev: %d, mapfrom: %X <--- %X of extra\n",  map->info->symbol, map->dev->id, map->map_source_wextra_ptr, map->map_dev_wextra_ptr);
	}
}

void * omp_unified_malloc(long size) {
	void * ptr = NULL;
#if defined (DEVICE_NVGPU_SUPPORT) && defined (DEVICE_NVGPU_VSHAREDM)
#if defined (DEVICE_NVGPU_CUDA_UNIFIEDMEM)
	/* this is only for kepler and > 4.0 cuda rt */
	cudaMallocManaged(&ptr, size, 0);
#else
	/* cuda zero-copy */
	cudaError_t result;
	result = cudaHostAlloc(&ptr, size, cudaHostAllocPortable || cudaHostAllocMapped);
	devcall_assert(result);
#endif
#else
	ptr = malloc(size);
#endif
	return ptr;
}

void omp_unified_free(void * ptr) {
#if defined (DEVICE_NVGPU_SUPPORT) && defined (DEVICE_NVGPU_VSHAREDM)
#if defined (DEVICE_NVGPU_CUDA_UNIFIEDMEM)
	/* match cudaMallocManaged */
	cudaFree(ptr);
#else
	/* cuda zero-copy */
	cudaFreeHost(ptr);
#endif
#else
	free(ptr);
#endif
	return;
}

void * omp_map_malloc_dev(omp_device_t * dev, long size) {
	omp_device_type_t devtype = dev->type;
	void * ptr = NULL;
#if defined (DEVICE_NVGPU_SUPPORT)
	if (devtype == OMP_DEVICE_NVGPU) {
		if (cudaErrorMemoryAllocation == cudaMalloc(&ptr, size)) {
			fprintf(stderr, "cudaMalloc error to allocate mem on device\n");
		}
	} else
#endif
	if (devtype == OMP_DEVICE_THSIM || devtype == OMP_DEVICE_HOSTCPU) {
		ptr = malloc(size);
	} else {
		fprintf(stderr, "device type is not supported for this call\n");
		abort();
	}

	//printf("dev memory allocated on %d, %X\n", dev->id, ptr);
	return ptr;
}

void omp_map_free_dev(omp_device_t * dev, void * ptr) {
	omp_device_type_t devtype = dev->type;
#if defined (DEVICE_NVGPU_SUPPORT)
	if (devtype == OMP_DEVICE_NVGPU) {
	    cudaError_t result = cudaFree(ptr);
	    devcall_assert(result);
	} else
#endif
	if (devtype == OMP_DEVICE_THSIM || devtype == OMP_DEVICE_HOSTCPU) {
		free(ptr);
	} else {
		fprintf(stderr, "device type is not supported for this call\n");
		abort();
	}
}

void omp_map_memcpy_to(void * dst, omp_device_t * dstdev, const void * src, long size) {
	omp_device_type_t devtype = dstdev->type;
#if defined (DEVICE_NVGPU_SUPPORT)
	if (devtype == OMP_DEVICE_NVGPU) {
	    cudaError_t result;
	    result = cudaMemcpy((void *)dst,(const void *)src,size, cudaMemcpyHostToDevice);
	    devcall_assert(result);
	} else
#endif
	if (devtype == OMP_DEVICE_THSIM || devtype == OMP_DEVICE_HOSTCPU) {
		memcpy((void *)dst,(const void *)src,size);
	} else {
		fprintf(stderr, "device type is not supported for this call\n");
		abort();
	}
}

void omp_map_memcpy_to_async(void * dst, omp_device_t * dstdev, const void * src, long size, omp_dev_stream_t * stream) {
//	printf("memcpytoasync: dev: %d, %X->%X\n", dstdev->id, src, dst);
	omp_device_type_t devtype = dstdev->type;
#if defined (DEVICE_NVGPU_SUPPORT)
	if (devtype == OMP_DEVICE_NVGPU) {
		cudaError_t result;
		result = cudaMemcpyAsync((void *)dst,(const void *)src,size, cudaMemcpyHostToDevice, stream->systream.cudaStream);
		devcall_assert(result);
	} else
#endif
	if (devtype == OMP_DEVICE_THSIM || devtype == OMP_DEVICE_HOSTCPU) {
//		fprintf(stderr, "no async call support, use sync memcpy call\n");
		memcpy((void *)dst,(const void *)src,size);
	} else {
		fprintf(stderr, "device type is not supported for this call\n");
		abort();
	}
}

void omp_map_memcpy_from(void * dst, const void * src, omp_device_t * srcdev, long size) {
	omp_device_type_t devtype = srcdev->type;
#if defined (DEVICE_NVGPU_SUPPORT)
	if (devtype == OMP_DEVICE_NVGPU) {
		cudaError_t result;
	    result = cudaMemcpy((void *)dst,(const void *)src,size, cudaMemcpyDeviceToHost);
		devcall_assert(result);
	} else
#endif
	if (devtype == OMP_DEVICE_THSIM || devtype == OMP_DEVICE_HOSTCPU) {
		memcpy((void *)dst,(const void *)src,size);
	} else {
		fprintf(stderr, "device type is not supported for this call\n");
		abort();
	}
}

/**
 *  device to host, async */
void omp_map_memcpy_from_async(void * dst, const void * src, omp_device_t * srcdev, long size, omp_dev_stream_t * stream) {
	omp_device_type_t devtype = srcdev->type;
#if defined (DEVICE_NVGPU_SUPPORT)
	if (devtype == OMP_DEVICE_NVGPU) {
		cudaError_t result;
		result = cudaMemcpyAsync((void *)dst,(const void *)src,size, cudaMemcpyDeviceToHost, stream->systream.cudaStream);
		devcall_assert(result);
	} else
#endif
	if (devtype == OMP_DEVICE_THSIM || devtype == OMP_DEVICE_HOSTCPU) {
//		fprintf(stderr, "no async call support, use sync memcpy call\n");
		memcpy((void *)dst,(const void *)src,size);
//		printf("memcpy from: dest: %X, src: %X, size: %d\n", map->map_buffer, map->map_dev_ptr);
	} else {
		fprintf(stderr, "device type is not supported for this call\n");
		abort();
	}
}

/**
 * this should be calling from src for NGVPU implementation
 */
int omp_map_enable_memcpy_DeviceToDevice(omp_device_t * dstdev, omp_device_t * srcdev) {
	omp_device_type_t dst_devtype = dstdev->type;
	omp_device_type_t src_devtype = srcdev->type;

#if defined (DEVICE_NVGPU_SUPPORT)
	if (dst_devtype == OMP_DEVICE_NVGPU && src_devtype == OMP_DEVICE_NVGPU) {
		int can_access = 0;
		cudaError_t result;
		result = cudaDeviceCanAccessPeer(&can_access, srcdev->sysid, dstdev->sysid);
		devcall_assert(result);
		if (can_access) {
			result = cudaDeviceEnablePeerAccess(dstdev->sysid, 0);
		    if(result != cudaErrorPeerAccessAlreadyEnabled) {
		    	return 0;
		    } else return 1;
		} else return 1;
	}
#endif
#if defined EXPERIMENT_RELAY_BUFFER_FOR_HALO_EXCHANGE
	return 0;
#else
	return 1;
#endif
}

void omp_map_memcpy_DeviceToDevice(void * dst, omp_device_t * dstdev, void * src, omp_device_t * srcdev, int size) {
	omp_device_type_t dst_devtype = dstdev->type;
	omp_device_type_t src_devtype = srcdev->type;

#if defined (DEVICE_NVGPU_SUPPORT)
	if (dst_devtype == OMP_DEVICE_NVGPU && src_devtype == OMP_DEVICE_NVGPU) {
		cudaError_t result;
	    result = cudaMemcpy((void *)dst,(const void *)src,size, cudaMemcpyDeviceToDevice);
//		result = cudaMemcpyPeer(dst, dstdev->sysid, src, srcdev->sysid, size);
		devcall_assert(result);
	    return;
	} else if ((dst_devtype == OMP_DEVICE_THSIM || dst_devtype == OMP_DEVICE_HOSTCPU) && src_devtype == OMP_DEVICE_NVGPU) {
		cudaError_t result;
	    result = cudaMemcpy((void *)dst,(const void *)src,size, cudaMemcpyDeviceToHost);
		devcall_assert(result);
	    return;
	} else if(dst_devtype == OMP_DEVICE_NVGPU && (src_devtype == OMP_DEVICE_THSIM || src_devtype == OMP_DEVICE_HOSTCPU)) {
		cudaError_t result;
	    result = cudaMemcpy((void *)dst,(const void *)src,size, cudaMemcpyHostToDevice);
		devcall_assert(result);
	    return;
	}
#endif
	if ((dst_devtype == OMP_DEVICE_THSIM || dst_devtype == OMP_DEVICE_HOSTCPU) && (src_devtype == OMP_DEVICE_THSIM || src_devtype ==
																													  OMP_DEVICE_HOSTCPU)) {
		memcpy((void *)dst, (const void *)src, size);
	} else {
		fprintf(stderr, "device type is not supported for this call: %s:%d\n", __FILE__, __LINE__);
		abort();
	}
}

/** it is a push operation, i.e. src push data to dst */
void omp_map_memcpy_DeviceToDeviceAsync(void * dst, omp_device_t * dstdev, void * src, omp_device_t * srcdev, int size, omp_dev_stream_t * srcstream) {
	omp_device_type_t dst_devtype = dstdev->type;
	omp_device_type_t src_devtype = srcdev->type;

#if defined (DEVICE_NVGPU_SUPPORT)
	if (dst_devtype == OMP_DEVICE_NVGPU && src_devtype == OMP_DEVICE_NVGPU) {
		cudaError_t result;
	    result = cudaMemcpyAsync((void *)dst,(const void *)src,size, cudaMemcpyDeviceToDevice,srcstream->systream.cudaStream);
	    //result = cudaMemcpyPeerAsync(dst, dstdev->sysid, src, srcdev->sysid, size, srcstream->systream.cudaStream);

		devcall_assert(result);
	} else
#endif
	if (dst_devtype == OMP_DEVICE_THSIM && src_devtype == OMP_DEVICE_THSIM) {
		memcpy((void *)dst, (const void *)src, size);
	} else {
		fprintf(stderr, "device type is not supported for this call: %s:%d\n", __FILE__, __LINE__);
		abort();
	}
}

/* In the current implementation of the runtime, we will NOT use stream callback to do the timing and others such as reduction operation.
 * The reason is because CUDA use a driver thread to handle callback, which become not necessnary since we have a dedicated helper thread
 * for each GPU and the helper thread could do this kind of work
 */
#if defined (DEVICE_NVGPU_SUPPORT)

#if 0
void xomp_beyond_block_reduction_float_stream_callback(cudaStream_t stream,  cudaError_t status, void*  userData ) {
	omp_reduction_float_t * rdata = (omp_reduction_float_t*)userData;
	float result = 0.0;
	int i;
	for (i=0; i<rdata->num; i++)
		result += rdata->input[i];
	rdata->result = result;
}
#endif

void omp_stream_host_timer_callback(cudaStream_t stream,  cudaError_t status, void*  userData ) {
	double * time = (double*)userData;
	*time = read_timer_ms();
}
#endif

void omp_stream_create(omp_device_t *d, omp_dev_stream_t *stream) {
	stream->dev = d;
	int i;

#if defined (DEVICE_NVGPU_SUPPORT)
	if (d->type == OMP_DEVICE_NVGPU) {
		cudaError_t result;
		//stream->systream.cudaStream = 0;
		result = cudaStreamCreateWithFlags(&stream->systream.cudaStream, cudaStreamNonBlocking);
		devcall_assert(result);
	} else
#endif
	if (d->type == OMP_DEVICE_THSIM || d->type == OMP_DEVICE_HOSTCPU){
		/* do nothing */
	} else {

	}
}

/**
 * sync device by syncing the stream so all the pending calls the stream are completed
 *
 * if destroy_stream != 0; the stream will be destroyed.
 */
void omp_stream_sync(omp_dev_stream_t *st) {
	omp_device_type_t devtype = st->dev->type;
#if defined (DEVICE_NVGPU_SUPPORT)
	if (devtype == OMP_DEVICE_NVGPU) {
		cudaError_t result;
		result = cudaStreamSynchronize(st->systream.cudaStream);
		devcall_assert(result);
	}
#else
#endif
}

void omp_stream_destroy(omp_dev_stream_t * st) {
	omp_device_type_t devtype = st->dev->type;
#if defined (DEVICE_NVGPU_SUPPORT)
	if (devtype == OMP_DEVICE_NVGPU) {
		cudaError_t result;
		result = cudaStreamDestroy(st->systream.cudaStream);
		devcall_assert(result);
	}
#else
#endif
}

/* the event msg has limited length defined by OMP_EVENT_MSG_LENGTH macro, additional char will be cut off */
void omp_event_init(omp_event_t * ev, omp_device_t * dev, omp_event_record_method_t record_method) {
	ev->dev = dev;
	ev->record_method = record_method;
	omp_device_type_t devtype = dev->type;
	ev->count = 0;
	ev->recorded = 0;
	ev->elapsed_dev = ev->elapsed_host = 0.0;
	ev->event_name = NULL;
	ev->event_description[0] = '\0';
	if (record_method == OMP_EVENT_DEV_RECORD || record_method == OMP_EVENT_HOST_DEV_RECORD) {
#if defined (DEVICE_NVGPU_SUPPORT)
		if (devtype == OMP_DEVICE_NVGPU) {
			cudaError_t result;
			result = cudaEventCreateWithFlags(&ev->start_event_dev, cudaEventBlockingSync);
			devcall_assert(result);
			result = cudaEventCreateWithFlags(&ev->stop_event_dev, cudaEventBlockingSync);
			devcall_assert(result);
		} else
#endif
		if (devtype == OMP_DEVICE_THSIM || devtype == OMP_DEVICE_HOSTCPU) {
			/* do nothing */
		} else {
			fprintf(stderr, "other type of devices are not yet supported to init this event\n");
			abort();
		}
	}
	//omp_event_print(ev);
}

void omp_event_print(omp_event_t * ev) {
	printf("ev: %X, dev: %X, stream: %X, record method: %d, name: %s, description: %s\n", ev, ev->dev,
			ev->stream, ev->record_method, ev->event_name, ev->event_description);
}

void omp_event_record_start(omp_event_t * ev, omp_dev_stream_t * stream,  const char * event_name, const char * event_msg, ...) {
	if (stream != NULL && stream->dev != ev->dev) {
		fprintf(stderr, "stream and event are not compatible, they are from two different devices: %s, %s\n", stream->dev->name, ev->dev->name);
		abort();
	}
	ev->stream = stream;
	omp_event_record_method_t rm = ev->record_method;
	ev->event_name = event_name;
	va_list l;
	va_start(l, event_msg);
    vsnprintf(ev->event_description, OMP_EVENT_MSG_LENGTH, event_msg, l);
	va_end(l);

	//printf("omp_event_record_start: ev %X name: %s, dev: %X\n", ev, ev->event_name, ev->dev);
	omp_device_type_t devtype = ev->dev->type;

	if (rm == OMP_EVENT_DEV_RECORD || rm == OMP_EVENT_HOST_DEV_RECORD) {
#if defined (DEVICE_NVGPU_SUPPORT)
		if (devtype == OMP_DEVICE_NVGPU) {
			cudaError_t result;
			result = cudaStreamAddCallback(stream->systream.cudaStream, omp_stream_host_timer_callback, &ev->start_time_dev, 0);
			result = cudaEventRecord(ev->start_event_dev, stream->systream.cudaStream);
			devcall_assert(result);
		} else
#endif
		if (devtype == OMP_DEVICE_THSIM || devtype == OMP_DEVICE_HOSTCPU) {
			ev->start_time_dev = read_timer_ms();
		} else {
			fprintf(stderr, "other type of devices are not yet supported to start event recording\n");
		}
	}

	if (rm == OMP_EVENT_HOST_RECORD || rm == OMP_EVENT_HOST_DEV_RECORD) {
		ev->start_time_host = read_timer_ms();
	}
}

void omp_event_record_stop(omp_event_t * ev) {
	omp_dev_stream_t * stream = ev->stream;
	omp_event_record_method_t record_method = ev->record_method;
	omp_device_type_t devtype = ev->dev->type;
	if (record_method == OMP_EVENT_DEV_RECORD || record_method == OMP_EVENT_HOST_DEV_RECORD) {
#if defined (DEVICE_NVGPU_SUPPORT)
		if (devtype == OMP_DEVICE_NVGPU) {
			cudaError_t result;
			result = cudaStreamAddCallback(stream->systream.cudaStream, omp_stream_host_timer_callback, &ev->stop_time_dev, 0);
			result = cudaEventRecord(ev->stop_event_dev, stream->systream.cudaStream);
			devcall_assert(result);
		} else
#endif
		if (devtype == OMP_DEVICE_THSIM || devtype == OMP_DEVICE_HOSTCPU) {
			ev->stop_time_dev = read_timer_ms();

		} else {
			fprintf(stderr, "other type of devices are not yet supported to stop event record\n");
		}
	}

	if (record_method == OMP_EVENT_HOST_RECORD || record_method == OMP_EVENT_HOST_DEV_RECORD) {
		ev->stop_time_host = read_timer_ms();
	}
	ev->recorded = 1;
}

static double omp_event_elapsed_ms_dev(omp_event_t * ev) {
	omp_device_type_t devtype = ev->dev->type;
	float elapsed = -1.0;
	double elapsed1 = -1.0;
#if defined (DEVICE_NVGPU_SUPPORT)
	if (devtype == OMP_DEVICE_NVGPU) {
		elapsed1 = ev->stop_time_dev - ev->start_time_dev;
		cudaError_t result;
		result = cudaEventSynchronize(ev->start_event_dev);
		devcall_assert(result);
		result = cudaEventSynchronize(ev->stop_event_dev);
		devcall_assert(result);
		result = cudaEventElapsedTime(&elapsed, ev->start_event_dev, ev->stop_event_dev);
		devcall_assert(result);
		//printf("timing difference, callback: %f, event: %f\n", elapsed1, elapsed);
	} else
#endif
	if (devtype == OMP_DEVICE_THSIM || devtype == OMP_DEVICE_HOSTCPU) {
		elapsed = ev->stop_time_dev - ev->start_time_dev;
	} else {
		fprintf(stderr, "other type of devices are not yet supported to calculate elapsed\n");
	}
	//printf("dev event: start: %f, stop: %f, elapsed: %f (%f)\n", ev->start_time_dev, ev->stop_time_dev, elapsed, elapsed1);

	return elapsed;
}

static double omp_event_elapsed_ms_host(omp_event_t * ev) {
	double elapsed = ev->stop_time_host - ev->start_time_host;
	//printf("host event: start: %f, stop: %f, elapsed: %f\n", ev->start_time_host, ev->stop_time_host, elapsed);
	return elapsed;
}

/**
 * Computes the elapsed time between two events (in milliseconds with a resolution of around 0.5 microseconds).
 */
void omp_event_elapsed_ms(omp_event_t * ev) {
	if (!ev->recorded) return;
	omp_event_record_method_t record_method = ev->record_method;
	omp_device_type_t devtype = ev->dev->type;
	if (record_method == OMP_EVENT_DEV_RECORD || record_method == OMP_EVENT_HOST_DEV_RECORD) {
		ev->elapsed_dev = omp_event_elapsed_ms_dev(ev);
	}
	if (record_method == OMP_EVENT_HOST_RECORD || record_method == OMP_EVENT_HOST_DEV_RECORD) {
		ev->elapsed_host = omp_event_elapsed_ms_host(ev);
	}
	ev->recorded = 0;
}

void omp_event_accumulate_elapsed_ms(omp_event_t * ev) {
	if (!ev->recorded) return;
	omp_event_record_method_t record_method = ev->record_method;
	omp_device_type_t devtype = ev->dev->type;
	if (record_method == OMP_EVENT_DEV_RECORD || record_method == OMP_EVENT_HOST_DEV_RECORD) {
		ev->elapsed_dev += omp_event_elapsed_ms_dev(ev);
	}
	if (record_method == OMP_EVENT_HOST_RECORD || record_method == OMP_EVENT_HOST_DEV_RECORD) {
		ev->elapsed_host += omp_event_elapsed_ms_host(ev);
	}
	ev->count++;
	ev->recorded = 0;
}

int omp_get_max_threads_per_team(omp_device_t * dev) {
	omp_device_type_t devtype = dev->type;
#if defined (DEVICE_NVGPU_SUPPORT)
	if (devtype == OMP_DEVICE_NVGPU) {
		return 	((struct cudaDeviceProp*)dev->dev_properties)->maxThreadsPerBlock;
	} else
#endif
	if (devtype == OMP_DEVICE_THSIM || devtype == OMP_DEVICE_HOSTCPU) {
		return dev->num_cores;
	} else {
	}
	return 0;
}

int omp_get_optimal_threads_per_team(omp_device_t * dev) {
	int max = omp_get_max_threads_per_team(dev);
	if (max == 1) return 1;
	else return max/4;
}

/**
 * so far we only do 1D, the first dimension
 */
int omp_get_max_teams_per_league(omp_device_t * dev) {
	omp_device_type_t devtype = dev->type;
#if defined (DEVICE_NVGPU_SUPPORT)
	if (devtype == OMP_DEVICE_NVGPU) {
		return 	((struct cudaDeviceProp*)dev->dev_properties)->maxGridSize[0];
	} else
#endif
	if (devtype == OMP_DEVICE_THSIM || devtype == OMP_DEVICE_HOSTCPU) {
		return 1;
	} else {
	}
	return 0;
}

int omp_get_optimal_teams_per_league(omp_device_t * dev, int threads_per_team, int total) {
	int teams_per_league = (total + threads_per_team - 1) / threads_per_team;
	int max_teams_per_league = omp_get_max_teams_per_league(dev);
	if (teams_per_league > max_teams_per_league) return max_teams_per_league;
	else return teams_per_league;
}
