The folder contains simple microbenchmarks to collect stressed
performance of a CPU core and a GPU. The stressed performance
is not the peak performance (less than), but equal to or higher than
real application performance. It is obtainable in real application or
benchmarks after applying crazy optimization techniques. 

For CPU per core performance, use cpu-flops.cu 
gcc -O3 cpu-flops.c -o stresscpu -lm -lrt
./stresscpu

For GPU performance, use gpu-flops.cu
nvcc gpu-flops.cu -o stressgpu
./stressgpu <devi id>

For NVGPU device bandwidth and latency test, check the bandwidthTest folder
