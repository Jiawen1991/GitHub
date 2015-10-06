/*Original source: 
  https://github.com/rasmusto/CUDA/blob/master/john_jacobi/jacobi.cu
  There are even better versions there. TODO:
  https://github.com/rasmusto/CUDA/blob/master/jacobi_final/1k_jacobi6.cu
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <omp.h>

#define gpuErrchk(ans) { gpuAssert((ans), __FILE__, __LINE__); }
inline void gpuAssert(cudaError_t code, char *file, int line, bool abort=true)
{
   if (code != cudaSuccess) 
   {
      fprintf(stderr,"GPUassert: %s %s %d\n", cudaGetErrorString(code), file, line);
      if (abort) exit(code);
   }
}
__global__ void
jacobikernel( float* a, float* newa, float* lchange, int n, int m, float w0, float w1, float w2 )
{        
    int ti = threadIdx.x;
    int tj = threadIdx.y;
    int i = blockIdx.x * blockDim.x + ti + 1;
    int j = blockIdx.y * blockDim.y + tj + 1;
/* looks like arrays are linearized, so explicit address calculation is used to simulate 2-D access */
// I think the linearization is wrong: it should be a[i*m + j -1] instead of a[j*m+i-1]
    if((i < m-1) && (j < n-1))
    {
    newa[j*m+i] = w0*a[j*m+i] +
            w1 * (a[j*m+i-1] + a[(j-1)*m+i] +
                  a[j*m+i+1] + a[(j+1)*m+i]) +
            w2 * (a[(j-1)*m+i-1] + a[(j+1)*m+i-1] +
                  a[(j-1)*m+i+1] + a[(j+1)*m+i+1]);
    }
//    __shared__ float mychange[256]; /* fixed 16x16 2-D thread block: each thread stores its own change into mychange[] */
//// convert 2-D (ti,tj) into a 1-D ii , so 2-D block's computation results can fit into a 1-D array mychange[]
//// Again, correct conversion should: ti*blockDim.y + tj
//    int ii = ti+blockDim.x*tj;
//    mychange[ii] = fabsf( newa[j*m+i] - a[j*m+i] );
//    __syncthreads();
//    // contiguous range reduction pattern: half folding and add
//    int nn = blockDim.x * blockDim.y; // total number of threads within the current thread block
//    while( (nn>>=1) > 0 ){
//        if( ii < nn )
//            mychange[ii] = fmaxf( mychange[ii], mychange[ii+nn] );
//        __syncthreads();
//    }
//    if( ii == 0 ) // thread (0,0) writes the block level reduction result (mychange) to grid level result variable: lchange[]
//        lchange[blockIdx.x + gridDim.x*blockIdx.y] = mychange[0];
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


void JacobiGPU( float* a, int input_n, int input_m, float w0, float w1, float w2, float tol, int Thr , float** dnewa)
{
    float change;
    int iters;
    size_t memsize;
    size_t offset;
    int bx, by, gx, gy;
    float *da, *lchange;
    cudaEvent_t e1, e2;
    int n,m, devID;  
    int numthread = omp_get_num_threads();
    int tid = omp_get_thread_num();
    int accessflag_l, accessflag_h;
    m = input_m;
    n = input_n / numthread; 
    cudaError_t result;
    result = cudaSetDevice(tid);
    gpuErrchk(result);   
    result = cudaGetDevice(&devID);

    accessflag_l = 0;
    accessflag_h = 0;   
    if(tid > 0)
    {
       cudaDeviceCanAccessPeer(&accessflag_l, (tid-1), tid);
       cudaDeviceEnablePeerAccess(tid-1,0);
    }
    if(tid < numthread-1)
    {
       cudaDeviceCanAccessPeer(&accessflag_h, (tid+1), tid);
       cudaDeviceEnablePeerAccess(tid+1,0);
    }
  
    if(accessflag_l*accessflag_h)
    { 
      memsize = sizeof(float) * (n+2) * m;
    }
    else
    {
      memsize = sizeof(float) * (n+1) * m;
    }

    bx = Thr;
    by = Thr;
    cudaMalloc( &da, memsize );
    cudaMalloc( &dnewa[tid], memsize );
    printf("%d %p\n",tid ,dnewa[tid]);
//    cudaMalloc( &lchange, gx * gy * sizeof(float) );
    printf("I am thread %d will allocate memory %p %d\n",tid,da, memsize); 

    if(tid == 0)
    {
      gx = (n-1)/bx + ((n-1)%bx == 0?0:1);
      gy = (m-2)/by + ((m-2)%by == 0?0:1);
      result = cudaMemcpy( da, a, memsize, cudaMemcpyHostToDevice );
      gpuErrchk(result);
      result = cudaMemcpy( dnewa[tid], a, memsize, cudaMemcpyHostToDevice );
      gpuErrchk(result);
    }
    else if(tid == numthread-1)
    {
      gx = (n-1)/bx + ((n-1)%bx == 0?0:1);
      gy = (m-2)/by + ((m-2)%by == 0?0:1);
      result = cudaMemcpy( da, a+(tid*input_m*input_n/numthread)-m, memsize, cudaMemcpyHostToDevice );
      gpuErrchk(result);
      result = cudaMemcpy( dnewa[tid], a+(tid*input_m*input_n/numthread)-m, memsize, cudaMemcpyHostToDevice );
      gpuErrchk(result);
    }
    else
    {
      gx = (n)/bx + ((n)%bx == 0?0:1);
      gy = (m-2)/by + ((m-2)%by == 0?0:1);
      result = cudaMemcpy( da, a+(tid*input_m*input_n/numthread)-m, memsize, cudaMemcpyHostToDevice );
      gpuErrchk(result);
      result = cudaMemcpy( dnewa[tid], a+(tid*input_m*input_n/numthread)-m, memsize, cudaMemcpyHostToDevice );
      gpuErrchk(result);
    }
//    sumtime = 0.0f;
//    cudaEventCreate( &e1 );
//    cudaEventCreate( &e2 );

    dim3 block( bx, by );
    dim3 grid( gx, gy );
        printf("\nGrids =  %i and %i\n", grid.x, grid.y);

    iters = 0;
    do{
        float msec;
        ++iters;

//        cudaEventRecord( e1 );
        if(tid == 0)
        {
          jacobikernel<<<(gx,gy), block>>>( da, dnewa[tid], lchange, n+1, m, w0, w1, w2 );
        }
        else if(tid == numthread-1)
        {
          jacobikernel<<< (gx,gy), block >>>( da, dnewa[tid], lchange, n+1, m, w0, w1, w2 );
        }
        else
        {
          jacobikernel<<< (gx,gy), block >>>( da, dnewa[tid], lchange, n+2, m, w0, w1, w2 );
        }
	cudaDeviceSynchronize();
//        reductionkernel<<< 1, bx*by >>>( lchange, gx*gy ); /* both levels of reduction happen on GPU */
//        cudaEventRecord( e2 );
        
        // exchange halo
        if(tid > 0)
        {
          result = cudaMemcpyPeer(dnewa[tid-1]+(n-1)*m,tid-1,dnewa[tid],tid,sizeof(float)*m); 
          gpuErrchk(result);
        }
//        if(tid < numthread-1)
//        {
//          result = cudaMemcpyPeerAsync(dnewa[tid],tid+1,dnewa[tid]+(n-1)*m,tid,sizeof(float)*m,0); 
//          gpuErrchk(result);
//        }

//        result = cudaMemcpy( &change, lchange, sizeof(float), cudaMemcpyDeviceToHost ); /* copy final reduction result to CPU */
//        gpuErrchk(result);
//        cudaEventElapsedTime( &msec, e1, e2 );
//        sumtime += msec;
        float *ta;
        ta = da;
        da = dnewa[tid];
        dnewa[tid] = ta; 
    //}while( change > tol );
    }while( iters <= 2);
//    printf( "JacobiGPU  converged in %d iterations to residual %f\n", iters, change );
//    printf( "JacobiGPU  used %f seconds total\n", sumtime/1000.0f );
    if(tid == 0)
    {
      cudaMemcpy( a, dnewa[tid], memsize-m*sizeof(float), cudaMemcpyDeviceToHost );
    }
    else if(tid == numthread-1)
    {
      cudaMemcpy( a+(tid*input_m*input_n/numthread), dnewa[tid]+m, memsize-m*sizeof(float), cudaMemcpyDeviceToHost );
    }
    else
    {
      cudaMemcpy( a+(tid*input_m*input_n/numthread), dnewa[tid]+m, memsize-2*m*sizeof(float), cudaMemcpyDeviceToHost );
    }
    cudaFree( da );
    cudaFree( dnewa[tid] );
    cudaFree( lchange );
    cudaEventDestroy( e1 );
    cudaEventDestroy( e2 );
}

static void init( float* a, int n, int m )
{
    int i, j;
    memset( a, 0, sizeof(float) * n * m );
    /* boundary conditions */
    for( j = 0; j < n; ++j ){
        a[j*m+n-1] = j;
    }
    for( i = 0; i < m; ++i ){
        a[(n-1)*m+i] = i;
    }
    a[(n-1)*m+m-1] = m+n;
}

void dumpFile(float *a, int m, int n, char* prefix)
{
    FILE *initfile;
    char name[15];
    strcpy(name, prefix);
    strcat(name, ".txt");
    initfile = fopen(name,"w+");
    for(int idxi = 0; idxi < n; ++idxi)
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

int
main( int argc, char* argv[] )
{
    int n, m;
    float *a_h;
    struct timeval tt1, tt2;
    int ms;
    float fms;
    int Thr;
    int devCount,devIdx;
    cudaError_t cudareturn;

    cudaGetDeviceCount(&devCount);
    omp_set_num_threads(devCount);

    float* dnewa[devCount];

#if 0
    if( argc <= 1 ){
        fprintf( stderr, "%s sizen [sizem]\n", argv[0] );
        return 1;
    }

    n = atoi( argv[1] );
    if( n <= 0 ) n = 100;
    m = n;
    if( argc > 3 ){
        m = atoi( argv[2] );
        Thr = atoi( argv[3] );
        if( m <= 0 ) m = 100;
    }
#endif
    n = 16;
    m=n; 
    Thr=4;

    printf( "Jacobi %d x %d\n", n, m );

    a_h = (float*)malloc( sizeof(float) * n * m );

    init( a_h, n, m );
    dumpFile(a_h,m,n,"init");

    gettimeofday( &tt1, NULL );

#pragma omp parallel default(shared) 
{
    JacobiGPU( a_h, n, m, .2, .1, .1, .1, Thr ,dnewa);
}
    gettimeofday( &tt2, NULL );
    ms = (tt2.tv_sec - tt1.tv_sec);
    ms = ms * 1000000 + (tt2.tv_usec - tt1.tv_usec);
    fms = (float)ms / 1000000.0f;
    printf( "time(gpu ) = %f seconds\n", fms );
    dumpFile(a_h,m,n,"new");
}

