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

axpy:
#	${ROSE_OMPTEST}/roseompacc ${TEST_INCLUDES} ${TEST_FLAGS} -rose:skipfinalCompileStep axpy_ompacc.c
#	${ROSE_OMPTEST}/roseomp ${TEST_INCLUDES} ${TEST_FLAGS} -rose:skipfinalCompileStep axpy_omp.c
#	${ROSE_OMPTEST}/roseomp ${TEST_INCLUDES} ${TEST_FLAGS} -rose:skipfinalCompileStep axpy.c
#	nvcc $(TEST_INCLUDES) ${TEST_LINK}  -DUSE_ROSE_GOMP_OPENMP_LIBRARY ../runtime/homp.c $(ROSE_XOMPSRC)/xomp_cuda_lib.cu rose_axpy_omp.c rose_axpy_ompacc.cu rose_axpy.c timer.c -c
	nvcc $(TEST_INCLUDES) ${TEST_LINK} -arch sm_20 -g -DUSE_ROSE_GOMP_OPENMP_LIBRARY ../runtime/homp.c rose_axpy_omp.c rose_axpy_ompacc.cu rose_axpy.c ../runtime/timer.c -c
	nvcc $(TEST_INCLUDES) ${TEST_LINK}  *.o -o $@

clean:
	rm -rf *.o axpy

