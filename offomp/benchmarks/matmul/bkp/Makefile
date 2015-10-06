# the translator should use: https://bitbucket.org/ChunhuaLiao/rose-omp-accelerator
# just build librose.so under buildtree/src, and then build the translator roseompacc under tests/roseTests/ompLoweringTests/
# The translator source is rose-omp-accelerator/tests/roseTests/ompLoweringTests/roseompacc.C
# The command lines (two steps for now) to use roseompacc is exemplified in
# rose-omp-accelerator/tests/roseTests/ompLoweringTests/Makefile.am
ROSE_DIR=/home/yy8/ROSE/rexc
# on shark node (crill-102)
ROSE_DIR=/data/yy8/ROSE/edg4x-rose
top_srcdir=${ROSE_DIR}
top_builddir=$(ROSE_DIR)-build
GOMP_PATH=/usr/lib/gcc/x86_64-linux-gnu/4.4.7/
# on shark node (crill-102)
GOMP_PATH=/opt/gnu/gcc/4.4.6/lib64/
ROSE_OMPTEST=${top_builddir}/tests/roseTests/ompLoweringTests
ROSE_XOMPSRC=$(top_srcdir)/src/midend/programTransformation/ompLowering

TEST_INCLUDES = -I../runtime -I$(top_srcdir)/src/frontend/SageIII -I$(ROSE_XOMPSRC) -I.
TEST_FLAGS=-rose:openmp:lowering -g --edg:no_warnings -DUSE_ROSE_GOMP_OPENMP_LIBRARY
TEST_LINK = -L$(top_builddir)/src/midend/.libs -lxomp $(GOMP_PATH)/libgomp.a -lpthread -lm

matmul:
#	${ROSE_OMPTEST}/roseompacc ${TEST_INCLUDES} ${TEST_FLAGS} -rose:skipfinalCompileStep matmul.c
#	${ROSE_OMPTEST}/roseomp ${TEST_INCLUDES} ${TEST_FLAGS} -rose:skipfinalCompileStep matmul_omp.c
#	${ROSE_OMPTEST}/roseomp ${TEST_INCLUDES} ${TEST_FLAGS} -rose:skipfinalCompileStep matmul.c
#	nvcc $(TEST_INCLUDES) ${TEST_LINK} -arch sm_20 -DUSE_ROSE_GOMP_OPENMP_LIBRARY ../runtime/homp.c rose_matmul_mdev.cu ../runtime/timer.c -c
	nvcc $(TEST_INCLUDES) ${TEST_LINK} -arch sm_20 -g -DDEBUG_MSG=1 -DUSE_ROSE_GOMP_OPENMP_LIBRARY ../runtime/homp.c rose_matmul_mdev.cu ../runtime/timer.c -c
	nvcc $(TEST_INCLUDES) ${TEST_LINK}  *.o -o $@

matmul_time:
	nvcc $(TEST_INCLUDES) ${TEST_LINK}  rose_matmul_time.cu ${XOMP_RUNTIME_PATH}/xomp_cuda_lib.cu -o $@

openacc:
	pgcc -DOPENACC=1 -ta=nvidia -Minfo=accel -O3 -o matmul_acc_pgi matmul.c
#	pgcc -DOPENACC=1 -ta=nvidia,nofma,cc20 -Minfo=accel -O3 -o matmul_acc_pgi matmul.c
	hmpp gcc -fopenmp -DOPENACC=1 matmul.c -o matmul_acc_hmpp -O3

clean:
	rm -rf *.o matmul

