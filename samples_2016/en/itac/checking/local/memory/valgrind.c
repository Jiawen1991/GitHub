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
 * When run under valgrind, but without MPI Correctness Checking, this
 * example demonstrates a false positive (an unneeded warning about
 * writing uninitialized data) and a false negative (the recipient
 * prints random data and normally valgrind would flag that as an
 * error).
 *
 * When run under valgrind with MPI Correctness Checking the expected
 * report at the recipient is printed by valgrind.
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

volatile int tmp;

struct partly {
    int initialized;
    int uninitialized;
};

/**
 * triggers a valgrind warning if the parameter is uninitialized
 */
static const char *checkdefined( int val )
{
    const char *res = val < 0 ? "< 0" : ">= 0";
    sleep(1);
    return res;
}

int main( int argc, char **argv )
{
    volatile struct partly partly_initialized;
     MPI_Status status;
    MPI_Request req;
    int rank, size;
    const int root = 0;

    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Comm_size( MPI_COMM_WORLD, &size );

    /*
     * only one field gets initialized on non-root processes,
     * the root process will be in the same state after the
     * receive
     */
    if( rank || size == 1 ) {
        partly_initialized.initialized = 1;
    }

    /*
     * send two ints of which only the first one is initialized:
     * as long as the recipient only uses that int, everything is okay
     */
    if( rank == 1 ) {
        printf( "sending uninitialized data: %s\n\n", checkdefined(partly_initialized.uninitialized) );
        MPI_Isend( (void *)&partly_initialized, 2, MPI_INT, 0, 100, MPI_COMM_WORLD, &req );

        /*
         * MPI now owns the send buffer. Even reading it violates the
         * MPI standard, because an MPI implementation is allowed to modify
         * the buffer in place.
         *
         * The following read and write both should trigger an error message.
         * Because the same value is written back into the buffer, this problem
         * will not manifest itself in checksum errors.
         */
        tmp = partly_initialized.initialized;
        partly_initialized.initialized = tmp;

        MPI_Wait( &req, &status );
    } else if( rank == 0  && size > 1 ) {
        MPI_Recv( (void *)&partly_initialized, 2, MPI_INT, 1, 100, MPI_COMM_WORLD, &status );

        /* okay */
        printf( "got data which was initialized by sender: %d\n\n", partly_initialized.initialized );

        /* not okay */
        printf( "got data which was not initialized by sender: %s\n\n", checkdefined(partly_initialized.uninitialized) );
    }

    /* transmit partially initialized data via collective operations: from root to everyone else */
    MPI_Bcast( (void *)&partly_initialized,
               2, MPI_INT,
               root, MPI_COMM_WORLD );
    printf( "bcast data, initialized: %d\n\n", partly_initialized.initialized );
    printf( "bcast data, not initialized: %s\n\n", checkdefined(partly_initialized.uninitialized) );

    MPI_Finalize();

    return 0;
}
