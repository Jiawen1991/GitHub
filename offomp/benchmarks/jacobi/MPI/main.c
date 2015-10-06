/*Original source: 
  https://github.com/rasmusto/CUDA/blob/master/john_jacobi/jacobi.cu
  There are even better versions there. TODO:
  https://github.com/rasmusto/CUDA/blob/master/jacobi_final/1k_jacobi6.cu
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <mpi.h>

void JacobiGPU( float* a, int n, int m, float w0, float w1, float w2, float tol, int Thr , float *da, float *dnewa, float *lchange);
void initDevice(float *a, int n, int m, float** da, float** dnewa, float** lchange , int Thr, int rank);
void cleanDevice(float *a, float *da, float *dnewa, float *lchange,int m, int n);

static void init( float* a, int n, int m, int rank)
{
    int i, j;
    memset( a, 0, sizeof(float) * (n+2) * m );
    /* boundary conditions */
    for( j = 1; j <= n; ++j ){
        a[j*m+m-1] = j - 1 + rank * n;
    }
    if((rank+1) * n == m)
    {
      for( i = 0; i < m; ++i ){
        a[n*m + i] = i;
      }
      a[n*m+m-1] = m+m;
    }
}


int
main( int argc, char* argv[] )
{
    int n, m;
    float *a;
    struct timeval tt1, tt2;
    int ms;
    float fms;
    int Thr;
    float *da,*dnewa,*lchange;
    float* idx[2];

// MPI setup
    int rank, nprocs;
    MPI_Status status;
    MPI_Init (&argc, &argv);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);
    MPI_Comm_size (MPI_COMM_WORLD, &nprocs);
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
    m = 1024;
    n=m/nprocs; 
    Thr=16;

    if(rank == 0)
    {
      printf("I am rank 0 and we have total %d ranks!\n",nprocs);
      printf( "Total Jacobi %d x %d\n", m, m );
      printf( "Subset: %d x %d\n", m, n );
    }

    a = (float*)malloc( sizeof(float) * (n+2) * m );


    init( a, n, m, rank );
    dumpFile(a,rank,m,n,"init_");
    initDevice(a,n,m,&da,&dnewa,&lchange,Thr, rank);
//    printf("MPI rank %d has address of %p %p\n",rank,da,dnewa);
    idx[0] = da;
    idx[1] = dnewa;
    int iters = 0;
    int i0 = 0;
    int i1 = 1;
    gettimeofday( &tt1, NULL );
    do{
        ++iters;
	/* Send up unless I'm at the top, then receive from below */
	/* Note the use of xlocal[i] for &xlocal[i][0] */
	if (rank > 0)
        {
//            printf("I am rank %d, sending to rank %d\n",rank, rank -1); 
	    MPI_Send( (float*)(a+m), m, MPI_FLOAT, rank - 1, 0, 
		      MPI_COMM_WORLD );
        }
	if (rank < nprocs - 1)
        { 
//            printf("I am rank %d, receiving to rank %d\n",rank, rank +1); 
	    MPI_Recv( (float*)(a+m*(n+1)) , m, MPI_FLOAT, rank + 1, 0, 
		      MPI_COMM_WORLD, &status );
        }
	/* Send down unless I'm at the bottom */
	if (rank < nprocs - 1) 
        {
//            printf("I am rank %d, sending to rank %d\n",rank, rank +1); 
	    MPI_Send( (float*)(a+m*n), m, MPI_FLOAT, rank + 1, 1, 
		      MPI_COMM_WORLD );
        }
	if (rank > 0)
        {
//            printf("I am rank %d, receiving to rank %d\n",rank, rank -1); 
	    MPI_Recv( (float*)(a), m, MPI_FLOAT, rank - 1, 1, 
		      MPI_COMM_WORLD, &status );
        }
        JacobiGPU( a, n, m, .2, .1, .1, .1, Thr, idx[i0], idx[i1], lchange);
        i0 = 1 - i0;
        i1 = 1 - i1;
    //}while( change > tol );
    }while( iters <= 5000);
    gettimeofday( &tt2, NULL );
////    printf( "JacobiGPU  converged in %d iterations to residual %f\n", iters, change );
////    printf( "JacobiGPU  used %f seconds total\n", sumtime/1000.0f );
// 
    if(rank == 0)
    {
    ms = (tt2.tv_sec - tt1.tv_sec);
    ms = ms * 1000000 + (tt2.tv_usec - tt1.tv_usec);
    fms = (float)ms / 1000000.0f;
    printf( "time(gpu ) = %f seconds\n", fms );
    }
    cleanDevice(a, idx[i0], idx[i1], lchange,m,n);
    dumpFile(a,rank,m,n,"new_");
//
    MPI_Finalize();
    return 0;
  }

