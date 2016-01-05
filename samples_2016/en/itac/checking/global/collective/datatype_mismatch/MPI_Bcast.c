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
 * Root uses a different datatype than the receiving processes in an
 * MPI_Bcast(). The number of transmitted bytes is the same, so
 * normally this error is not detected by MPI.
 */

#include <mpi.h>
#include <string.h>
#include <stdlib.h>

int main (int argc, char **argv)
{
    int rank, size;

    MPI_Init( &argc, &argv );
    MPI_Comm_size( MPI_COMM_WORLD, &size );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    /* error: types do not match */
    if( !rank ) {
        int send = 0;
        MPI_Bcast( &send, 1, MPI_INT, 0, MPI_COMM_WORLD );
    } else {
        char recv[4];
        MPI_Bcast( &recv, 4, MPI_CHAR, 0, MPI_COMM_WORLD );
    }

    MPI_Finalize( );

    return 0;
}
