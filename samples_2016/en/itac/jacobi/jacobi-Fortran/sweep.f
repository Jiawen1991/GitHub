c
c Perform a Jacobi sweep for a 2-d decomposition
c
      subroutine sweep2d( a, f, n, sx, ex, sy, ey, b )
      integer n, sx, ex, sy, ey
      double precision a(sx-1:ex+1, sy-1:ey+1), f(sx-1:ex+1, sy-1:ey+1),
     +                 b(sx-1:ex+1, sy-1:ey+1)
c
      integer i, j
      double precision h
c#ifdef V_T
      integer ierr
      include 'VT.inc'
      include 'vtcommon.inc'

      call VTBEGIN( sweep2d_state , ierr)
c#endif
c     
      h = 1.0d0 / dble(n+1)
      do 10 j=sy, ey
         do 10 i=sx, ex
            b(i,j) = 0.25 * (a(i-1,j)+a(i,j+1)+a(i,j-1)+a(i+1,j)) - 
     +               h * h * f(i,j)
 10   continue
c#ifdef V_T
      call VTEND( sweep2d_state , ierr)
c#endif
      return
      end
