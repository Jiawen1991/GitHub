Sample: Bandwidth Test
Minimum spec: SM 1.1

This is a simple test program to measure the memcopy bandwidth of the GPU and memcpy bandwidth across PCI-e.
This test application is capable of measuring device to device copy bandwidth, host to device copy bandwidth 
for pageable and page-locked memory, and device to host copy bandwidth for pageable and page-locked memory.

The source code use headers from ../../NVIDIA_CUDA-6.5_Samples_common/inc in the repo and 
you will also need to set CUDA_PATH env to point the nvcc folder. 

"make" will build bandwidthTest executable and "make -f Makefile-parallel" will build the bandwidthTest-parallel 
executable. Launch the executable with options that can be listed using "-help" option to the executable. 
When run the bandwidthTest-parallel, use "-device=all" so it will launch multiple threads and test the bandwidth in paralle, e.g.
bandwidthTest-parallel -device=all

By default, it will measure the bandwidth of transfer 32MBytes of data. If you
want to produce a full report for different data sizes and latency info, pass the argument for
mode and enable csv output, e.g.:  
./bandwidthTest --mode=shmoo --csv
