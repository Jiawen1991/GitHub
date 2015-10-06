CUDA_INSTALL_PATH := /opt/cuda/4.2/cuda
CUDA_INCLUDES := -I. -I${CUDA_INSTALL_PATH}/include 
CUDA_LINK := -L${CUDA_INSTALL_PATH}/lib64 -lcuda -lcudart -lgomp 
NVCC = ${CUDA_INSTALL_PATH}/bin/nvcc -m64 -O3 -gencode arch=compute_20,code=sm_20 -Xptxas -v --maxrregcount 63 -Xcompiler -mcmodel=medium 
CC=mpicc
CFLAGS=
DEPS = 

all:	jacobi

jacobi: jacobi.cu
	$(NVCC) ${CUDA_LINK} -o jacobi jacobi.cu 

clean:
	rm jacobi *.o 
