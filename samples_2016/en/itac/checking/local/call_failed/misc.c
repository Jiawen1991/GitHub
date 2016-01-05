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
 * A failed MPI call can have various reasons, the most common one
 * being invalid parameters. Depending on whether the problem is
 * expected to change the application logic such a failure is handled
 * as warning or error.
 */

#include <mpi.h>

int main (int argc, char **argv)
{
    int rank, size, peer;
    MPI_Request req = MPI_REQUEST_NULL;
    char send = 123;

    MPI_Init( &argc, &argv );
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    /* warning: freeing MPI_COMM_NULL */
    MPI_Request_free( &req );

    /* error: invalid recipient */
    peer = size;
    MPI_Send( &send, 1, MPI_CHAR, peer, 100, MPI_COMM_WORLD );

    MPI_Finalize( );

    return 0;
}
