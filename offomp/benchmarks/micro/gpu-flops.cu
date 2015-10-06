#include<cuda.h>
#include <unistd.h>
#include <math.h>
#include <stdio.h>

/* this two numbers should be the same */
int ops_host = 1000000;
__device__ int ops = 1000000;
__device__ float d_flops;
__global__ void Add_Mul() {
    // need to initialise differently otherwise compiler might optimise away

    double add = M_PI;
    double mul = 1.0 + 1e-8;

    double sum1 = 0.1, sum2 = -0.1, sum3 = 0.2, sum4 = -0.2, sum5 = 0.0;
    double mul1 = 1.0, mul2 = 1.1, mul3 = 1.2, mul4 = 1.3, mul5 = 1.4;
    int loops = ops / 10;    // we have 10 floating point ops inside the loop
    double expected = 5.0 * add * loops + (sum1 + sum2 + sum3 + sum4 + sum5)
                      + pow(mul, loops) * (mul1 + mul2 + mul3 + mul4 + mul5);

    int i;
    for (i = 0; i < loops; i++) {
        mul1 *= mul;
        mul2 *= mul;
        mul3 *= mul;
        mul4 *= mul;
        mul5 *= mul;
        sum1 += add;
        sum2 += add;
        sum3 += add;
        sum4 += add;
        sum5 += add;
    }

    d_flops =
            sum1 + sum2 + sum3 + sum4 + sum5 + mul1 + mul2 + mul3 + mul4 + mul5 -
            expected;
}


int
main(int argc, char *argv[]) {

    //int n = 1000000;
    //add 20 iterations 
    //device control modifier
    if (argc < 2) {
        fprintf(stderr, "Usage: ./a.out <device_number> eg:./a.out 1 \n");
        exit(1);
    }
    int N = atoi(argv[1]);
    if (N > 3) {
        printf("Please enter a valid device number\n");
        return (0);
    }

    cudaSetDevice(N);

    int i;
    cudaEvent_t start;
    cudaEventCreate(&start);
    cudaEvent_t stop;
    cudaEventCreate(&stop);
    //sample run
//    Add_Mul << < 128 * 128, 512 >> > ();

    float msecTotal = 0.0f;
    cudaEventRecord(start, NULL);
    int num_its = 1;
    for (i = 0; i < num_its; i++) {
    Add_Mul << < 128 * 128, 512 >> > ();
    }
    cudaEventRecord(stop, NULL);
    cudaEventSynchronize(stop);
    cudaEventElapsedTime(&msecTotal, start, stop);

    msecTotal = msecTotal / num_its;

    double flops = (ops_host * 512 * 128*128) /msecTotal; // Value was over flowing, so I self-optimized it in order to make it stay within the range.
//1024*32 is added because this will be 1024 threads in each of the 32 blocks working in parallel to compute the result. Hence GFlops is related to how many threads are made to work.

    printf("GPU %d performance is %f MFLOPs/s\n", N, flops);
}

