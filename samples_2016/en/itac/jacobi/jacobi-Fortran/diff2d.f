c
c  The rest of the 2-d program
c
      double precision function diff2d( a, b, nx, sx, ex, sy, ey )
      integer nx, sx, ex, sy, ey
      double precision a(sx-1:ex+1, sy-1:ey+1), b(sx-1:ex+1, sy-1:ey+1)
c
      double precision sum
      integer i, j
c

      integer ierr
      include 'VT.inc'
      include 'vtcommon.inc'

      call VTBEGIN( diff2d_state , ierr)

      sum = 0.0d0
      do 10 j=sy,ey
         do 10 i=sx,ex
            sum = sum + (a(i,j) - b(i,j)) ** 2
 10      continue
c      
      diff2d = sum

      call VTEND( diff2d_state , ierr)

      return
      end



