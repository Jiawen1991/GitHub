!
!  ./Poisson/output.f90
!
MODULE output
!
! Module for soilution output
!
! Contains:
!       show_sol (LOGICAL): switches output on/off
!       unit     (INTEGER): output file unit
!

LOGICAL, PARAMETER:: show_sol=.FALSE.
INTEGER:: unit = 20

!
! Functions:
!
!   prep_out:                   open output file, name is out_<proc_id>
!   out(x,imin,imax,jmin,jmax)  output of x(imin:imax,jmin:jmax) to output file
!   close_out:                  close output file

CONTAINS

SUBROUTINE prep_out

USE types
USE parallel_data

IMPLICIT NONE

LOGICAL:: first=.TRUE.
SAVE first

CHARACTER*10:: filename

IF( show_sol ) THEN
IF( first ) THEN

WRITE(filename,'(a4,i3.3)') 'out_',myid
OPEN(unit,file=filename)
first=.FALSE.

ENDIF
ENDIF

END SUBROUTINE prep_out

SUBROUTINE out(x,imin,imax,jmin,jmax)

USE types
USE parallel_data

IMPLICIT NONE

INTEGER, INTENT(IN):: imin,imax,jmin,jmax

REAL(KIND=REAL8), DIMENSION(imin-1:imax+1,jmin-1:jmax+1), INTENT(IN) :: x

INTEGER:: j

IF( show_sol ) THEN

WRITE(unit,*) ' Local grid is: ',imin,' .. ',imax,' x ',jmin,' .. ',jmax
WRITE(unit,*) ' '

DO j=jmax,jmin,-1
  WRITE(unit,'(5e12.4)') x(imin:imax,j)
ENDDO

ENDIF

END SUBROUTINE out

SUBROUTINE close_out

IF( show_sol ) THEN
close(unit)
ENDIF

END SUBROUTINE close_out


END MODULE
