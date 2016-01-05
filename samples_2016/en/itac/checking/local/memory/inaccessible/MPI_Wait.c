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
 * Does a large MPI_Isend() with a dynamically allocated buffer, frees
 * it as soon as the peer has received the chunk and only then calls
 * MPI_Wait().
 *
 * This makes the memory inaccessible in MPI_Wait() but happens to
 * work with most MPI although it is incorrect. Because it is not
 * guaranteed that libc actually unmaps the memory chunk, some of the
 * bytes are modified before free() to trigger an illegal modification
 * error if the memory is still available.
 */

#include <mpi.h>
#include <stdlib.h>
#include <string.h>

int main (int argc, char **argv)
{
    int rank, size, peer;
    MPI_Request req;
    MPI_Status status;

    /* make this very large so that libc uses mmap() in malloc() and unmap() in free() */
    int buffersize = 1024 * 1024 * 100;
    char *send, *recv;

    MPI_Init( &argc, &argv );
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    peer = rank ^ 1;

    send = malloc( buffersize );
    memset( send, 2, buffersize );
    recv = malloc( buffersize );

    if( peer < size ) {
        MPI_Isend( send, buffersize, MPI_CHAR, peer, 100, MPI_COMM_WORLD, &req );
        MPI_Recv( recv, buffersize, MPI_CHAR, peer, 100, MPI_COMM_WORLD, &status );
    }
    MPI_Barrier( MPI_COMM_WORLD );

    /*
     * modifying the send buffer will either trigger LOCAL:MEMORY:ILLEGAL_MODIFICATION
     * (if the memory is still accessible after the free()) or LOCAL:MEMORY:INACCESSIBLE
     * (if free() unmaps it)
     */
    send[0] = 255;
    free( send );
    free( recv );

    if( peer < size ) {
        MPI_Wait( &req, &status );
    }

    MPI_Finalize( );

    return 0;
}
