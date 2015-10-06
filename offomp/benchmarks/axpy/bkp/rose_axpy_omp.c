#include "axpy.h"
#include "libxomp.h" 

struct OUT__1__5609___data 
{
  void *x_p;
  void *y_p;
  void *n_p;
  void *a_p;
}
;
static void OUT__1__5609__(void *__out_argv);

void axpy_omp(REAL *x,REAL *y, long n,REAL a)
{
   long i;
  struct OUT__1__5609___data __out_argv1__5609__;
  __out_argv1__5609__.a_p = ((void *)(&a));
  __out_argv1__5609__.n_p = ((void *)(&n));
  __out_argv1__5609__.y_p = ((void *)(&y));
  __out_argv1__5609__.x_p = ((void *)(&x));
  XOMP_parallel_start(OUT__1__5609__,&__out_argv1__5609__,1,0,"/data/yy8/2013-8-multiple-gpu-work/benchmarks/axpy/axpy_omp.c",5);
  XOMP_parallel_end("/data/yy8/2013-8-multiple-gpu-work/benchmarks/axpy/axpy_omp.c",7);
}

static void OUT__1__5609__(void *__out_argv)
{
  REAL **x = (REAL **)(((struct OUT__1__5609___data *)__out_argv) -> x_p);
  REAL **y = (REAL **)(((struct OUT__1__5609___data *)__out_argv) -> y_p);
   long *n = ( long *)(((struct OUT__1__5609___data *)__out_argv) -> n_p);
  REAL *a = (REAL *)(((struct OUT__1__5609___data *)__out_argv) -> a_p);
   long _p_i;
  long p_index_;
  long p_lower_;
  long p_upper_;
  XOMP_loop_default(0, *n - 1,1,&p_lower_,&p_upper_);
  for (p_index_ = p_lower_; p_index_ <= p_upper_; p_index_ += 1) {
    ( *y)[p_index_] += ( *a * ( *x)[p_index_]);
  }
  XOMP_barrier();
}
