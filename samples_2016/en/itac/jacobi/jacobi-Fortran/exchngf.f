      subroutine exchng2( a, sx, ex, sy, ey, 
     $     comm2d, stridetype, 
     $     nbrleft, nbrright, nbrtop, nbrbottom  )
      include "mpif.h"
      integer sx, ex, sy, ey, stridetype
      double precision a(sx-1:ex+1, sy-1:ey+1)
      integer nbrleft, nbrright, nbrtop, nbrbottom, comm2d
      integer status(MPI_STATUS_SIZE), ierr, nx
c#ifdef V_T
      include 'VT.inc'
      include 'vtcommon.inc'

      call VTBEGIN( exchng2_state , ierr)
c#endif
c
      nx = ex - sx + 1
c  These are just like the 1-d versions, except for less data
      call MPI_SENDRECV( a(sx,ey),  nx, MPI_DOUBLE_PRECISION, 
     $ 	                  nbrtop, 0,
     $                    a(sx,sy-1), nx, MPI_DOUBLE_PRECISION, 
     $                    nbrbottom, 0, comm2d, status, ierr )
      call MPI_SENDRECV( a(sx,sy),  nx, MPI_DOUBLE_PRECISION,
     $                    nbrbottom, 1,
     $                    a(sx,ey+1), nx, MPI_DOUBLE_PRECISION, 
     $                    nbrtop, 1, comm2d, status, ierr )
c
c This uses the "strided" datatype
      call MPI_SENDRECV( a(ex,sy),  1, stridetype, nbrright, 0,
     $     a(sx-1,sy), 1, stridetype, nbrleft, 0, 
     $     comm2d, status, ierr )
      call MPI_SENDRECV( a(sx,sy),  1, stridetype, nbrleft,   1,
     $     a(ex+1,sy), 1, stridetype, nbrright, 1, 
     $     comm2d, status, ierr )
c#ifdef V_T
      call VTEND( exchng2_state , ierr)
c#endif
      return
      end





