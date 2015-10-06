/*Original source: 
  https://github.com/rasmusto/CUDA/blob/master/john_jacobi/jacobi.cu
  There are even better versions there. TODO:
  https://github.com/rasmusto/CUDA/blob/master/jacobi_final/1k_jacobi6.cu
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <cuda.h>
#include <cuda_runtime.h>

__global__ void
jacobikernel( float* a, float* newa, float* lchange, int n, int m, float w0, float w1, float w2 )
{        
    int ti = threadIdx.x;
    int tj = threadIdx.y;
    int i = blockIdx.x * blockDim.x + ti + 1;
    int j = blockIdx.y * blockDim.y + tj + 2;
///* looks like arrays are linearized, so explicit address calculation is used to simulate 2-D access */
//// I think the linearization is wrong: it should be a[i*m + j -1] instead of a[j*m+i-1]
    if((i < m-1) && (j < n))
    {
    newa[j*m+i] = w0*a[j*m+i] +
            w1 * (a[j*m+i-1] + a[(j-1)*m+i] +
                  a[j*m+i+1] + a[(j+1)*m+i]) +
            w2 * (a[(j-1)*m+i-1] + a[(j+1)*m+i-1] +
                  a[(j-1)*m+i+1] + a[(j+1)*m+i+1]);
    }

    __shared__ float mychange[256]; /* fixed 16x16 2-D thread block: each thread stores its own change into mychange[] */
// convert 2-D (ti,tj) into a 1-D ii , so 2-D block's computation results can fit into a 1-D array mychange[]
// Again, correct conversion should: ti*blockDim.y + tj
    int ii = ti+blockDim.x*tj;
    mychange[ii] = fabsf( newa[j*m+i] - a[j*m+i] );
    __syncthreads();
    // contiguous range reduction pattern: half folding and add
    int nn = blockDim.x * blockDim.y; // total number of threads within the current thread block
    while( (nn>>=1) > 0 ){
        if( ii < nn )
            mychange[ii] = fmaxf( mychange[ii], mychange[ii+nn] );
        __syncthreads();
    }
    if( ii == 0 ) // thread (0,0) writes the block level reduction result (mychange) to grid level result variable: lchange[]
        lchange[blockIdx.x + gridDim.x*blockIdx.y] = mychange[0];
}

__global__ void
reductionkernel( float* lchange, int n ) /* lchange[n] stores the local reduction results of each thread block*/
{
    __shared__ float mychange[256]; /*Again, store lchange[] content into a shared memory buffer. 256 = 16x16, a 1-D thread block*/
    float mych = 0.0f;
    int ii = threadIdx.x, m;
    if( ii < n ) 
      mych = lchange[ii]; /* transfer to a register variable i when thread id < n*/
    m = blockDim.x; /* total number of threads of this 1-D thread block*/
    while( m <= n ){ /*handle the case when n > number of threads:  fold them ?*/
        mych = fmaxf( mych, lchange[ii+m] );
        m += blockDim.x;
    }
    mychange[ii] = mych; /*now all elements are reduced into mych*/
    __syncthreads();
    int nn = blockDim.x; /*consider only the exact number of thread number of reduction variables*/
    while( (nn>>=1) > 0 ){
        if( ii < nn )
            mychange[ii] = fmaxf(mychange[ii],mychange[ii+nn]);
        __syncthreads();
    }
    if( ii == 0 )
        lchange[0] = mychange[0];
}

static float sumtime;

extern "C" void initDevice(float *a, int n, int m, float** da, float** dnewa, float** lchange , int Thr, int rank)
{
    size_t memsize;
    int bx, by, gx, gy;
    int devCount,devID;
    cudaError_t cudareturn;
    cudaGetDeviceCount(&devCount);

//    if(rank >= devCount)
//    {
//      printf("not enough devices!\n");
//      exit(-1);
//    }
//    cudareturn = cudaSetDevice(rank+1);
//    if (cudareturn == cudaErrorInvalidDevice) 
//    { 
//        perror("cudaSetDevice returned  cudaErrorInvalidDevice"); 
//        exit(-1);
//    }else 
//        cudaGetDevice(&devID);
    bx = Thr;
    by = Thr;
    gx = (n-2)/bx + ((n-2)%bx == 0?0:1);
    gy = (m-2)/by + ((m-2)%by == 0?0:1);
    memsize = sizeof(float) * (n+2) * m;
    cudaMalloc( da, memsize );
    cudaMalloc( dnewa, memsize );
    cudaMalloc( lchange, gx * gy * sizeof(float) );
    cudaMemcpy( *da, a, memsize, cudaMemcpyHostToDevice );
    cudaMemcpy( *dnewa, a, memsize, cudaMemcpyHostToDevice );
}

extern "C" void dumpFile(float *a, int rank, int m, int n, char* prefix)
{
    FILE *initfile;
    char name[15];
    char buffer[5];
    sprintf(buffer,"%d",rank);
    strcpy(name, prefix);
    strcat(name, buffer);
    strcat(name, ".txt");
    initfile = fopen(name,"w+");
    for(int idxi = 0; idxi <= n+1; ++idxi)
    {
     fprintf(initfile, "row id = %d\n",idxi);
    for(int idxj = 0; idxj < m; ++idxj)
    {
     fprintf(initfile," %f ", a[idxj + idxi*m]);
    }
     fprintf(initfile, "\n",idxi);
    }
    fclose(initfile);
}

extern "C" void JacobiGPU( float* a, int n, int m, float w0, float w1, float w2, float tol, int Thr, float *da, float *dnewa, float *lchange)
{
    float change;
    int bx, by, gx, gy;
    cudaEvent_t e1, e2;

    int devID;
    cudaGetDevice(&devID);

    bx = Thr;
    by = Thr;
    gx = (m-2)/bx + ((m-2)%bx == 0?0:1);
    gy = (n-2)/by + ((n-2)%by == 0?0:1);
//        printf("\nGrids =  %i and %i\n", gx, gy);
//    sumtime = 0.0f;
//    cudaEventCreate( &e1 );
//    cudaEventCreate( &e2 );

    dim3 block( bx, by );
    dim3 grid( gx, gy );
    cudaMemcpy( da, a, sizeof(float) * m, cudaMemcpyHostToDevice );
    cudaMemcpy( (da+(n+1)*m), (a+(n+1)*m), sizeof(float) * m, cudaMemcpyHostToDevice );

    float msec;

//    cudaEventRecord( e1 );
    jacobikernel<<< grid, block >>>( da, dnewa, lchange, n, m, w0, w1, w2 );
//    reductionkernel<<< 1, bx*by >>>( lchange, gx*gy ); /* both levels of reduction happen on GPU */
//    cudaEventRecord( e2 );

//    cudaMemcpy( &change, lchange, sizeof(float), cudaMemcpyDeviceToHost ); /* copy final reduction result to CPU */
//    cudaEventElapsedTime( &msec, e1, e2 );
//    sumtime += msec;
//    cudaMemcpy( (a+m), (dnewa+m), sizeof(float) * m, cudaMemcpyDeviceToHost );
//    cudaMemcpy( (a+n*m), (dnewa+n*m), sizeof(float) * m, cudaMemcpyDeviceToHost );

//    cudaEventDestroy( e1 );
//    cudaEventDestroy( e2 );
}

extern "C" void cleanDevice(float *a, float *da, float *dnewa, float *lchange,int m, int n)
{
    size_t memsize;
    memsize = sizeof(float) * (n+2) * m;
    cudaMemcpy( a, da, memsize, cudaMemcpyDeviceToHost );
    cudaFree( da );
    cudaFree( dnewa );
    cudaFree( lchange );

}
