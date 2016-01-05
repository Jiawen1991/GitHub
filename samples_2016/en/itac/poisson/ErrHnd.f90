!
!  ./Errhnd/ErrHnd.f90
!
MODULE errhnd
!
! Module: Initialization/Finalization of MPI error handlers
!
! Contains:
!   No data
!   Functions:
!       init_errh(Comm)      (CREATE/SET and error handler for communicator
!                             Comm)
!       end_errh(Comm)       (FREE it)
!       errfunc( Comm, MPIerr )
!                             Callback function
!                             MPI_SUCCESS<=MPIerr<= MPI_ERR_LASTCODE
!                                      => MPI-error ccurred
!                             Else: User error code assumed.
!
!

CONTAINS

SUBROUTINE init_errh(Comm)

USE mpi

IMPLICIT NONE

INTEGER, INTENT(IN):: Comm   ! Communicator

INTEGER:: ERRHND, ierr

IF( Comm /= MPI_COMM_NULL ) THEN

CALL MPI_ERRHANDLER_CREATE(errfunc,ERRHND,ierr)
CALL MPI_ERRHANDLER_SET   (Comm,ERRHND,ierr)

ENDIF

END SUBROUTINE init_errh

SUBROUTINE end_errh(Comm)

USE mpi

IMPLICIT NONE

INTEGER, INTENT(IN):: Comm   ! Communicator

INTEGER:: ERRHND, ierr

IF( Comm /= MPI_COMM_NULL ) THEN

CALL MPI_ERRHANDLER_GET   (Comm,ERRHND,ierr)
CALL MPI_ERRHANDLER_FREE  (ERRHND,ierr)

ENDIF

END SUBROUTINE end_errh

SUBROUTINE errfunc(Comm, MPIerr)

USE mpi

IMPLICIT NONE

INTEGER :: Comm
INTEGER :: MPIerr

INTEGER:: myid_world, myid, ierr, errlen
INTEGER, PARAMETER:: MAXMSG=128
CHARACTER(LEN=MAXMSG)::MESSAGE

IF ( MPIerr /= MPI_SUCCESS ) THEN

CALL MPI_COMM_RANK(Comm, myid, ierr)
CALL MPI_COMM_RANK(MPI_COMM_WORLD, myid_world,ierr)

WRITE(*,*) myid_world,' of MPI_COMM_WORLD (=', myid,' of ',            &
'local communicator), got error'

IF( MPIerr > MPI_SUCCESS .AND. MPIerr <= MPI_ERR_LASTCODE ) THEN

CALL MPI_ERROR_STRING(MPIerr,MESSAGE,errlen,ierr)
WRITE(*,*) myid,' has an MPI error: ',MESSAGE(1:errlen)

ELSE

WRITE(*,*) myid,' has a user defined error: Allocation failed'

ENDIF

CALL MPI_ABORT(Comm,MPIerr,ierr)

ENDIF

END SUBROUTINE errfunc

END MODULE errhnd
