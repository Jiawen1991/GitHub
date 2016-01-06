#include <stdio.h>
#include <float.h>
#include <math.h>
#ifdef _OPENMP
#include <omp.h>
#endif

#define REAL float
#define NTIMES 2000 
#define SIZE 500000
#define FACTOR 123.456

///////////////////axpy

#pragma offload_attribute(push, target(mic))
static REAL x[SIZE];
static REAL y[SIZE];
#pragma offload_attribute(pop)


static void init(REAL *A, long n);

void axpy()
{
    int i,n;
    init(x, SIZE);
    init(y, SIZE);
    double start_timer = omp_get_wtime();
    #pragma offload target(mic) optional
    {
    for(n=0; n<NTIMES; n++)
    {
    for (i=0; i<SIZE; i++)
	{
	    x[i] = x[i] * FACTOR + y[i];
	}
    }
    }
    double walltime = omp_get_wtime() - start_timer;
    printf("PASS axpy\n");
    printf("Matmul kernel wall clock time = %.2f sec\n", walltime);
}

static void init(REAL *A, long n) {
    long i;
    for (i = 0; i < n; i++) {
        A[i] = ((REAL) (drand48()) + 13);
    }
}

////////////////// check_devices

__attribute__((target(mic))) int chk_target00();
void check_devices()
{
   int num_devices = 0;
   int offload_mode = 0;
   int target_ok;
   int i;
  printf("Checking for Intel(R) Xeon Phi(TM) (Target CPU) devices...\n\n");

#ifdef __INTEL_OFFLOAD
   num_devices = _Offload_number_of_devices();
#endif
   printf("Number of Target devices installed: %d\n\n",num_devices);

   if (num_devices < 1) {
printf("Offload sections will execute on: Host CPU (fallback mode)\n\n");
      offload_mode = 0;
   }
   else {
      printf("Offload sections will execute on: Target CPU (offload mode)\n\n");
      offload_mode = 1;

      #pragma noinline
      for (i = 0; i < num_devices; i++) {
         target_ok = 0;
         #pragma offload target(mic: i) optional in(i)
         target_ok = chk_target00();
         if (! target_ok) {
            printf(" ***Warning: offload to device #%d : failed\n", i);
         }
       }
    }
}
__attribute__((target(mic))) int chk_target00()
{
    int retval;

    #ifdef __MIC__
        retval = 1;
    #else
        retval = 0;
    #endif

    // Return 1 if target available
       return retval;
}



int main(void)
{
    check_devices();

    axpy();
}
