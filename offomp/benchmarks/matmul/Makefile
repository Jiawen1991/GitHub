TEST_INCLUDES = -I../../runtime -I.
TEST_LINK = -L/usr/lib/gcc/x86_64-redhat-linux/4.4.6 -lm -lrt -lgomp -lpthread
TEST_LINK = -L/usr/lib64/gcc/x86_64-suse-linux/4.8 -lm -lrt -lgomp -lpthread

#NVGPU_CUDA_PATH=/opt/NVIDIA/cuda/
NVGPU_CUDA_PATH=/usr/local/cuda-5.5/
NVGPU_CUDA_PATH=/APPS/cuda/include

default: matmul-thsim
all: matmul-thsim matmul-nvgpu matmul-nvgpu-vsharedm

matmul-thsim:	
#	gcc $(TEST_INCLUDES) -DOMP_BREAKDOWN_TIMING -g ../../runtime/homp.c ../../runtime/homp_dev.c ../../runtime/dev_xthread.c ../../util/iniparser.c ../../util/dictionary.c matmul_mdev.c -c
	gcc $(TEST_INCLUDES) -g -fopenmp -DOMP_BREAKDOWN_TIMING=1 -DPROFILE_PLOT=1 ../../runtime/homp.c ../../runtime/homp_dev.c ../../runtime/dev_xthread.c ../../util/iniparser.c ../../util/dictionary.c  matmul_mdev.c -c
	gcc $(TEST_INCLUDES) -g *.o -o $@ ${TEST_LINK}

matmul-nvgpu:
	nvcc $(TEST_INCLUDES) -I${NVGPU_CUDA_PATH}/include -Xcompiler -fopenmp -DDEVICE_NVGPU_SUPPORT=1 -DOMP_BREAKDOWN_TIMING=1 -DPROFILE_PLOT=1 ../../runtime/homp.c ../../runtime/homp_dev.c ../../runtime/dev_xthread.c ../../util/iniparser.c ../../util/dictionary.c  -c
	nvcc -I../../runtime -I. -O2 -I/APPS/cuda/include/include -Xcompiler -fopenmp -Xcompiler -O2 -DDEVICE_NVGPU_SUPPORT=1 -DOMP_BREAKDOWN_TIMING=1 -DPROFILE_PLOT=1 matmul_mdev.cu  -c
	nvcc $(TEST_INCLUDES) -g *.o -o $@ ${TEST_LINK}

matmul-nvgpu-vsharedm:
	nvcc $(TEST_INCLUDES) -I${NVGPU_CUDA_PATH}/include -Xcompiler -fopenmp -DDEVICE_NVGPU_SUPPORT=1 -DDEVICE_NVGPU_VSHAREDM=1  -DOMP_BREAKDOWN_TIMING=1 -DPROFILE_PLOT=1 ../../runtime/homp.c ../../runtime/homp_dev.c ../../runtime/dev_xthread.c ../../util/iniparser.c ../../util/dictionary.c  -c
	nvcc -I../../runtime -I. -O2 -I/APPS/cuda/include/include -Xcompiler -fopenmp -Xcompiler -O2 -DDEVICE_NVGPU_SUPPORT=1 -DDEVICE_NVGPU_VSHAREDM=1  -DOMP_BREAKDOWN_TIMING=1 -DPROFILE_PLOT=1 matmul_mdev.cu  -c
	nvcc $(TEST_INCLUDES) -g *.o -o $@ ${TEST_LINK}

clean:
	rm -rf *.o matmul-* core *.plot
