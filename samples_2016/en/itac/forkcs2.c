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
 * This version passes the contact infos of the main process to
 * all forked subprocesses, then traces all with libVTcs.a without
 * requiring spawning support.
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

static int childmain( int procid, const char *clientname, const char *contact, int filedes[4] )
{
    char buffer[80];
    int parentcomm;
    int root;

    /* initialize child */
    CHECK( VT_clientinit( procid, clientname, &contact ), "VT_clientinit" );
    CHECK( VT_initialize( NULL, NULL ), "VT_initialize" );
    CHECK( VT_get_parent( &parentcomm ), "VT_get_parent" );
    assert( parentcomm == VT_COMM_INVALID );

    CHECK( VT_log_sendmsg( 0, 0, 100, VT_COMM_SELF, VT_NOSCL ), "VT_log_sendmsg" );
    CHECK( VT_log_recvmsg( 0, 0, 100, VT_COMM_SELF, VT_NOSCL ), "VT_log_recvmsg" );

    /* first level child, our peer has rank #0 */
    root = 0;

    readline( filedes[0], buffer );
    printf( "%s: got %s", clientname, buffer );
    CHECK( VT_log_recvmsg( root, strlen( buffer ), 0, VT_COMM_WORLD, VT_NOSCL ), "VT_log_recvmsg" );
    sprintf( buffer, "hello back from #%d\n", procid );
    CHECK( VT_log_sendmsg( root, strlen( buffer ), 1, VT_COMM_WORLD, VT_NOSCL ), "VT_log_sendmsg" );
    write( filedes[3], buffer, strlen( buffer ) );

    CHECK( VT_finalize(), "VT_finalize" );
    return 0;
}




int main( int argc, char **argv )
{
    const char *contact = NULL;
    int parentcomm;
    int procid, numchildren = 2;
#define MAXCHILDREN 256
    int filedes[MAXCHILDREN][4];
    char contactcopy[128];

    if( argc > 1 ) {
        numchildren = atoi( argv[1] );
    }

    /* this process is started first and initializes itself as the server who waits for clients */
    CHECK( VT_serverinit( "master", numchildren + 1, NULL, &contact ), "VT_serverinit" );

    /* make a copy suitable for use in VT_clientinit (with S prefix) */
    sprintf( contactcopy, "S%s", contact );

    /* fork two processes */
    for( procid = 0; procid < numchildren; procid++ ) {
	char clientname[256];
	pid_t childpid;

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

	    sprintf( clientname, "child %d", procid );
	    exit( childmain( procid + 1, clientname, contactcopy, filedes[procid] ) );
	    break;
	}
    }

    /* master process continues normal initialization */
    CHECK( VT_initialize( &argc, &argv ), "VT_initialize" );
    /* assert( argc == 1 ); */
    CHECK( VT_get_parent( &parentcomm ), "VT_get_parent" );
    assert( parentcomm == VT_COMM_INVALID );

    /* talk to children */
    for( procid = 0; procid < numchildren; procid++ ) {
	char buffer[80];

	sprintf( buffer, "hello child #%d\n", procid );
	CHECK( VT_log_sendmsg( procid + 1, strlen( buffer ), 0, VT_COMM_WORLD, VT_NOSCL ), "VT_log_sendmsg" );
	write( filedes[procid][1], buffer, strlen( buffer ) );
	readline( filedes[procid][2], buffer );
	printf( "master from %d: got %s", procid, buffer ); 
	CHECK( VT_log_recvmsg( procid + 1, strlen( buffer ), 1, VT_COMM_WORLD, VT_NOSCL ), "VT_log_recvmsg" );
    }

    /* write trace */
    CHECK( VT_finalize(), "VT_finalize" );
    return 0;
}
