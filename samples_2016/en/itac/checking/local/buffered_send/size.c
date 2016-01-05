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
 * This program first executes a sequence of buffered send operations
 * with exactly the buffer size that is required by the MPI standard.
 * Then it repeats the same sequence using a buffer that it is just one
 * byte too small.
 *
 * In one case a large message is sent with a buffer that is way too small,
 * but even that happens to work in practice if the MPI implementation
 * decides to bypass the buffer.
 */

static int size, rank;

static void testpairs( int buffer_gap_size )
{
    char *buffer;
    int bufsize;
    int chunksize_small, chunksize_large;
    int ierr, sendierr;
    MPI_Status status;
    MPI_Request req;
    struct set_of_messages {
        unsigned int small_bsend:1;
        unsigned int large_bsend_1:1;
        unsigned int small_ibsend:1;
        unsigned int large_bsend_2:1;
        unsigned int small_bsend_init:1;
        unsigned int large_bsend_3:1;
    } set;
    static char msg_small[16], msg_large[128 * 1024];

    /* all messages have one of two different sizes */
    MPI_Pack_size( sizeof(msg_small), MPI_CHAR, MPI_COMM_WORLD, &chunksize_small );
    MPI_Pack_size( sizeof(msg_large), MPI_CHAR, MPI_COMM_WORLD, &chunksize_large );

    /* the buffer for buffered sends must also account for a fixed-sized message header */
    chunksize_small += MPI_BSEND_OVERHEAD;
    chunksize_large += MPI_BSEND_OVERHEAD;

    /* intentionally decrease buffer size */
    chunksize_small -= buffer_gap_size;
    chunksize_large -= buffer_gap_size;

    /* simple small message */
    bufsize = chunksize_small;
    if( rank == 0 ) {
        buffer = malloc( bufsize );
        ierr = MPI_Buffer_attach( buffer, bufsize );
        assert( ierr == MPI_SUCCESS );
        ierr = sendierr = MPI_Bsend( msg_small,
                                     sizeof(msg_small),
                                     MPI_CHAR,
                                     1,
                                     100,
                                     MPI_COMM_WORLD );
        printf( "buffer size %d, message tag 100, size %d: %s\n",
                bufsize, (int)sizeof(msg_small),
                sendierr == MPI_SUCCESS ? "okay" : "not sent" );
        ierr = MPI_Bcast( &sendierr, 1, MPI_INT, 0, MPI_COMM_WORLD );
        assert( ierr == MPI_SUCCESS );
        ierr = MPI_Buffer_detach( &buffer, &bufsize );
        assert( ierr == MPI_SUCCESS );
        free( buffer );
    } else {
        ierr = MPI_Bcast( &sendierr, 1, MPI_INT, 0, MPI_COMM_WORLD );
        assert( ierr == MPI_SUCCESS );
        if( rank == 1 &&
            sendierr == MPI_SUCCESS ) {
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

    /*
     * same with large message - in contrast to the small one it might be sent
     * without copying it into the buffer and thus MPI_Bsend() might succeed
     * even if the buffer is too small
     */
    bufsize = chunksize_large;
    if( rank == 0 ) {
        buffer = malloc( bufsize );
        ierr = MPI_Buffer_attach( buffer, bufsize );
        assert( ierr == MPI_SUCCESS );
        ierr = sendierr = MPI_Bsend( msg_large,
                                     sizeof(msg_large),
                                     MPI_CHAR,
                                     1,
                                     200,
                                     MPI_COMM_WORLD );
        printf( "buffer size %d, message tag 200, size %d: %s\n",
                bufsize, (int)sizeof(msg_large),
                sendierr == MPI_SUCCESS ? "okay" : "not sent" );
        ierr = MPI_Bcast( &sendierr, 1, MPI_INT, 0, MPI_COMM_WORLD );
        assert( ierr == MPI_SUCCESS );
        ierr = MPI_Buffer_detach( &buffer, &bufsize );
        assert( ierr == MPI_SUCCESS );
        free( buffer );
    } else {
        ierr = MPI_Bcast( &sendierr, 1, MPI_INT, 0, MPI_COMM_WORLD );
        assert( ierr == MPI_SUCCESS );
        if( rank == 1 &&
            sendierr == MPI_SUCCESS ) {
            ierr = MPI_Recv( msg_large,
                             sizeof(msg_large),
                             MPI_CHAR,
                             0,
                             200,
                             MPI_COMM_WORLD,
                             &status );
            assert( ierr == MPI_SUCCESS );
        }
    }

    /*
     * again a large message, but this time with the small buffer size
     * instead of just some bytes less than required
     */
    bufsize = buffer_gap_size ? chunksize_small : chunksize_large;
    if( rank == 0 ) {
        buffer = malloc( bufsize );
        ierr = MPI_Buffer_attach( buffer, bufsize );
        assert( ierr == MPI_SUCCESS );
        ierr = sendierr = MPI_Bsend( msg_large,
                                     sizeof(msg_large),
                                     MPI_CHAR,
                                     1,
                                     300,
                                     MPI_COMM_WORLD );
        printf( "buffer size %d, message tag 300, size %d: %s\n",
                bufsize, (int)sizeof(msg_large),
                sendierr == MPI_SUCCESS ? "okay" : "not sent" );
        ierr = MPI_Bcast( &sendierr, 1, MPI_INT, 0, MPI_COMM_WORLD );
        assert( ierr == MPI_SUCCESS );
        ierr = MPI_Buffer_detach( &buffer, &bufsize );
        assert( ierr == MPI_SUCCESS );
        free( buffer );
    } else {
        ierr = MPI_Bcast( &sendierr, 1, MPI_INT, 0, MPI_COMM_WORLD );
        assert( ierr == MPI_SUCCESS );
        if( rank == 1 &&
            sendierr == MPI_SUCCESS ) {
            ierr = MPI_Recv( msg_large,
                             sizeof(msg_large),
                             MPI_CHAR,
                             0,
                             300,
                             MPI_COMM_WORLD,
                             &status );
            assert( ierr == MPI_SUCCESS );
        }
    }

    /* a more complex combination of different buffered sends */
    bufsize = 3 * chunksize_small + 3 * chunksize_large;
    if( rank == 0 ) {
        buffer = malloc( bufsize );
        ierr = MPI_Buffer_attach( buffer, bufsize );
        assert( ierr == MPI_SUCCESS );

        ierr = MPI_Bsend( msg_small,
                          sizeof(msg_small),
                          MPI_CHAR,
                          1,
                          400,
                          MPI_COMM_WORLD );
        set.small_bsend = ierr == MPI_SUCCESS;
        printf( "buffer size %d, small bsend message, tag 400, size %d: %s\n",
                bufsize, (int)sizeof(msg_small),
                set.small_bsend ? "okay" : "not sent" );

        ierr = MPI_Bsend( msg_large,
                          sizeof(msg_large),
                          MPI_CHAR,
                          1,
                          401,
                          MPI_COMM_WORLD );
        set.large_bsend_1 = ierr == MPI_SUCCESS;
        printf( "buffer size %d, large bsend message 1, tag 401, size %d: %s\n",
                bufsize, (int)sizeof(msg_large),
                set.large_bsend_1 ? "okay" : "not sent" );

        ierr = MPI_Ibsend( msg_small,
                           sizeof(msg_small),
                           MPI_CHAR,
                           1,
                           402,
                           MPI_COMM_WORLD,
                           &req );
        ierr = MPI_Wait( &req, &status );
        assert( ierr == MPI_SUCCESS );
        set.small_ibsend = ierr == MPI_SUCCESS;
        printf( "buffer size %d, small ibsend message, tag 402, size %d: %s\n",
                bufsize, (int)sizeof(msg_small),
                set.small_ibsend ? "okay" : "not sent" );

        ierr = MPI_Bsend( msg_large,
                          sizeof(msg_large),
                          MPI_CHAR,
                          1,
                          403,
                          MPI_COMM_WORLD );
        set.large_bsend_2 = ierr == MPI_SUCCESS;
        printf( "buffer size %d, large bsend message 2, tag 403, size %d: %s\n",
                bufsize, (int)sizeof(msg_large),
                set.large_bsend_2 ? "okay" : "not sent" );

        ierr = MPI_Bsend_init( msg_small,
                               sizeof(msg_small),
                               MPI_CHAR,
                               1,
                               404,
                               MPI_COMM_WORLD,
                               &req );
        ierr = MPI_Start( &req );
        assert( ierr == MPI_SUCCESS );
        ierr = MPI_Wait( &req, &status );
        assert( ierr == MPI_SUCCESS );
        ierr = MPI_Request_free( &req );
        assert( ierr == MPI_SUCCESS );
        set.small_bsend_init = ierr == MPI_SUCCESS;
        printf( "buffer size %d, small bsend_init message, tag 404, size %d: %s\n",
                bufsize, (int)sizeof(msg_small),
                set.small_bsend_init ? "okay" : "not sent" );

        ierr = MPI_Bsend( msg_large,
                          sizeof(msg_large),
                          MPI_CHAR,
                          1,
                          405,
                          MPI_COMM_WORLD );
        set.large_bsend_3 = ierr == MPI_SUCCESS;
        printf( "buffer size %d, large bsend message 3, tag 405, size %d: %s\n",
                bufsize, (int)sizeof(msg_large),
                set.large_bsend_3 ? "okay" : "not sent" );

        assert( sizeof(set) == sizeof(int) );
        ierr = MPI_Bcast( &set, 1, MPI_INT, 0, MPI_COMM_WORLD );
        assert( ierr == MPI_SUCCESS );

        ierr = MPI_Buffer_detach( &buffer, &bufsize );
        assert( ierr == MPI_SUCCESS );
        free( buffer );
    } else {
        ierr = MPI_Bcast( &set, 1, MPI_INT, 0, MPI_COMM_WORLD );
        assert( ierr == MPI_SUCCESS );
        if( rank == 1 ) {
            if( set.small_bsend ) {
                ierr = MPI_Recv( msg_small,
                                 sizeof(msg_small),
                                 MPI_CHAR,
                                 0,
                                 400,
                                 MPI_COMM_WORLD,
                                 &status );
                assert( ierr == MPI_SUCCESS );
            }

            if( set.large_bsend_1 ) {
                ierr = MPI_Recv( msg_large,
                                 sizeof(msg_large),
                                 MPI_CHAR,
                                 0,
                                 401,
                                 MPI_COMM_WORLD,
                                 &status );
                assert( ierr == MPI_SUCCESS );
            }

            if( set.small_ibsend ) {
                ierr = MPI_Recv( msg_small,
                                 sizeof(msg_small),
                                 MPI_CHAR,
                                 0,
                                 402,
                                 MPI_COMM_WORLD,
                                 &status );
                assert( ierr == MPI_SUCCESS );
            }

            if( set.large_bsend_2 ) {
                ierr = MPI_Recv( msg_large,
                                 sizeof(msg_large),
                                 MPI_CHAR,
                                 0,
                                 403,
                                 MPI_COMM_WORLD,
                                 &status );
                assert( ierr == MPI_SUCCESS );
            }

            if( set.small_bsend_init ) {
                ierr = MPI_Recv( msg_small,
                                 sizeof(msg_small),
                                 MPI_CHAR,
                                 0,
                                 404,
                                 MPI_COMM_WORLD,
                                 &status );
                assert( ierr == MPI_SUCCESS );
            }

            if( set.large_bsend_3 ) {
                ierr = MPI_Recv( msg_large,
                                 sizeof(msg_large),
                                 MPI_CHAR,
                                 0,
                                 405,
                                 MPI_COMM_WORLD,
                                 &status );
                assert( ierr == MPI_SUCCESS );
            }
        }
    }
}

static void rightbuffer() { testpairs( 0 ); }
static void wrongbuffer() { testpairs( 1 ); }

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
    
    /* okay */
    rightbuffer();

    /* not okay, buffer one byte too small */
    wrongbuffer();

    MPI_Finalize();

    return 0;
}

