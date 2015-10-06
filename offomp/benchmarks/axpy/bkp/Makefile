GOMP_PATH=/usr/lib/gcc/x86_64-linux-gnu/4.4.7/
# on shark node (crill-102)
# GOMP_PATH=/opt/gnu/gcc/4.4.6/lib64/
#GOMP_PATH=/usr/lib/gcc/x86_64-linux-gnu/4.8.2/
GOMP_PATH=/usr/lib/gcc/x86_64-redhat-linux/4.4.6
# on eureka
#GOMP_PATH=/opt/local/gcc/4.4.7/lib64/

TEST_INCLUDES = -I../runtime -I../ompLowering/ -I.
TEST_LINK = -L../ompLowering ../ompLowering/xomp.o $(GOMP_PATH)/libgomp.a -lpthread -lm

axpy:
#	${ROSE_OMPTEST}/roseompacc ${TEST_INCLUDES} ${TEST_FLAGS} -rose:skipfinalCompileStep axpy_ompacc.c
#	${ROSE_OMPTEST}/roseomp ${TEST_INCLUDES} ${TEST_FLAGS} -rose:skipfinalCompileStep axpy_omp.c
#	${ROSE_OMPTEST}/roseomp ${TEST_INCLUDES} ${TEST_FLAGS} -rose:skipfinalCompileStep axpy.c
#	nvcc $(TEST_INCLUDES) ${TEST_LINK}  -DUSE_ROSE_GOMP_OPENMP_LIBRARY ../runtime/homp.c $(ROSE_XOMPSRC)/xomp_cuda_lib.cu rose_axpy_omp.c rose_axpy_ompacc.cu rose_axpy.c timer.c -c
#	nvcc $(TEST_INCLUDES) ${TEST_LINK} -arch sm_20 -DUSE_ROSE_GOMP_OPENMP_LIBRARY ../runtime/homp.c ../runtime/homp_dev.c rose_axpy_omp.c rose_axpy_ompacc.cu rose_axpy.c -c
	nvcc $(TEST_INCLUDES) -DDEVICE_NVGPU_SUPPORT ../runtime/homp.c ../runtime/homp_dev.c rose_axpy_omp.c rose_axpy_ompacc.cu rose_axpy.c -c
	nvcc $(TEST_INCLUDES) *.o -o $@ ${TEST_LINK}

clean:
	rm -rf *.o axpy

