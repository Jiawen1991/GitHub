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
 * The buffer of a non-blocking send started with MPI_Issend() is
 * modified, most likely before MPI gets a chance to transmit the
 * original data.
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
    peer = rank ^ 1;

    if( peer < size ) {
        char send;
        char recv;
        MPI_Request reqs[2];
        MPI_Status statuses[2];

        /*
         * synchronous send: sender must post a receive before the
         * request can complete
         */
        send = 0;
        MPI_Issend( &send, 1, MPI_CHAR, peer, 100, MPI_COMM_WORLD, reqs + 0 );

        /*
         * error: send buffer is in use by MPI and must not be touched
         *
         * Usually it is uncertain whether the original or the
         * modified data gets transmitting, depending on
         * implementation and timing. Because of the synchronous send
         * most likely the modified data gets sent.
         * 
         * In that case checking not only finds a locally modified
         * send buffer, but also reports a corrupted transmission
         * because the received data is different from the one
         * originally scheduled for sending.
         */
        send = 1;

        /* ensure that buffer is modified before posting the receive */
        MPI_Barrier( MPI_COMM_WORLD );
        MPI_Irecv( &recv, 1, MPI_CHAR, peer, 100, MPI_COMM_WORLD, reqs + 1 );

        MPI_Waitall( 2, reqs, statuses );
    } else {
        MPI_Barrier( MPI_COMM_WORLD );
    }

    MPI_Finalize( );

    return 0;
}
