!
!  ./Poisson/solutions/red_black.f90
!
!
! Contents of this file:

! PROGRAM relax (MPI driver program)
! SUBROUTINE red_black (parallel red black relaxation program)
!

PROGRAM relax
!
! Driver program for an MPI  parallel  Poisson model problem solver 
! with red black relaxation. Parallelization by grid partitioning.
!

USE parallel_data
USE types
USE mpi
USE output

USE errhnd  ! for setting (init_errh) and freeing (end_errh) error handlers
            ! see Courses/MPI-1/ErrHnd directory


IMPLICIT NONE

!!!!! INTERFACES !!!!!!!!!!!!!

INTERFACE

SUBROUTINE poisson_red_black( x, n, imin, imax, jmin, jmax, niter )

USE mpi
USE types

IMPLICIT NONE

INTEGER, INTENT(IN):: n, imin, imax, jmin, jmax
INTEGER, INTENT(OUT):: niter

REAL(KIND=REAL8), DIMENSION(imin-1:imax+1,jmin-1:jmax+1), INTENT(INOUT) :: x

END SUBROUTINE poisson_red_black

END INTERFACE

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

INTEGER:: error, msg(3)
INTEGER:: n, imin, imax, jmin, jmax, niter

REAL(KIND=REAL8), DIMENSION(:,:), ALLOCATABLE:: x
REAL(KIND=REAL8):: t

CALL MPI_INIT( error )

CALL init_errh(MPI_COMM_WORLD)

CALL MPI_COMM_RANK( MPI_COMM_WORLD, myid, error )
CALL MPI_COMM_SIZE( MPI_COMM_WORLD, numprocs, error )

IF (myid == 0) THEN

OPEN(5,file='inp')

PRINT*, "Input N s.th. the overall grid size is N x N: "
READ(5,*) n

IF( numprocs>1 ) THEN
PRINT*, "Input p1,p2 s.th. process grid is p1 x p2"
READ(5,*) p1,p2
ELSE
p1=1
p2=1
ENDIF

msg(1)=n
msg(2)=p1
msg(3)=p2

ENDIF

CALL MPI_BCAST(msg, 3, MPI_INTEGER, 0, MPI_COMM_WORLD, error)

n=msg(1)
p1=msg(2)
p2=msg(3)


OKAY: IF( p1>0 .AND. p2>0 .AND. p1*p2 == numprocs .AND. MAX(p1,p2)<=n ) THEN

! Initialize grid partitioning data (=> MODULE parallel_data in pardat.f90)
  CALL init_par(n, imin, imax, jmin, jmax, error)

  ACTIVE: IF( myid<p1*p2 ) THEN
!! Calculate own domain

  ALLOCATE(x(imin-1:imax+1,jmin-1:jmax+1), stat=error)
  CALL errfunc(MPI_COMM_WORLD,MPI_SUCCESS-abs(error))

  IF( error == 0) THEN

    CALL prep_out()

    x=REAL(0.,KIND=REAL8)

    t = MPI_Wtime()
    CALL poisson_red_black( x, n, imin, imax, jmin, jmax, niter )
    t = MPI_Wtime()-t

    IF( myid==0 ) THEN
       WRITE(*,*) ' W-Time: ',t
       WRITE(*,*) ' MFlops: ',8.*n*n*niter/t*1.e-6
    ENDIF

    CALL out(x, imin, imax, jmin, jmax)

    CALL close_out()

  ENDIF

  ELSE

    WRITE(*,*) myid,' IDLE ..... '

  ENDIF ACTIVE

ELSE

  WRITE(*,*) 'Invalid input '

ENDIF OKAY

CALL end_errh(MPI_COMM_WORLD)
CALL MPI_FINALIZE(error)

END PROGRAM relax

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

SUBROUTINE poisson_red_black( x, n, imin, imax, jmin, jmax, niter )

USE mpi
USE types

USE Boundary_functions
USE parallel_data

IMPLICIT NONE

!!!! Control parameters

INTEGER, PARAMETER:: MAXIT=50                ! Max. #iterations
REAL(KIND=REAL8), PARAMETER:: EPS=0.00001    ! Convergence criterion is
                                             ! || resid. ||^2 < EPS

!!!!

INTEGER, INTENT(IN):: n,imin,imax,jmin,jmax
INTEGER, INTENT(OUT):: niter

REAL(KIND=REAL8), DIMENSION(imin-1:imax+1,jmin-1:jmax+1), INTENT(INOUT) :: x


!
! Solves Uxx + Uyy =f; U=g (boundary conditions); on unit square
!
! f,g external REAL(KIND=REAL8) functions
!
! INOUT: x (Start solution on input, discrete solution on output)
! IN:    n (Grid size is NxN)
!        imin,imax,jmin,jmax: local process' grid is imin..imax x jmin..jmax
! OUT:   niter (number of iterations used)
!
! n: Diskretisierung 
!
! Method :  u_new = 0.25*(h^2 f(.,.) + u_north (.,.) + u_west(.,.)
!                                    + u_east(.,.)   + u_south(.,.) )
!
!           (interior points), first red, then black points.
!
! F90 technique: use pointers for all auxiliary fields u_north,..,u_south
!

REAL(KIND=REAL8), DIMENSION(imin:imax,jmin:jmax):: f_disk  
                                               ! Automatic, auxiliary f.
                                               ! discrete f(.,.)
REAL(KIND=REAL8), DIMENSION(imin-1:imax+1,jmin-1:jmax+1), TARGET:: x_iter
                                               ! Auxiliary for Iteration

LOGICAL, DIMENSION(imin:imax,jmin:jmax):: RED  ! Mask for 'red' points



REAL(KIND=REAL8), DIMENSION(:,:), POINTER:: u, u_north, u_west, u_east, u_south

REAL(KIND=REAL8) :: h,h2,zero=0._REAL8, one=1._REAL8, qurt=.25_REAL8, resid, r

INTEGER:: i,j, ni, nj, it, ierr

ni = imax-imin+1
nj = jmax-jmin+1

! Randwerte
h = REAL(1.,KIND=REAL8)/REAL(n,KIND=REAL8)

x_iter= x

!Boundary conditions

IF( imin   == 1 ) THEN
x_iter(0,:) = (/ (g(zero,h*REAL(j,KIND=REAL8)),j=jmin-1,jmax+1) /)
ENDIF
IF( imax   == n ) THEN
x_iter(n+1,:) = (/ (g(one,h*REAL(j,KIND=REAL8)),j=jmin-1,jmax+1) /)
ENDIF
IF( jmin   == 1 ) THEN
x_iter(:,0) = (/ (g(h*REAL(i,KIND=REAL8),zero),i=imin-1,imax+1) /)
ENDIF
IF( jmax   == n ) THEN
x_iter(:,n+1) = (/ (g(h*REAL(i,KIND=REAL8),one ),i=imin-1,imax+1) /)
ENDIF


u        => x_iter(imin:imax,jmin:jmax)
u_north   => x_iter(imin:imax,jmin+1:jmax+1)
u_west   => x_iter(imin-1:imax-1,jmin:jmax)
u_east    => x_iter(imin+1:imax+1,jmin:jmax)
u_south   => x_iter(imin:imax,jmin-1:jmax-1)

! red points: (i,j) with (i+j) even

RED = RESHAPE( (/((MOD(i+j,2) == 0, i=imin,imax), j=jmin,jmax)/),          &
                   SHAPE=(/ni,nj/) )

h2 = h*h

f_disk = h2*RESHAPE &
( (/(( f(h*REAL(i,KIND=REAL8),h*REAL(j,KIND=REAL8))                        &
  &  ,i=imin,imax),j=jmin,jmax)/) ,SHAPE = (/ni,nj/) )

niter = MAXIT

iter: DO it=1,MAXIT     ! Max. MAXIT Iterations

      x = x_iter

      WHERE( RED )
       u = qurt*(f_disk+ u_north + u_west + u_east + u_south)
      ENDWHERE

!Boundary exchange red points
      CALL exchange('RED', x_iter, imin, imax, jmin, jmax)

      WHERE( .NOT.RED )
       u = qurt*(f_disk+ u_north + u_west + u_east + u_south)
      ENDWHERE

!Boundary exchange black points
      CALL exchange('BLACK', x_iter, imin, imax, jmin, jmax)

! Test convergence

      r = 0.
      DO j=jmin,jmax
      DO i=imin,imax

          r = r+ (x_iter(i,j)-x(i,j))**2

      ENDDO
      ENDDO

      CALL MPI_ALLREDUCE(r, resid, 1, MPI_DOUBLE_PRECISION, MPI_SUM,       &
                         MPI_COMM_WORLD, ierr)


      IF( myid==0 .AND. (it==MAXIT.OR.resid<EPS.OR.mod(it,100)==0) ) THEN
      WRITE(*,'(a,i5,a,e12.3)') 'Residuum after iteration ',it,' is ',resid
      ENDIF

      IF( resid < EPS ) THEN

         IF( myid==0 ) THEN
         WRITE(*,'(a)') 'Convergence !'
         ENDIF

         niter=it

         EXIT iter

      ENDIF

      ENDDO iter

      x = x_iter

END SUBROUTINE poisson_red_black
