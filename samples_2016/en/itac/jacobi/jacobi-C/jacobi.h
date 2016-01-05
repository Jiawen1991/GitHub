/*
 * This example handles an arbitrary sized mesh, with DEFAULT_MAXN the
 * default size
 */
#define DEFAULT_MAXN (128 + 2)

#ifdef V_T
#include "VT.h"

/* VT-API calls version of library at time of instrumentation */
#define MYVERSION 3101
/* prevent us from using calls that might have changed in future versions */
#if VT_VERSION_COMPATIBILITY > MYVERSION
# error Version conflict: uses old API calls that are not to compatible with your version of ITC
#endif


/* Variables for definitions that are done once in VTc.c */
extern  int application_class, setup_class, communication_class;
extern  int calculation_class, i_o_class;
extern  int counterhandle;

void setup_tracing(void);		/* Intel(R) Trace Collector instrumentation setup (VT.c) */
#endif

typedef struct {
  double   *xlocal, *xnew;
  int       maxm, maxn, lrow;
  int       up_nbr, down_nbr;
  MPI_Comm  ring_comm;
  MPI_Request rq[4];
}         Mesh;

/* Forward refs */
void Setup_mesh(int, int, Mesh *);
void Init_mesh(Mesh *);
void Get_command_line (int rank, int argc, char **argv ,
		       int *maxm , int *maxn, int *doprint,
		       int *maxit);
void ExchangeInit(Mesh *mesh);
/* void Exchange(Mesh *mesh); */
void ExchangeStart(Mesh *mesh);
void ExchangeEnd(Mesh *mesh);









