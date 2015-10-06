TEST_INCLUDES = -I../../runtime -I../../util -I. 
#crill-102
TEST_LINK = -L/usr/lib/gcc/x86_64-linux-gnu/4.8.2 -lgomp -lm -lrt -lpthread
#gpu.sec
TEST_LINK = -L/usr/lib/gcc/x86_64-redhat-linux/4.4.6 -lgomp -lm -lrt -lpthread

NVGPU_CUDA_PATH=/usr/local/cuda-5.5/

#crill-102
NVGPU_CUDA_PATH=/opt/NVIDIA/cuda/

#gpu.secs
NVGPU_CUDA_PATH=/APPS/cuda

default: axpy-thsim
all: axpy-thsim axpy-nvgpu axpy-nvgpu-vsharedm

axpy-thsim:
	gcc $(TEST_INCLUDES) -g -DOMP_BREAKDOWN_TIMING -DPROFILE_PLOT=1 -fopenmp ../../runtime/homp.c ../../runtime/homp_dev.c ../../runtime/dev_xthread.c ../../runtime/dev_xthread.c ../../util/iniparser.c ../../util/dictionary.c axpy_ompacc.c axpy.c -c
#	gcc $(TEST_INCLUDES) -g ../../runtime/homp.c ../../runtime/homp_dev.c ../../runtime/dev_xthread.c ../../util/iniparser.c ../../util/dictionary.c axpy_ompacc.c axpy.c -c
	gcc $(TEST_INCLUDES) -g *.o -o $@ ${TEST_LINK}

axpy-nvgpu:
	nvcc $(TEST_INCLUDES) -g -I${NVGPU_CUDA_PATH}/include -Xcompiler -fopenmp -DDEVICE_NVGPU_SUPPORT=1 -DOMP_BREAKDOWN_TIMING -DPROFILE_PLOT=1 ../../runtime/homp.c ../../runtime/homp_dev.c ../../runtime/dev_xthread.c ../../util/iniparser.c ../../util/dictionary.c  axpy_ompacc.cu axpy.c -c
	nvcc $(TEST_INCLUDES) -g *.o -o $@ ${TEST_LINK}

axpy-nvgpu-vsharedm:
	nvcc $(TEST_INCLUDES) -g -I${NVGPU_CUDA_PATH}/include -Xcompiler -fopenmp -DDEVICE_NVGPU_SUPPORT=1 -DDEVICE_NVGPU_VSHAREDM=1 -DOMP_BREAKDOWN_TIMING -DPROFILE_PLOT=1 ../../runtime/homp.c ../../runtime/homp_dev.c ../../runtime/dev_xthread.c ../../util/iniparser.c ../../util/dictionary.c  axpy_ompacc.cu axpy.c -c
	nvcc $(TEST_INCLUDES) -g *.o -o $@ ${TEST_LINK}

clean:
	rm -rf *.o axpy-* *.plot core

