#cmake_minimum_required(VERSION 3.1)
cmake_minimum_required(VERSION 2.8)
project(offomp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(RUNTIME_SOURCE_FILES
    runtime/dev_xthread.c
    runtime/homp.c
    runtime/homp.h
    runtime/homp_dev.c
    util/dictionary.c
    util/dictionary.h
    util/iniparser.c
    util/iniparser.h)

set(AXPY_SOURCE_FILES
    benchmarks/axpy/axpy.c
    benchmarks/axpy/axpy.h
    benchmarks/axpy/axpy_ompacc.c)

set(MATVEC_SOURCE_FILES
    benchmarks/matvec/matvec.c
    benchmarks/matvec/matvec.h
    benchmarks/matvec/matvec_ompacc.c)

set(MATMUL_SOURCE_FILES
    benchmarks/matmul/matmul_mdev.c)

set(RADIX_SOURCE_FILES
    benchmarks/radixsort/radix.c)

set(STENCIL2D_SOURCE_FILES
    benchmarks/stencil2d/stencil2d.h)
    benchmarks/stencil2d/stencil2d.c)
    benchmarks/stencil2d/nvgpu_kernels.c)
    benchmarks/stencil2d/kernel_launcher.c)

set(JACOBI_SOURCE_FILES
    benchmarks/jacobi/jacobi_ompacc_mdev.c)

include_directories("runtime")

add_executable(axpy-thsim ${RUNTIME_SOURCE_FILES} ${AXPY_SOURCE_FILES})
add_executable(matvec-thsim ${RUNTIME_SOURCE_FILES} ${MATVEC_SOURCE_FILES})
add_executable(matmul-thsim ${RUNTIME_SOURCE_FILES} ${MATMUL_SOURCE_FILES})
add_executable(radix-thsim ${RUNTIME_SOURCE_FILES} ${RADIX_SOURCE_FILES})
add_executable(stencil2d-thsim ${RUNTIME_SOURCE_FILES} ${STENCIL2D_SOURCE_FILES} benchmarks/stencil2d/uoldcopy.c benchmarks/stencil2d/stencil2d.h)
add_executable(jacobi-thsim ${RUNTIME_SOURCE_FILES} ${JACOBI_SOURCE_FILES})
target_link_libraries(axpy-thsim pthread m)
target_link_libraries(matvec-thsim pthread m)
target_link_libraries(matmul-thsim pthread m)
target_link_libraries(radix-thsim pthread m)
target_link_libraries(stencil2d-thsim pthread m)
target_link_libraries(jacobi-thsim pthread m)

