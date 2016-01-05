/***************************************************************************************
 *
 * Intel(R) Trace Collector Library.
 *
 * Copyright 2004-2008 Intel Corporation.
 * All Rights Reserved.
 * 
 * The source code contained or described herein and all documents
 * related to the source code ("Material") are owned by Intel
 * Corporation or its suppliers or licensors. Title to the Material
 * remains with Intel Corporation or its suppliers and licensors. The
 * Material contains trade secrets and proprietary and confidential
 * information of Intel or its suppliers and licensors. The Material
 * is protected by worldwide copyright and trade secret laws and
 * treaty provisions. No part of the Material may be used, copied,
 * reproduced, modified, published, uploaded, posted, transmitted,
 * distributed, or disclosed in any way without Intel's prior express
 * written permission.
 * 
 * No license under any patent, copyright, trade secret or other
 * intellectual property right is granted to or conferred upon you by
 * disclosure or delivery of the Materials, either expressly, by
 * implication, inducement, estoppel or otherwise. Any license under
 * such intellectual property rights must be express and approved by
 * Intel in writing.
 * 
 ***************************************************************************************/

/*
 * This examples uses the same buffer in several different
 * non-blocking sends at the same time. The MPI standard does not
 * allow that because some MPI implementations might have to
 * manipulate the memory.
 * 
 * Because a violation of this rule has no negative consequences with
 * most MPI implementations and it is convenient, many applications
 * contain the same error. They should be fixed to increase
 * portability.
 */

#include <mpi.h>
#include <string.h>
#include <stdlib.h>

int main (int argc, char **argv)
{
    int rank, size, peer;

    MPI_Init( &argc, &argv );
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    if( !rank ) {
        char send = 0;
        MPI_Request *reqs;
        MPI_Status *statuses;

        reqs = malloc( (size - 1) * sizeof(reqs[0]) );
        statuses = malloc( (size - 1) * sizeof(statuses[0]) );
        for( peer = 1; peer < size; peer++ ) {
            /* warning: reuses the same buffer as previous iterations */
            MPI_Isend( &send, 1, MPI_CHAR, peer, 100, MPI_COMM_WORLD, reqs + peer - 1 );
        }
        MPI_Waitall( size - 1, reqs, statuses );
        free( reqs );
        free( statuses );
    } else {
        char recv;
        MPI_Status status;

        MPI_Recv( &recv, 1, MPI_CHAR, 0, 100, MPI_COMM_WORLD, &status );
    }

    MPI_Finalize( );

    return 0;
}
