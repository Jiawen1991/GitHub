TEST_INCLUDES = -I../../runtime -I. -w
TEST_LINK = -L/usr/lib/gcc/x86_64-redhat-linux/4.4.6 -lm -lrt -lgomp -lpthread
TEST_LINK = -L/usr/lib64/gcc/x86_64-suse-linux/4.8 -lm -lrt -lgomp -lpthread

NVGPU_CUDA_PATH=/opt/NVIDIA/cuda/
NVGPU_CUDA_PATH=/APPS/cuda/include

default: jacobi-thsim
all: jacobi-thsim jacobi-nvgpu jacobi-nvgpu-vsharedm

jacobi-thsim:	
	gcc $(TEST_INCLUDES) -DOMP_BREAKDOWN_TIMING -DPROFILE_PLOT=1 -g -O0 ../../runtime/homp.c ../../runtime/homp_dev.c ../../runtime/dev_xthread.c ../../util/iniparser.c ../../util/dictionary.c   jacobi_ompacc_mdev.c -c
	#gcc $(TEST_INCLUDES) -g ../../runtime/homp.c ../../runtime/homp_dev.c ../../runtime/dev_xthread.c ../../util/iniparser.c ../../util/dictionary.c   jacobi_ompacc_mdev.c -c
	gcc $(TEST_INCLUDES) -g *.o -o $@ ${TEST_LINK}

jacobi-nvgpu:
	nvcc $(TEST_INCLUDES) -g -I${NVGPU_CUDA_PATH}/include -Xcompiler -fopenmp -DDEVICE_NVGPU_SUPPORT=1 -DOMP_BREAKDOWN_TIMING -DPROFILE_PLOT=1 ../../runtime/homp.c ../../runtime/homp_dev.c ../../runtime/dev_xthread.c ../../util/iniparser.c ../../util/dictionary.c  jacobi_ompacc_mdev.cu  -c
#	nvcc $(TEST_INCLUDES) -O2 -g -I${NVGPU_CUDA_PATH}/include -Xcompiler -fopenmp -Xcompiler -O2 -DDEVICE_NVGPU_SUPPORT=1 -DOMP_BREAKDOWN_TIMING -DPROFILE_PLOT=1  jacobi_ompacc_mdev.cu  -c
	nvcc $(TEST_INCLUDES) -g *.o -o $@ ${TEST_LINK}

jacobi-nvgpu-vsharedm:
	nvcc $(TEST_INCLUDES) -g -I${NVGPU_CUDA_PATH}/include -Xcompiler -fopenmp -DDEVICE_NVGPU_SUPPORT=1 -DDEVICE_NVGPU_VSHAREDM=1 -DOMP_BREAKDOWN_TIMING -DPROFILE_PLOT=1 ../../runtime/homp.c ../../runtime/homp_dev.c ../../runtime/dev_xthread.c ../../util/iniparser.c ../../util/dictionary.c  jacobi_ompacc_mdev.cu  -c
	nvcc $(TEST_INCLUDES) -g *.o -o $@ ${TEST_LINK}

clean:
	rm -rf *.o jacobi-* *.plot core
