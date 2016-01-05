!
!  ./Poisson/boundary_functions.f90
!
MODULE boundary_functions

!!! External functions f,g, defining PDE   -(Laplace)u =f (interior)
!                                                    u =g (boundary)

CONTAINS

FUNCTION f(x,y)

USE types

IMPLICIT NONE

REAL(KIND=REAL8), INTENT(IN):: x,y
REAL(KIND=REAL8):: f

f = 0._REAL8

END FUNCTION f

FUNCTION g(x,y)

USE types

IMPLICIT NONE

REAL(KIND=REAL8), INTENT(IN):: x,y
REAL(KIND=REAL8):: g

g = 1._REAL8

END FUNCTION g

END MODULE
