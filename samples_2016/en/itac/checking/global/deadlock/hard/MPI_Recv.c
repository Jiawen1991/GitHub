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
 * Every process enters an MPI_Recv() from its right neighbor, but
 * none of them sends any message -> deadlock.
 */

#include <mpi.h>
#include <string.h>
#include <stdlib.h>

int main (int argc, char **argv)
{
    int rank, size, peer;
    char send, recv;
    MPI_Status status;

    MPI_Init( &argc, &argv );
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    peer = rank + 1;
    if( peer >= size ) {
        peer = 0;
    }

    /* waiting for a message that has not been sent */
    MPI_Recv( &recv, 1, MPI_CHAR, peer, 100, MPI_COMM_WORLD, &status );

    /*
     * Too late, this code is not going to be reached.
     * Beware, simply moving this code up would rely on
     * buffering in the MPI. The correct solution is to
     * use MPI_Isend() before the receive and MPI_Wait()
     * afterwards.
     */
    send = 0;
    MPI_Send( &send, 1, MPI_CHAR, peer, 100, MPI_COMM_WORLD );

    MPI_Finalize( );

    return 0;
}
