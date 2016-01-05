#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mpi.h"
#include "jacobi.h"
#include "testname.h"

/*
 * do_print controls output useful for debugging and observing, particularly
 * the sloooowwww convergence of this method.
 */
static int do_print = 0;
static int max_it = 100;

/*
 * Define the symbol NO_CONV_TEST to mimic an explicit differencing scheme
 */

int 
main(int argc, char **argv)
{
  int       rank, size, i, j, itcnt;
  Mesh      mesh;

  double    diffnorm, gdiffnorm;

  double   *swap;
  double   *xlocalrow, *xnewrow;

  int       maxm, maxn, lrow;
  int       k;
  double    t;

  MPI_Init(&argc, &argv);

#ifdef V_T
  setup_tracing();		/* Setup the instrumentation (VT.c) */
#endif
  
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);


  Get_command_line(rank, argc, argv, &maxm, &maxn, &do_print, &max_it);

  Setup_mesh(maxm, maxn, &mesh);

  /* Run twice and take the last time (ensures that code is paged in) */
  for (k = 0; k < 2; k++) {
    itcnt = 0;

    Init_mesh(&mesh);

    MPI_Barrier(mesh.ring_comm);
    t = MPI_Wtime();
    if (do_print && rank == 0) {

      printf("Starting at time %f\n", t);
      fflush(stdout);
    }
    /* lrow is used enough that we make a local copy */
    lrow = mesh.lrow;

    do {
	ExchangeStart(&mesh);
      /*
       * Compute new values (but not on boundary). Note that many compilers
       * will not generate good code for this (i.e., will not do the obvious
       * strength reductions). The code below gets reasonable performance on
       * some systems; additional rewriting might help others (e.g., 1+j
       * instead of j+1).
       * 
       * Note that without some "disjoint" xnewrow, xlocalrow, extra loads are
       * needed in the loop.
       */
      itcnt++;

      diffnorm = 0.0;

      /* Interior points */
      xnewrow = mesh.xnew + 2 * maxm;
      xlocalrow = mesh.xlocal + 2 * maxm;
      for (i = 2; i <= lrow - 1; i++) {
	for (j = 1; j < maxm - 1; j++) {
	  double    diff;
	  /* Multiply by .25 instead of divide by 4.0 */
	  xnewrow[j] = (xlocalrow[j + 1] +
			xlocalrow[j - 1] +
			xlocalrow[maxm + j] +
			xlocalrow[-maxm + j]) * 0.25;

	  /*
	   * Accumulating diffnorm here can reduce cache misses on large
	   * data.  Local variable diff is used because some compilers can
	   * not do this simple common sub-expression analysis
	   */
	  diff = xnewrow[j] - xlocalrow[j];
	  diffnorm += diff * diff;
	}
	xnewrow += maxm;
	xlocalrow += maxm;
      }
      ExchangeEnd(&mesh);

      /* Points sharing ghost points */
      xnewrow = mesh.xnew + 1 * maxm;
      xlocalrow = mesh.xlocal + 1 * maxm;
      for (j = 1; j < maxm - 1; j++) {
	double    diff;
	/* Multiply by .25 instead of divide by 4.0 */
	xnewrow[j] = (xlocalrow[j + 1] +
		      xlocalrow[j - 1] +
		      xlocalrow[maxm + j] +
		      xlocalrow[-maxm + j]) * 0.25;

	/*
	 * Accumulating diffnorm here can reduce cache misses on large data.
	 * Local variable diff is used because some compilers can not do this
	 * simple common sub-expression analysis
	 */
	diff = xnewrow[j] - xlocalrow[j];
	diffnorm += diff * diff;
      }
      /* Points sharing ghost points */
      xnewrow = mesh.xnew + lrow * maxm;
      xlocalrow = mesh.xlocal + lrow * maxm;
      for (j = 1; j < maxm - 1; j++) {
	double    diff;
	/* Multiply by .25 instead of divide by 4.0 */
	xnewrow[j] = (xlocalrow[j + 1] +
		      xlocalrow[j - 1] +
		      xlocalrow[maxm + j] +
		      xlocalrow[-maxm + j]) * 0.25;

	/*
	 * Accumulating diffnorm here can reduce cache misses on large data.
	 * Local variable diff is used because some compilers can not do this
	 * simple common sub-expression analysis
	 */
	diff = xnewrow[j] - xlocalrow[j];
	diffnorm += diff * diff;
      }

      swap = mesh.xlocal;
      mesh.xlocal = mesh.xnew;
      mesh.xnew = swap;

      /* Convergence test */
      MPI_Allreduce(&diffnorm, &gdiffnorm, 1, MPI_DOUBLE, MPI_SUM,
		    mesh.ring_comm);
      gdiffnorm = sqrt(gdiffnorm);

#ifdef V_T /* counter */
      {
	  VT_countval( 1, &counterhandle, &gdiffnorm );
      }
#endif
      
      if (do_print && rank == 0) {
	printf("At iteration %d, diff is %e\n", itcnt, gdiffnorm);
	fflush(stdout);
      }
    } while (gdiffnorm > 1.0e-2 && itcnt < max_it);

    t = MPI_Wtime() - t;
    ExchangeEnd(&mesh);
  }
  if (rank == 0) {
    printf("%s: %d iterations in %f secs (%f MFlops), m=%d n=%d np=%d\n",
	   argv[0], itcnt, t,
    itcnt * (maxm - 2.0) * (maxn - 2) * (4) * 1.0e-6 / t, maxm, maxn, size);
  }
  MPI_Comm_free(&mesh.ring_comm);
  MPI_Finalize();
  return 0;
}

