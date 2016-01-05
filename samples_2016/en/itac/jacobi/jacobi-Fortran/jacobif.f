c**********************************************************************
c   twod.f - a solution to the Poisson problem by using Jacobi 
c   iteration on a 2-d decomposition
c     
c****************************************************************************
c     
c     code modified to include Intel(R) Trace Collector instrumentation
c     
      program main
      include "mpif.h"

      integer maxn
      parameter (maxn = 1024)
      double precision  a(maxn,maxn), b(maxn,maxn), f(maxn,maxn)
      integer nx, ny
      integer myid, numprocs, it, rc, comm2d, ierr, stride
      integer nbrleft, nbrright, nbrtop, nbrbottom
      integer sx, ex, sy, ey
      integer dims(2)
      logical periods(2)
      double precision diff2d, diffnorm, dwork
      double precision t1, t2
      external diff2d
      data periods/2*.false./

c#ifdef V_T
      include 'VT.inc'
      include 'vtcommon.inc'

      call VTTRACEOFF(ierr)
c#endif

      call MPI_INIT( ierr )
c#ifdef V_T
      call VTSETUP()
      call VTTRACEON(ierr)
      call VTBEGIN( main_state , ierr)
c#endif

      call MPI_COMM_RANK( MPI_COMM_WORLD, myid, ierr )
      call MPI_COMM_SIZE( MPI_COMM_WORLD, numprocs, ierr )
c      print *, "Process ", myid, " of ", numprocs, " is alive"
      if (myid .eq. 0) then
c
c         Get the size of the problem
c
c          print *, 'Enter nx'
c          read *, nx
           nx = 10
      endif
c      print *, 'About to do bcast on ', myid
      call MPI_BCAST(nx,1,MPI_INTEGER,0,MPI_COMM_WORLD,ierr)
      ny   = nx
c
c Get a new communicator for a decomposition of the domain.  Let MPI
c find a "good" decomposition
c
      dims(1) = 0
      dims(2) = 0
      call MPI_DIMS_CREATE( numprocs, 2, dims, ierr )
      call MPI_CART_CREATE( MPI_COMM_WORLD, 2, dims, periods, .true.,
     *                    comm2d, ierr )
c
c Get my position in this communicator
c 
      call MPI_COMM_RANK( comm2d, myid, ierr )
c      print *, "Process ", myid, " of ", numprocs, " is alive"
c
c My neighbors are now +/- 1 with my rank.  Handle the case of the 
c boundaries by using MPI_PROCNULL.
      call fnd2dnbrs( comm2d, nbrleft, nbrright, nbrtop, nbrbottom )
c      print *, "Process ", myid, ":", 
c     *     nbrleft, nbrright, nbrtop, nbrbottom
c
c Compute the decomposition
c     
      call fnd2ddecomp( comm2d, nx, sx, ex, sy, ey )
c      print *, "Process ", myid, ":", sx, ex, sy, ey
c
c Create a new, "strided" datatype for the exchange in the "non-contiguous"
c direction
c
      call mpi_Type_vector( ey-sy+1, 1, ex-sx+3, 
     $                      MPI_DOUBLE_PRECISION, stride, ierr )
      call mpi_Type_commit( stride, ierr )
c
c
c Initialize the right-hand-side (f) and the initial solution guess (a)
c
      call twodinit( a, b, f, nx, sx, ex, sy, ey )
c
c Actually do the computation.  Note the use of a collective operation to
c check for convergence, and a do-loop to bound the number of iterations.
c
      call MPI_BARRIER( MPI_COMM_WORLD, ierr )
      t1 = MPI_WTIME()
      do 10 it=1, 100
	call exchng2( b, sx, ex, sy, ey, comm2d, stride, 
     $                nbrleft, nbrright, nbrtop, nbrbottom )
	call sweep2d( b, f, nx, sx, ex, sy, ey, a )
	call exchng2( a, sx, ex, sy, ey, comm2d, stride, 
     $                nbrleft, nbrright, nbrtop, nbrbottom )
	call sweep2d( a, f, nx, sx, ex, sy, ey, b )
	dwork = diff2d( a, b, nx, sx, ex, sy, ey )
	call MPI_Allreduce( dwork, diffnorm, 1, MPI_DOUBLE_PRECISION, 
     $                     MPI_SUM, comm2d, ierr )
        if (diffnorm .lt. 1.0e-5) goto 20
        if (myid .eq. 0) print *, 2*it, ' Difference is ', diffnorm
10     continue
      if (myid .eq. 0) print *, 'Failed to converge'
20    continue
      t2 = MPI_WTIME()
c      if (myid .eq. 0) then
c         print *, 'Converged after ', 2*it, ' Iterations in ', t2 - t1,
c     $        ' secs '
c      endif
c
c
      call MPI_Type_free( stride, ierr )
      call MPI_Comm_free( comm2d, ierr )
c#ifdef V_T
c     leave main program
      call VTEND( main_state , ierr)
c#endif
      call MPI_FINALIZE(rc)
      end
