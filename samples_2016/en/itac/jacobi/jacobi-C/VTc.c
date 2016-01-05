#include "mpi.h"
#include "jacobi.h"

#include <stdio.h>
#include <stdlib.h>
#include <VT.h>

#ifdef V_T

/* Variables for different classes */
int application_class, setup_class, communication_class;
int calculation_class, i_o_class;  
int counterhandle;

void
setup_tracing(void)
{
  /**
   * Define handles for groups 
   **/

  int *grouphandles, i, numprocs, numgroups, grouphandle, *processes;
  int numodd, numeven, oddeven[2];
  char buffer[120];
  static const double boundaries[2] = { 0, 1e10 };

  /**
   * Define different classes for symbols
   **/

  VT_classdef( "Application" , &application_class );
  VT_classdef( "Setup" , &setup_class );
  VT_classdef( "Communication" , &communication_class );
  VT_classdef( "Calculation", &calculation_class );
  VT_classdef( "I/O", &i_o_class );

  /**
   * A counter that logs the criteria for aborting the calculation.
   */

  VT_countdef( "diffnorm", application_class,
	       VT_COUNT_FLOAT|VT_COUNT_ABSVAL|VT_COUNT_VALID_POINT,
	       VT_GROUP_PROCESS, boundaries, "#", &counterhandle );

  /**
   * define groups that contain pairs of consecutive processes (0+1, 2+3, ...)
   * and put these groups into the group "pairs"
   */

  MPI_Comm_size( MPI_COMM_WORLD, &numprocs );
  numgroups = ( numprocs + 1 ) / 2;
  grouphandles = malloc( sizeof( *grouphandles ) * numgroups );
  for( i = 0; i < numgroups; i++ ) {
      int processes[2];
      int numentries = 1;

      VT_getthreadid( i * 2, 0 /* we have no threads */,
		      processes + 0 );
      /* we might have uneven number or processes */
      if( i * 2 + 1 < numprocs ) {
	  VT_getthreadid( i * 2 + 1, 0, processes + 1 );
	  numentries++;
      }
      
      sprintf( buffer, "pair#%d", i );
      VT_groupdef( buffer, numentries, processes, grouphandles + i );
  }
  VT_groupdef( "pairs", i, grouphandles, &grouphandle );
  free( grouphandles );

  /**
   * define a group "odd_even" that contains a group with
   * all even processes and another with all odd ones
   */
  numeven = ( numprocs + 1 ) / 2;
  numodd = numprocs - numeven;
  processes = malloc( sizeof( *processes ) * numeven );

  for( i = 0; i < numprocs; i += 2 ) {
      VT_getthreadid( i, 0, processes + i / 2 );
  }
  VT_groupdef( "even", numeven, processes, oddeven + 0 );

  for( i = 1; i < numprocs; i += 2 ) {
      VT_getthreadid( i, 0, processes + i / 2 );
  }
  VT_groupdef( "odd", numodd, processes, oddeven + 1 );

  VT_groupdef( "oddeven", 2, oddeven, &grouphandle );
  free( processes );
}
#endif

