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

/**
 * This program triggers various warnings and errors. It can be used
 * with different configurations to test the error handling and
 * reporting. Interesting configurations, specified as environment
 * variables here:
 * - one instance of each error at most
 *   VT_CHECK_SUPPRESSION_LIMIT=1
 * - unlimited instances
 *   VT_CHECK_SUPPRESSION_LIMIT=0
 * - abort at first report, regardless what it is
 *   VT_CHECK_MAX_REPORTS=1
 * - keep running even when MPI calls fail
 *   VT_CHECK_MAX_ERRORS=0
 * - disable msg type mismatch
 *   VT_CHECK="GLOBAL:MSG:DATATYPE:MISMATCH off"
 * - disable anything that requires extra messages
 *   VT_CHECK="GLOBAL:MSG:** off"
 */

#include <mpi.h>

int main (int argc, char **argv)
{
    int rank, size, peer;
    int i;
    char send = 0, recv;
    MPI_Status status, statuses[2];
    MPI_Request reqs[2], req;

    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    peer = rank ^ 1;

    /* 20 overlap warnings */
    if( peer < size ) {
        for( i = 0; i < 20; i++ ) {
            if( rank & 1 ) {
                MPI_Recv( &recv, 1, MPI_CHAR, peer, 100, MPI_COMM_WORLD, &status );
                MPI_Recv( &recv, 1, MPI_CHAR, peer, 100, MPI_COMM_WORLD, &status );
            } else {
                MPI_Isend( &send, 1, MPI_CHAR, peer, 100, MPI_COMM_WORLD, reqs + 0 );
                MPI_Isend( &send, 1, MPI_CHAR, peer, 100, MPI_COMM_WORLD, reqs + 1 );
                MPI_Waitall( 2, reqs, statuses );
            }
        }
    }

    MPI_Barrier( MPI_COMM_WORLD );

    /* warning: free an invalid request */
    req = MPI_REQUEST_NULL;
    MPI_Request_free( &req );

    MPI_Barrier( MPI_COMM_WORLD );

    /* 15 errors: send to invalid recipient */
    for( i = 0; i < 15; i++ ) {
        MPI_Send( &send, 1, MPI_CHAR, -1, 200, MPI_COMM_WORLD );
    }

    MPI_Barrier( MPI_COMM_WORLD );

    /* datatype mismatch */
    if( peer < size ) {
        if( rank & 1 ) {
            int recvint;

            MPI_Recv( &recvint, 1, MPI_INT, peer, 300, MPI_COMM_WORLD, &status );
        } else {
            MPI_Send( &send, 1, MPI_CHAR, peer, 300, MPI_COMM_WORLD );
        }
    }

    MPI_Barrier( MPI_COMM_WORLD );

    /* collop mismatch */
    if( rank & 1 ) {
        MPI_Barrier( MPI_COMM_WORLD );
    } else {
        MPI_Reduce( &send, &recv, 1, MPI_BYTE, MPI_SUM, 0, MPI_COMM_WORLD );
    }

    MPI_Barrier( MPI_COMM_WORLD );

    /* root mismatch */
    MPI_Reduce( &send, &recv, 1, MPI_BYTE, MPI_SUM, rank ? 0 : 1, MPI_COMM_WORLD );

    MPI_Barrier( MPI_COMM_WORLD );

    /* create unfreed requests */
    if( peer < size ) {
        for( i = 0; i < 2; i++ ) {
            MPI_Send_init( &send, 1, MPI_CHAR, peer, 300, MPI_COMM_WORLD, &req );
        }
    }

    MPI_Finalize( );

    return 0;
}
