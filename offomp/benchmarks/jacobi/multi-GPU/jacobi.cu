/*Original source: 
  https://github.com/rasmusto/CUDA/blob/master/john_jacobi/jacobi.cu
  There are even better versions there. TODO:
  https://github.com/rasmusto/CUDA/blob/master/jacobi_final/1k_jacobi6.cu
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>

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
jacobikernel( float* a, float* newa, float* lchange, int n, int m, float w0, float w1, float w2 , int devID, int numDev)
{        
    int ti = threadIdx.x;
    int tj = threadIdx.y;
    int i = blockIdx.x * blockDim.x + ti + 1;
    int j = blockIdx.y * blockDim.y + tj + 1;
    int i_upbound, j_upbound;
    i_upbound = m-1;
    j_upbound = n-1;
/* looks like arrays are linearized, so explicit address calculation is used to simulate 2-D access */
// I think the linearization is wrong: it should be a[i*m + j -1] instead of a[j*m+i-1]
    if((i < i_upbound) && (j < j_upbound))
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

static float sumtime[4];


void JacobiGPU( float* a, int input_n, int input_m, float w0, float w1, float w2, float tol, int Thr)
{
    float change, change_red;
    int iters;
    size_t memsize;
    int bx, by;
    int n,m, devID;  
    int numDev;
    int accessflag_l, accessflag_h;
    cudaError_t result;
    result = cudaGetDeviceCount(&numDev);
    gpuErrchk(result);   
    float* da[numDev];
    float* dnewa[numDev];
    float* lchange[numDev];
    int gx[numDev], gy[numDev];
    cudaStream_t stream[numDev];
    cudaEvent_t e1[numDev], e2[numDev];
    m = input_m;
    n = input_n / numDev; 

    bx = Thr;
    by = Thr;
    for(devID=0;devID<numDev;devID++)
    {
      result = cudaSetDevice(devID);
      gpuErrchk(result);   
      accessflag_l = 0;
      accessflag_h = 0;   
      if(devID > 0)
      {
         cudaDeviceCanAccessPeer(&accessflag_l, (devID-1), devID);
         cudaDeviceEnablePeerAccess(devID-1,0);
      }
      if(devID < numDev-1)
      {
         cudaDeviceCanAccessPeer(&accessflag_h, (devID+1), devID);
         cudaDeviceEnablePeerAccess(devID+1,0);
      }
      if(accessflag_l*accessflag_h)
      { 
        memsize = sizeof(float) * (n+2) * m;
      }
      else
      {
        memsize = sizeof(float) * (n+1) * m;
      }
      cudaMalloc( &da[devID], memsize );
      cudaMalloc( &dnewa[devID], memsize );
      result = cudaStreamCreate(&stream[devID]);
    
      if(devID == 0)
      {
        gx[devID] = (n-1)/bx + ((n-1)%bx == 0?0:1);
        gy[devID] = (m-2)/by + ((m-2)%by == 0?0:1);
        result = cudaMemcpyAsync( da[devID], a, memsize, cudaMemcpyHostToDevice, stream[devID] );
        gpuErrchk(result);
        result = cudaMemcpyAsync( dnewa[devID], a, memsize, cudaMemcpyHostToDevice, stream[devID] );
        gpuErrchk(result);
      }
      else if(devID == numDev-1)
      {
        gx[devID] = (n-1)/bx + ((n-1)%bx == 0?0:1);
        gy[devID] = (m-2)/by + ((m-2)%by == 0?0:1);
        result = cudaMemcpyAsync( da[devID], a+(devID*input_m*input_n/numDev)-m, memsize, cudaMemcpyHostToDevice, stream[devID] );
        gpuErrchk(result);
        result = cudaMemcpyAsync( dnewa[devID], a+(devID*input_m*input_n/numDev)-m, memsize, cudaMemcpyHostToDevice, stream[devID] );
        gpuErrchk(result);
      }
      else
      {
        gx[devID] = (n)/bx + ((n)%bx == 0?0:1);
        gy[devID] = (m-2)/by + ((m-2)%by == 0?0:1);
        result = cudaMemcpyAsync( da[devID], a+(devID*input_m*input_n/numDev)-m, memsize, cudaMemcpyHostToDevice, stream[devID] );
        gpuErrchk(result);
        result = cudaMemcpyAsync( dnewa[devID], a+(devID*input_m*input_n/numDev)-m, memsize, cudaMemcpyHostToDevice, stream[devID] );
        gpuErrchk(result);
      }
      cudaMalloc( &lchange[devID], gx[devID] * gy[devID] * sizeof(float) );
      cudaEventCreate( &e1[devID] );
      cudaEventCreate( &e2[devID] );
      sumtime[devID] = 0.0f;
    }
    iters = 0;
    do{
      float msec[numDev];
      ++iters;
      change_red = 0.f;
     for(devID=0;devID<numDev;devID++)
     {
       result = cudaSetDevice(devID);
       gpuErrchk(result);   
       dim3 block( bx, by ,1);
       cudaEventRecord( e1[devID] );
       //printf("\n Device:%d Grids =  %i and %i\n", devID, gx[devID], gy[devID]);
       if(devID == 0)
       {
         dim3 grid( 64,16,1);
         jacobikernel<<<grid, block, 0, stream[devID]>>>( da[devID], dnewa[devID], lchange[devID], n+1, m, w0, w1, w2, devID, numDev);
       }
       else if(devID == numDev-1)
       {
         dim3 grid( 64,16,1);
         jacobikernel<<<grid, block, 0, stream[devID]>>>( da[devID], dnewa[devID], lchange[devID], n+1, m, w0, w1, w2, devID, numDev);
       }
       else
       {
         dim3 grid( 64,16,1);
         jacobikernel<<<grid, block, 0, stream[devID]>>>( da[devID], dnewa[devID], lchange[devID], n+2, m, w0, w1, w2, devID, numDev);
       }
       reductionkernel<<< 1, bx*by , 0, stream[devID]>>>( lchange[devID], gx[devID]*gy[devID] ); /* both levels of reduction happen on GPU */
       cudaEventRecord( e2[devID] );
     }
     cudaDeviceSynchronize();
     for(devID=0;devID<numDev;devID++)
     {
       result = cudaMemcpy( &change, lchange[devID], sizeof(float), cudaMemcpyDeviceToHost ); /* copy final reduction result to CPU */
       gpuErrchk(result);
       change_red = (change > change_red) ? change : change_red;
     }
           
     for(devID=0;devID<numDev;devID++)
     {
           result = cudaSetDevice(devID);
           gpuErrchk(result);   
           // exchange halo
           if(devID > 0)
           {
             if(devID == 1)
               result = cudaMemcpyPeerAsync(dnewa[devID-1]+n*m,devID-1,dnewa[devID]+m,devID,sizeof(float)*m, stream[devID]);
             else 
               result = cudaMemcpyPeerAsync(dnewa[devID-1]+(n+1)*m,devID-1,dnewa[devID]+m,devID,sizeof(float)*m, stream[devID]);
             gpuErrchk(result);
           }
           if(devID < numDev-1)
           {
             if(devID == 0)
               result = cudaMemcpyPeerAsync(dnewa[devID+1],devID+1,dnewa[devID]+(n-1)*m,devID,sizeof(float)*m, stream[devID]);
             else 
               result = cudaMemcpyPeerAsync(dnewa[devID+1],devID+1,dnewa[devID]+n*m,devID,sizeof(float)*m, stream[devID]);
             gpuErrchk(result);
           }
           cudaEventElapsedTime( &msec[devID], e1[devID], e2[devID] );
           sumtime[devID] += msec[devID];
     } 
//     cudaDeviceSynchronize();
     for(devID=0;devID<numDev;devID++)
     {
           result = cudaSetDevice(devID);
           gpuErrchk(result);   
           float *ta;
           ta = da[devID];
           da[devID] = dnewa[devID];
           dnewa[devID] = ta; 
     }
    }while( iters <= 5000);
//}while( change > tol );

    printf( "JacobiGPU  converged in %d iterations to residual %f\n", iters, change_red );
    for(devID=0;devID<numDev;devID++)
      printf( "Device %d: JacobiGPU  used %f seconds total\n", devID, sumtime[devID]/1000.0f );
    for(devID=0;devID<numDev;devID++)
    {
      result = cudaSetDevice(devID);
      gpuErrchk(result);   

    if(devID == 0)
    {
      cudaMemcpy( a, dnewa[devID], m*n*sizeof(float), cudaMemcpyDeviceToHost );
    }
    else if(devID == numDev-1)
    {
      cudaMemcpy( a+(devID*input_m*input_n/numDev), dnewa[devID]+m, m*n*sizeof(float), cudaMemcpyDeviceToHost );
    }
    else
    {
      cudaMemcpy( a+(devID*input_m*input_n/numDev), dnewa[devID]+m, m*n*sizeof(float), cudaMemcpyDeviceToHost );
    }
    cudaFree( da[devID] );
    cudaFree( dnewa[devID] );
    cudaFree( lchange[devID] );
    cudaEventDestroy( e1[devID] );
    cudaEventDestroy( e2[devID] );
    }
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
    n = 1024;
    m=n; 
    Thr=16;

    printf( "Jacobi %d x %d\n", n, m );

    a_h = (float*)malloc( sizeof(float) * n * m );

    init( a_h, n, m );
//    dumpFile(a_h,m,n,"init");

    gettimeofday( &tt1, NULL );

    JacobiGPU( a_h, n, m, .2, .1, .1, .1, Thr);

    gettimeofday( &tt2, NULL );
    ms = (tt2.tv_sec - tt1.tv_sec);
    ms = ms * 1000000 + (tt2.tv_usec - tt1.tv_usec);
    fms = (float)ms / 1000000.0f;
    printf( "time(gpu ) = %f seconds\n", fms );
//    dumpFile(a_h,m,n,"new");
}

