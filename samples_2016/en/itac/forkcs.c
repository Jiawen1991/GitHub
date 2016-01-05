/*******************************************************************
 * 
 * Copyright 2004-2007 Intel Corporation.
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
 *******************************************************************/

/**
 * This test program uses libVTcs.a and dynamic attach/spawn to trace
 * forked subprocesses.
 */

#include <VT.h>

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "testutil.h"

/* utility function that reads a whole line */
static void readline( int fd, char *buffer )
{
    int n = 0;
    do {
	switch( read( fd, buffer + n, 1 ) ) {
	case 0:
	    break;
	case 1:
	    n++;
	    break;
	default:
	    perror( "read()" );
	    exit(1);
	    break;
	}
    } while( !n || buffer[n-1] != '\n' );
    buffer[n] = 0;
}

static void spawn( int root, int comm, const char *prefix, int numchildren, const char *contact, int dospawn );

static int childmain( int procid, const char *clientname, const char *contact, int filedes[4], int dospawn )
{
    char buffer[80];
    int parentcomm;
    int root;

    /* initialize child */
    CHECK( VT_clientinit( procid, clientname, &contact ), "VT_clientinit" );
    CHECK( VT_initialize( NULL, NULL ), "VT_initialize" );
    CHECK( VT_get_parent( &parentcomm ), "VT_get_parent" );
    assert( parentcomm != VT_COMM_INVALID );

    CHECK( VT_log_sendmsg( 0, 0, 100, VT_COMM_SELF, VT_NOSCL ), "VT_log_sendmsg" );
    CHECK( VT_log_recvmsg( 0, 0, 100, VT_COMM_SELF, VT_NOSCL ), "VT_log_recvmsg" );

    if( dospawn ) {
	/* recursive spawn, with child #1 as new parent */
	spawn( 1, VT_COMM_WORLD, "second level", 3, contact, 0 );
	root = 0;
    } else {
	/* second level child, our peer has rank #1 */
	root = 1;
    }

    readline( filedes[0], buffer );
    printf( "%s: got %s", clientname, buffer );
    CHECK( VT_log_recvmsg( root, strlen( buffer ), 0, parentcomm, VT_NOSCL ), "VT_log_recvmsg" );
    sprintf( buffer, "hello back from #%d\n", procid );
    CHECK( VT_log_sendmsg( root, strlen( buffer ), 1, parentcomm, VT_NOSCL ), "VT_log_sendmsg" );
    write( filedes[3], buffer, strlen( buffer ) );

    CHECK( VT_finalize(), "VT_finalize" );
    return 0;
}




/* this function forks several times and then attaches all forked processes */
static void spawn( int root, int comm, const char *prefix, int numchildren, const char *contact, int dospawn )
{
    int procid;
#define MAXCHILDREN 16
    int filedes[MAXCHILDREN][4];
    int childcomm;
    int rank, size;

    CHECK( VT_comm_rank( comm, &rank ), "VT_comm_rank" );
    CHECK( VT_comm_size( comm, &size ), "VT_comm_size" );

    if( root == rank ) {
	for( procid = 0; procid < numchildren; procid++ ) {
	    char clientname[256];
	    pid_t childpid;
            int res;

	    /* create two pipes per child for communication */
	    if( pipe( &filedes[procid][0] ) ||
		pipe( &filedes[procid][2] ) ) {
		perror( "pipe()" );
		exit(1);
	    }

	    childpid = fork();
	    switch( childpid ) {
	    case -1:
		/* fork failed */
		perror( "fork()" );
		exit(1);
		break;
	    case 0:
#if 0
		{
		    int i;
		
		    for( i = 0; i < 10; i++ ) {
			sleep( 2 );
		    }
		}
#endif

		sprintf( clientname, "%s %d", prefix, procid );
		res = childmain( procid, clientname, contact, filedes[procid], dospawn );
                exit( res );
		break;
	    }
	}
    }

    /* attach to children and then exchange messages */
    VT_attach( root, comm,
	       numchildren, &childcomm );
    if( root == rank ) {
	for( procid = 0; procid < numchildren; procid++ ) {
	    char buffer[80];

	    sprintf( buffer, "hello child #%d\n", procid );
	    CHECK( VT_log_sendmsg( procid + size, strlen( buffer ), 0, childcomm, VT_NOSCL ), "VT_log_sendmsg" );
	    write( filedes[procid][1], buffer, strlen( buffer ) );
	    readline( filedes[procid][2], buffer );
	    printf( "%s from %d: got %s", prefix, procid, buffer ); 
	    CHECK( VT_log_recvmsg( procid + size, strlen( buffer ), 1, childcomm, VT_NOSCL ), "VT_log_recvmsg" );
	}
    }
}


int main( int argc, char **argv )
{
    const char *contact = NULL;
    int parentcomm;
    const char *loaderror;

#ifdef VT_enabled
    /* VT_dynamic.h - not loaded yet */
    assert( !VT_enabled() );
#endif
    loaderror = VT_loadlibrary( "libVTcs.so" );
    if( loaderror ) {
        fprintf( stderr, "%s\n", loaderror);
        assert( !loaderror);
    }
    assert( VT_enabled() );

    /* this process is started first and initializes itself as the server with no clients */
    CHECK( VT_serverinit( "master", 0, NULL, &contact ), "VT_serverinit" );

    /* master process continues normal initialization */
    CHECK( VT_initialize( &argc, &argv ), "VT_initialize" );
    assert( argc == 1 );
    CHECK( VT_get_parent( &parentcomm ), "VT_get_parent" );
    assert( parentcomm == VT_COMM_INVALID );

    spawn( 0, VT_COMM_SELF, "first level", 2, contact, 1 );

    /* write trace */
    CHECK( VT_finalize(), "VT_finalize" );
    return 0;
}
