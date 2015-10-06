NVGPU_CUDA_PATH=~/../../usr/local/cuda-7.0
NVGPU_CUDA_PATH=/usr/local/cuda-5.5/
NVGPU_CUDA_PATH=/APPS/cuda/include
NVGPU_CUDA_PATH=/opt/NVIDIA/cuda/

GNU_GCCLIB_PATH=/usr/lib/gcc/x86_64-linux-gnu/4.8.2

CFLAGS += -I../../runtime -I../../util -I. -w
LDFLAGS += -L${GNU_GCCLIB_PATH} -lgomp -lm -lrt -lpthread

default: stencil2d-thsim

all: stencil2d-thsim stencil2d-nvgpu stencil2d-nvgpu-vsharedm

stencil2d-thsim:
	gcc $(CFLAGS) -g -O0 -Wall -DOMP_BREAKDOWN_TIMING -DPROFILE_PLOT=1 -fopenmp ../../runtime/homp.c ../../runtime/homp_dev.c ../../runtime/dev_xthread.c ../../runtime/dev_xthread.c ../../util/iniparser.c ../../util/dictionary.c stencil2d.c kernel_launcher.c -c
	gcc $(CFLAGS) *.o -o $@ ${LDFLAGS}

stencil2d-nvgpu:
	nvcc $(CFLAGS) -g -O0 -I${NVGPU_CUDA_PATH}/include -Xcompiler -fopenmp -DDEVICE_NVGPU_SUPPORT=1 -DOMP_BREAKDOWN_TIMING -DPROFILE_PLOT=1 ../../runtime/homp.c ../../runtime/homp_dev.c ../../runtime/dev_xthread.c ../../util/iniparser.c ../../util/dictionary.c  stencil2d.c kernel_launcher.cu nvgpu_kernels.cu -c
	nvcc $(CFLAGS) *.o -o $@ ${LDFLAGS}

stencil2d-nvgpu-vsharedm:
	nvcc $(CFLAGS) -g -O0 -I${NVGPU_CUDA_PATH}/include -Xcompiler -fopenmp -DDEVICE_NVGPU_SUPPORT=1 -DDEVICE_NVGPU_VSHAREDM=1 -DOMP_BREAKDOWN_TIMING -DPROFILE_PLOT=1 ../../runtime/homp.c ../../runtime/homp_dev.c ../../runtime/dev_xthread.c ../../util/iniparser.c ../../util/dictionary.c  stencil2d.c kernel_launcher.cu nvgpu_kernels.cu -c
	nvcc $(CFLAGS) *.o -o $@ ${LDFLAGS}

clean:
	rm -rf *.o stencil2d-* *.plot core

