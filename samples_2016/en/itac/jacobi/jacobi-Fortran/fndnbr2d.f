c
c This routine show how to determine the neighbors in a 2-d decomposition of
c the domain. This assumes that MPI_Cart_create has already been called 
c
      subroutine fnd2dnbrs( comm2d, 
     $                      nbrleft, nbrright, nbrtop, nbrbottom )
      integer comm2d, nbrleft, nbrright, nbrtop, nbrbottom
c
      integer ierr
c
c#ifdef V_T
      include 'VT.inc'
      include 'vtcommon.inc'

      call VTBEGIN( fnd2dnbrs_state , ierr)
c#endif      
      call MPI_Cart_shift( comm2d, 0,  1, nbrleft,   nbrright, ierr )
      call MPI_Cart_shift( comm2d, 1,  1, nbrbottom, nbrtop,   ierr )
c#ifdef V_T
      call VTEND( fnd2dnbrs_state , ierr)
c#endif
      return
      end
c
      subroutine fnd2ddecomp( comm2d, n, sx, ex, sy, ey )
      integer comm2d
      integer n, sx, ex, sy, ey
      integer dims(2), coords(2), ierr
      logical periods(2)
c
      include 'VT.inc'
      include 'vtcommon.inc'
      call VTBEGIN( fnd2ddecomp_state , ierr )
      call MPI_Cart_get( comm2d, 2, dims, periods, coords, ierr )

      call MPE_DECOMP1D( n, dims(1), coords(1), sx, ex )
      call MPE_DECOMP1D( n, dims(2), coords(2), sy, ey )
c     
c#ifdef V_T
      call VTEND( fnd2ddecomp_state , ierr )
c#endif
      return
      end
      

