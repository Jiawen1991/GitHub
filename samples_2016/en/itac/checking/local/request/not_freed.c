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

/**
 * This example covers request leak reporting. It is expected to be run with
 * one process and CHECK-MAX-REQUESTS 4.
 */

int main (int argc, char **argv)
{
    int i, e;
    char buffer[1024];
    int offset = 0;
    MPI_Request req;
    MPI_Request reqs[4];

    MPI_Init( &argc, &argv );
    for( i = 0; i < sizeof(buffer)/sizeof(buffer[0]); i++ ) {
        buffer[i] = (char)i;
    }

    /* generate four requests - should trigger a leak report */
    for( i = 0; i < 4; i++ ) {
        MPI_Send_init( buffer + (offset++), 1, MPI_CHAR, 0, 0, MPI_COMM_SELF, reqs + i );
    }
    /* now free one and create it again - should not trigger a report */
    MPI_Request_free( reqs + 3 );
    MPI_Send_init( buffer + (offset++), 1, MPI_CHAR, 0, 1, MPI_COMM_SELF, reqs + 3 );

    /* leak one request in various functions, all with different source/line */
    MPI_Send_init( buffer + (offset++), 1, MPI_CHAR, 0, 100, MPI_COMM_SELF, &req );
    MPI_Recv_init( buffer + (offset++), 1, MPI_CHAR, 0, 101, MPI_COMM_SELF, &req );
    MPI_Isend( buffer + (offset++), 1, MPI_CHAR, 0, 102, MPI_COMM_SELF, &req );
    MPI_Irecv( buffer + (offset++), 1, MPI_CHAR, 0, 104, MPI_COMM_SELF, &req );

    /* leak with different datatypes (should be merged despite the different numerical handles) */
    for( i = 0; i < 2; i++ ) {
        MPI_Datatype type;

        MPI_Type_vector( 1, i+2, i+2, MPI_CHAR, &type );
        MPI_Type_commit( &type );
        MPI_Isend( buffer + offset, 1, type, 0, 200, MPI_COMM_SELF, &req );
        offset += i + 2;
        /*
         * also leak the type to ensure that ids are different in each loop iteration
         * MPI_Type_free( &type );
         */
    }

    /* now do the same in a loop */
    for( i = 0; i < 5; i++ ) {
        MPI_Send_init( buffer + (offset++), 1, MPI_CHAR, 0, 300, MPI_COMM_SELF, &req );
        MPI_Recv_init( buffer + (offset++), 1, MPI_CHAR, 0, 301, MPI_COMM_SELF, &req );
        MPI_Isend( buffer + (offset++), 1, MPI_CHAR, 0, 302, MPI_COMM_SELF, &req );
        for( e = 0; e < 2; e++ ) {
            MPI_Irecv( buffer + (offset++), 1, MPI_CHAR, 0, 304, MPI_COMM_SELF, &req );
        }
    }

    /* leak an active request */
    MPI_Isend( buffer + (offset++), 1, MPI_CHAR, 0, 400, MPI_COMM_SELF, &req );
    MPI_Request_free( &req );

    MPI_Finalize( );

    return 0;
}
