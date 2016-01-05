#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mpi.h"
#include "jacobi.h"

void 
Setup_mesh(int maxm, int maxn, Mesh *mesh)
{
  int       false = 0;
  int       true = 1;
  /* For some items, it is helpful to have local copies */
  int       lrow, rank, size;

#ifdef V_T
  {
      static int scl = VT_NOSCL;
      static int setupmesh_state;
      
      if( scl == VT_NOSCL ) {
	  VT_scldef( __FILE__, __LINE__, &scl );
	  VT_funcdef( "Setup_mesh", setup_class, &setupmesh_state );	
      }
      VT_enter( setupmesh_state, scl );
  }
#endif

  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Cart_create(MPI_COMM_WORLD, 1, &size, &false, true,
		  &mesh->ring_comm);
  MPI_Cart_shift(mesh->ring_comm, 0, 1, &mesh->down_nbr, &mesh->up_nbr);
  MPI_Comm_rank(mesh->ring_comm, &rank);
  MPI_Comm_size(mesh->ring_comm, &size);

  /* Number of local rows */
  lrow = (maxn - 2) / size;
  if (rank < ((maxn - 2) % size))
    lrow++;

  mesh->lrow = lrow;
  mesh->maxm = maxm;
  mesh->maxn = maxn;

  /* Allocate the local meshes */
  mesh->xlocal =
    (double *) malloc(maxm * (lrow + 2) * sizeof(double));
  mesh->xnew =
    (double *) malloc(maxm * (lrow + 2) * sizeof(double));
  if (!mesh->xlocal || !mesh->xnew) {
    fprintf(stderr, "Unable to allocate local mesh\n");
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
#ifdef V_T
  VT_leave( VT_NOSCL );
#endif
 
}

void 
Init_mesh(Mesh *mesh)
{
  int       i, j, lrow, rank, maxm;
  register double *xlocal, *xnew;

  xlocal = mesh->xlocal;
  xnew = mesh->xnew;
  lrow = mesh->lrow;
  maxm = mesh->maxm;

#ifdef V_T
  {
      static int scl = VT_NOSCL;
      static int init_mesh_state;
      if( scl == VT_NOSCL) {
	  VT_scldef( __FILE__, __LINE__, &scl );
	  VT_funcdef( "Init_mesh", setup_class, &init_mesh_state );
      }
      VT_enter( init_mesh_state, scl );
  }
#endif
  MPI_Comm_rank(mesh->ring_comm, &rank);

  /* Fill the data as specified */
  for (i = 1; i <= lrow; i++)
    for (j = 0; j < maxm; j++) {
      xlocal[i * maxm + j] = rank;
      xnew[i * maxm + j] = rank;
    }
  /* Boundary data or initial values in ghost cells */
  for (j = 0; j < maxm; j++) {
    xlocal[j] = -1;
    xlocal[(lrow + 1) * maxm + j] = rank + 1;
    xnew[j] = -1;
    xnew[(lrow + 1) * maxm + j] = rank + 1;
  }
  /* Boundary data in xnew if we use the "swap" approach */
  for (i = 1; i <= lrow; i++) {
    xnew[i * maxm] = rank;
    xnew[i * maxm + maxm - 1] = rank;
  }
#ifdef V_T
  VT_leave( VT_NOSCL );
#endif 
}

