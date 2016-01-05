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
 * The MPI standard describes that buffered sends use the memory available in
 * the buffer between the end of the active message queue and its start. Even
 * if messages in the middle of the active message queue complete, their memory
 * is not reused until all messages in front of them have also completed.
 *
 * This example uses a buffer which is large enough two store three small messages
 * or two such small messages and one larger one. First it demonstrates how the
 * buffer is used correctly, then it shows how memory cannot be reused because
 * the initial message does not complete and blocks the buffer.
 */

static int size, rank;

static void okay( void )
{
    char *buffer;
    int bufsize;
    int chunksize_small, chunksize_large;
    int ierr;
    MPI_Status status;
    struct set_of_messages {
        unsigned int sent_0:1;
        unsigned int sent_1:1;
        unsigned int sent_2:1;
        unsigned int large_sent:1;
    } set;
    
    static char msg_small[256], msg_large[512];

    /* all messages have one of two different sizes */
    MPI_Pack_size( sizeof(msg_small), MPI_CHAR, MPI_COMM_WORLD, &chunksize_small );
    MPI_Pack_size( sizeof(msg_large), MPI_CHAR, MPI_COMM_WORLD, &chunksize_large );

    /* the buffer for buffered sends must also account for a fixed-sized message header */
    chunksize_small += MPI_BSEND_OVERHEAD;
    chunksize_large += MPI_BSEND_OVERHEAD;

    /*
     * Send three small messages where the second one completes before the
     * first one. The third one is then placed after the second one,
     * making room for a large one at the beginning once the first message also
     * completes.
     */
    bufsize = 3 * chunksize_small;
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
        set.sent_0 = ierr == MPI_SUCCESS;

        ierr = MPI_Bsend( msg_small,
                          sizeof(msg_small),
                          MPI_CHAR,
                          1,
                          101,
                          MPI_COMM_WORLD );
        set.sent_1 = ierr == MPI_SUCCESS;
    }

    assert( sizeof(set) == sizeof(int) );
    ierr = MPI_Bcast( &set, 1, MPI_INT, 0, MPI_COMM_WORLD );
    assert( ierr == MPI_SUCCESS );

    if( rank == 1 ) {
        printf( "small bsend message tag 101: %s\n",
                set.sent_1 ? "okay" : "not sent" );
        if( set.sent_1 ) {
            ierr = MPI_Recv( msg_small,
                             sizeof(msg_small),
                             MPI_CHAR,
                             0,
                             101,
                             MPI_COMM_WORLD,
                             &status );
            assert( ierr == MPI_SUCCESS );
        }        
    }

    /* delay until recipient has received message 101 */
    ierr == MPI_Barrier( MPI_COMM_WORLD );
    assert( ierr == MPI_SUCCESS );

    if( rank == 0 ) {
        ierr = MPI_Bsend( msg_small,
                          sizeof(msg_small),
                          MPI_CHAR,
                          1,
                          102,
                          MPI_COMM_WORLD );
        set.sent_2 = ierr == MPI_SUCCESS;
    }

    /* delay receiver until message 102 was buffered */
    ierr == MPI_Barrier( MPI_COMM_WORLD );
    assert( ierr == MPI_SUCCESS );

    if( rank == 1 ) {
        printf( "small bsend message tag 100: %s\n",
                set.sent_0 ? "okay" : "not sent" );
        if( set.sent_0 ) {
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

    /* delay until recipient has received message 100 */
    ierr == MPI_Barrier( MPI_COMM_WORLD );
    assert( ierr == MPI_SUCCESS );

    if( rank == 0 ) {
        /* okay to buffer large message at the beginning */
        ierr = MPI_Bsend( msg_large,
                          sizeof(msg_large),
                          MPI_CHAR,
                          1,
                          103,
                          MPI_COMM_WORLD );
        set.large_sent = ierr == MPI_SUCCESS;
    }

    assert( sizeof(set) == sizeof(int) );
    ierr = MPI_Bcast( &set, 1, MPI_INT, 0, MPI_COMM_WORLD );
    assert( ierr == MPI_SUCCESS );

    if( rank == 0 ) {
        ierr = MPI_Buffer_detach( &buffer, &bufsize );
        assert( ierr == MPI_SUCCESS );
        free( buffer );
    }

    if( rank == 1 ) {
        printf( "large bsend message tag 103: %s\n",
                set.large_sent ? "okay" : "not sent" );
        if( set.large_sent ) {
            ierr = MPI_Recv( msg_large,
                             sizeof(msg_large),
                             MPI_CHAR,
                             0,
                             103,
                             MPI_COMM_WORLD,
                             &status );
            assert( ierr == MPI_SUCCESS );
        }

        printf( "small bsend message tag 102: %s\n",
                set.sent_2 ? "okay" : "not sent" );
        if( set.sent_2 ) {
            ierr = MPI_Recv( msg_small,
                             sizeof(msg_small),
                             MPI_CHAR,
                             0,
                             102,
                             MPI_COMM_WORLD,
                             &status );
            assert( ierr == MPI_SUCCESS );
        }
    }
}

static void failures( void )
{
    char *buffer;
    int bufsize;
    int chunksize_small, chunksize_large;
    int ierr;
    MPI_Status status;
    struct set_of_messages {
        unsigned int sent_0:1;
        unsigned int sent_1:1;
        unsigned int sent_2:1;
        unsigned int large_sent:1;
    } set;
    
    static char msg_small[256], msg_large[512];

    /* all messages have one of two different sizes */
    MPI_Pack_size( sizeof(msg_small), MPI_CHAR, MPI_COMM_WORLD, &chunksize_small );
    MPI_Pack_size( sizeof(msg_large), MPI_CHAR, MPI_COMM_WORLD, &chunksize_large );

    /* the buffer for buffered sends must also account for a fixed-sized message header */
    chunksize_small += MPI_BSEND_OVERHEAD;
    chunksize_large += MPI_BSEND_OVERHEAD;

    /*
     * Same messages sent as in okay(), but messages 201 and 202 are received while
     * 200 is not. So although there is enough buffer available for the large message,
     * it cannot be used yet according to the MPI standard.
     */
    bufsize = 3 * chunksize_small;
    if( rank == 0 ) {
        buffer = malloc( bufsize );
        ierr = MPI_Buffer_attach( buffer, bufsize );
        assert( ierr == MPI_SUCCESS );

        ierr = MPI_Bsend( msg_small,
                          sizeof(msg_small),
                          MPI_CHAR,
                          1,
                          200,
                          MPI_COMM_WORLD );
        set.sent_0 = ierr == MPI_SUCCESS;

        ierr = MPI_Bsend( msg_small,
                          sizeof(msg_small),
                          MPI_CHAR,
                          1,
                          201,
                          MPI_COMM_WORLD );
        set.sent_1 = ierr == MPI_SUCCESS;

        ierr = MPI_Bsend( msg_small,
                          sizeof(msg_small),
                          MPI_CHAR,
                          1,
                          202,
                          MPI_COMM_WORLD );
        set.sent_2 = ierr == MPI_SUCCESS;
    }

    assert( sizeof(set) == sizeof(int) );
    ierr = MPI_Bcast( &set, 1, MPI_INT, 0, MPI_COMM_WORLD );
    assert( ierr == MPI_SUCCESS );

    if( rank == 1 ) {
        printf( "small bsend message tag 201: %s\n",
                set.sent_1 ? "okay" : "not sent" );
        if( set.sent_1 ) {
            ierr = MPI_Recv( msg_small,
                             sizeof(msg_small),
                             MPI_CHAR,
                             0,
                             201,
                             MPI_COMM_WORLD,
                             &status );
            assert( ierr == MPI_SUCCESS );
        }        

        printf( "small bsend message tag 202: %s\n",
                set.sent_2 ? "okay" : "not sent" );
        if( set.sent_2 ) {
            ierr = MPI_Recv( msg_small,
                             sizeof(msg_small),
                             MPI_CHAR,
                             0,
                             202,
                             MPI_COMM_WORLD,
                             &status );
            assert( ierr == MPI_SUCCESS );
        }
    }

    /* delay until recipient has received messages 201 and 202 */
    ierr == MPI_Barrier( MPI_COMM_WORLD );
    assert( ierr == MPI_SUCCESS );

    if( rank == 0 ) {
        /* there shouldn't be any room for message 203 yet because message 200 is still pending */
        ierr = MPI_Bsend( msg_large,
                          sizeof(msg_large),
                          MPI_CHAR,
                          1,
                          203,
                          MPI_COMM_WORLD );
        set.large_sent = ierr == MPI_SUCCESS;
    }

    assert( sizeof(set) == sizeof(int) );
    ierr = MPI_Bcast( &set, 1, MPI_INT, 0, MPI_COMM_WORLD );
    assert( ierr == MPI_SUCCESS );
    
    if( rank == 1 ) {
        printf( "small bsend message tag 200: %s\n",
                set.sent_0 ? "okay" : "not sent" );
        if( set.sent_0 ) {
            ierr = MPI_Recv( msg_small,
                             sizeof(msg_small),
                             MPI_CHAR,
                             0,
                             200,
                             MPI_COMM_WORLD,
                             &status );
            assert( ierr == MPI_SUCCESS );
        }

        printf( "large bsend message tag 203: %s\n",
                set.large_sent ? "okay" : "not sent" );
        if( set.large_sent ) {
            ierr = MPI_Recv( msg_large,
                             sizeof(msg_large),
                             MPI_CHAR,
                             0,
                             203,
                             MPI_COMM_WORLD,
                             &status );
            assert( ierr == MPI_SUCCESS );
        }
    }

    if( rank == 0 ) {
        ierr = MPI_Buffer_detach( &buffer, &bufsize );
        assert( ierr == MPI_SUCCESS );
        free( buffer );
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
    
    okay();
    failures();

    MPI_Finalize();

    return 0;
}

