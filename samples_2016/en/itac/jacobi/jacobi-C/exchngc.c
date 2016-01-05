#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mpi.h"
#include "jacobi.h"

void 
ExchangeStart(Mesh *mesh)
{
  double   *xlocal = mesh->xlocal;
  int       maxm = mesh->maxm;
  int       lrow = mesh->lrow;
  int       up_nbr = mesh->up_nbr;
  int       down_nbr = mesh->down_nbr;
  MPI_Comm  ring_comm = mesh->ring_comm;

#ifdef V_T
  {
      static int scl = VT_NOSCL;
      static int exchangestart_state;
      
      if( scl == VT_NOSCL ) {
	  VT_scldef( __FILE__, __LINE__, &scl );
	  VT_funcdef( "ExchangeStart", communication_class, &exchangestart_state );
      }
      VT_enter( exchangestart_state, scl );
  }
#endif
  /* Send up, then receive from below */
  MPI_Irecv(xlocal, maxm, MPI_DOUBLE, down_nbr, 0, ring_comm,
	    &mesh->rq[0]);
  MPI_Irecv(xlocal + maxm * (lrow + 1), maxm, MPI_DOUBLE, up_nbr, 1,
	    ring_comm, &mesh->rq[1]);
  MPI_Isend(xlocal + maxm * lrow, maxm, MPI_DOUBLE, up_nbr, 0,
	    ring_comm, &mesh->rq[2]);
  MPI_Isend(xlocal + maxm, maxm, MPI_DOUBLE, down_nbr, 1, ring_comm,
	    &mesh->rq[3]);
#ifdef V_T
  VT_leave( VT_NOSCL );
#endif
}

void 
ExchangeEnd( Mesh *mesh)
{
  MPI_Status statuses[4];
#ifdef V_T
  {
      static int scl = VT_NOSCL;
      static int exchangeend_state;

      if( scl == VT_NOSCL ) {
	  VT_scldef( __FILE__, __LINE__, &scl );
	  VT_funcdef( "ExchangeEnd", communication_class, &exchangeend_state );
      }
      VT_enter( exchangeend_state, scl );
  }
#endif
  MPI_Waitall(4, mesh->rq, statuses);
#ifdef V_T
  VT_leave( VT_NOSCL );
#endif
}



