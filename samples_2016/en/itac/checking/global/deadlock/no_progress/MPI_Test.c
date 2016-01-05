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
 * One process uses an MPI_Isend() and then polls for completion while
 * the others use MPI_Send() directly. This happens to work for small
 * messages if the MPI buffers the data, but there is no guarantee
 * that and at which message sizes this is done.
 *
 * The polling means that there is no real deadlock because that process
 * could still resolve the situation by posting a receive.
 */

#include <mpi.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#ifndef _WIN32
# include <unistd.h>
#else
# include <windows.h>
# define sleep( n ) Sleep( 1000 * (n) )
#endif

int main (int argc, char **argv)
{
    int rank, size, to, from;
    char send, recv;
    MPI_Status status;

    MPI_Init( &argc, &argv );
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    to = rank + 1;
    if( to >= size ) {
        to = 0;
    }
    from = rank - 1;
    if( from < 0 ) {
        from = size - 1;
    }

    /* sending will block unless MPI buffers the message */
    send = 0;
    if( !rank ) {
        MPI_Request req;
        int flag;

        MPI_Isend( &send, 1, MPI_CHAR, to, 100, MPI_COMM_WORLD, &req );
        do {
            sleep(1);
            MPI_Test( &req, &flag, &status );
        } while( req != MPI_REQUEST_NULL );
    } else {
        MPI_Send( &send, 1, MPI_CHAR, to, 100, MPI_COMM_WORLD );
    }

    MPI_Recv( &recv, 1, MPI_CHAR, from, 100, MPI_COMM_WORLD, &status );

    MPI_Finalize( );

    return 0;
}
