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

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/**
 * Buffer becomes available for another buffered send only when
 * the oldest messages have been transmitted to the recipient.
 * The application developer must take care that this has happened
 * before doing a buffered send that reuses buffer.
 *
 * This program simulates a race condition by forcing the
 * recipient to wait until the second, faulty bufferend send
 * occurred at the sender.
 */

static int size, rank;

static void race( void )
{
    char *buffer;
    int bufsize;
    int chunksize_small;
    int ierr;
    MPI_Status status;
    static char msg_small[256];

    /*
     * the size of the buffer for buffered sends depends on the length of the messages
     * and a fixed overhead per message
     */
    MPI_Pack_size( sizeof(msg_small), MPI_CHAR, MPI_COMM_WORLD, &chunksize_small );
    chunksize_small += MPI_BSEND_OVERHEAD;

    /*
     * the buffer is only large enough for one message
     */
    bufsize = 1 * chunksize_small;
    if( rank == 0 ) {
        buffer = malloc( bufsize );
        ierr = MPI_Buffer_attach( buffer, bufsize );
        assert( ierr == MPI_SUCCESS );

        ierr = MPI_Bsend( msg_small,
                          sizeof(msg_small),
                          MPI_CHAR,
                          1,
                          100,
                          MPI_COMM_WORLD );

        /*
         * This call would work if the recipient received the message
         * by the time the second MPI_Bsend() is entered; the barrier
         * below ensures that this does not happen. Without the barrier
         * there would be a race condition.
         */
        ierr = MPI_Bsend( msg_small,
                          sizeof(msg_small),
                          MPI_CHAR,
                          1,
                          101,
                          MPI_COMM_WORLD );
    }

    /*
     * Without this barrier there would be a race condition. With
     * the barrier the recipient will always receive the message
     * too late.
     */
    ierr = MPI_Barrier( MPI_COMM_WORLD );

    if( rank == 1 ) {
        ierr = MPI_Recv( msg_small,
                         sizeof(msg_small),
                         MPI_CHAR,
                         0,
                         100,
                         MPI_COMM_WORLD,
                         &status );
        assert( ierr == MPI_SUCCESS );
    }
}


int main( int argc, char **argv )
{
    MPI_Init( &argc, &argv );

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Errhandler_set(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    
    if (size < 2) {
        printf("Number of processes should be at least 2.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    race();

    MPI_Finalize();

    return 0;
}

