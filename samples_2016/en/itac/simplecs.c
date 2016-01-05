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

#include "config.h"
#include <mpi.h>
#include <VT.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

/*
 * Running different executables as one MPI application is difficult,
 * therefore we execute one uniform executable which acts differently
 * depending on the rank within the MPI application:
 * - the process with rank 0 behaves like a server
 * - all others behave like a client
 *
 * A client calls
 * - VT_clientinit()
 * - sends contact infos to server
 * - VT_initialize()
 * - traces functions
 * - VT_finalize()
 *
 * At the same time the server
 * - gathers contact infos
 * - executes VTserver
 *
 * The MPI application executes the blocking VT API functions in the
 * main thread and VTserver is also executed in a blocking way. Error
 * conditions are not handled at all - please refer to clientserver.c
 * for a more complete example.
 *
 */

/**
 * Time stamps in the STF count clock ticks.
 * The length of one clock tick is defined in the header
 * of the file.
 */
static double clockperiod;

/**
 * This callback gets the clock period when reading STF.
 *
 * The custom pointer is chosen by us when starting the STF request.
 */
static int clockcallback( void *custom, F8 clkperiod )
{
    clockperiod = clkperiod;
    return 0; /* no error, continue reading */
}

/**
 * This callback gets the data when reading STF - we can ignore the
 * additional region id and sequence number, the interesting part
 * is the opaque data.
 */
static int datacallback( void *custom, U8 time, U4 willy, U4 region, U4 seq_nr, 
			 U4 data_size, const void *data, U4 scl )
{
    printf( "Process %d, Thread %d, Time %f: ",
	    willy & 0xFFFF, willy >> 16, /* willy is an artificial name for an entity
					    that logs events; in our case an entity is a
					    thread described by its process and thread number */
	    (I8)time * clockperiod /* must explicitly cast to I8 first, or VisualC will not compile it */ );
    if( data_size > 0 && isprint( *(char *)data ) ) {
	/* we know that some of our data is a real string */
	printf( "%s\n", (char *)data );
    } else {
	printf( "%d bytes\n", data_size );
    }

    return 0; /* no error, continue reading */
}


int main( int argc, char **argv )
{
    int rank, numclients;
    char clientname[MPI_MAX_PROCESSOR_NAME + 1];
    int i, maxiteration = 2, maxdata = 10000;
    MPI_Comm clients;

    MPI_Init( &argc, &argv );

    if( argc > 1 ) {
	maxiteration = atoi( argv[1] );
    }
    if( argc > 2 ) {
	maxdata = atoi( argv[2] );
    }

    /* any process with rank > 0 is a client, rank == 0 is the server */
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Comm_size( MPI_COMM_WORLD, &numclients );
    numclients--;
    sprintf( clientname, "client_%d", rank );

    /*
     * build a communicator which contains only the clients,
     * in the same order as in the original communicator
     */
    MPI_Comm_split( MPI_COMM_WORLD, rank != 0, rank, &clients );

    /*
     * Iterate over VT initialization, tracing, finalize several times.
     * The first two iteration produce a very short file, the (optional)
     * third one a larger one.
     */
    for( i = 1; i <= maxiteration; i++ ) {
	char *buffer = NULL;
	int size;
	int success;

	MPI_Barrier( MPI_COMM_WORLD );

        /* server code on the left,
                         client on the right */

	if( rank ) {
	                            const char *contact = NULL;

				    /* initialize locally */
				    VT_clientinit( rank, clientname, &contact );
				    /* send contact infos to server */
	                            MPI_Send( (void *)contact, strlen( contact ) + 1,
					      MPI_CHAR, 0, 0, MPI_COMM_WORLD );
	} else {
	    int i;
	    MPI_Status status;

	    /* setup beginning of command line */
	    size = 80 + 80 * numclients;
	    buffer = malloc( size );
	    strcpy( buffer, "VTserver" );

	    /* gather contact infos and add it to the command line */
	    for( i = 0; i < numclients; i++ ) {
		int len = strlen(buffer);

		buffer[len++] = ' ';
		MPI_Recv( &buffer[len],
			  size-len,
			  MPI_CHAR, i + 1, 0,
			  MPI_COMM_WORLD, 
			  &status );
	    }
	}

	if( rank ) {
	                            const char *string = "hello world";
				    unsigned char data[1024], val;
				    int peer, numprocs;
				    int e, maxe = i > 2 ? maxdata : 1;
				    MPI_Status status;

	                            /* initialize globally */
	                            VT_initialize( &argc, &argv );

				    /* fill in dummy data */
				    for( e = 0, val = 0; e < sizeof( data ); e++, val++ ) {
					data[e] = val;
				    }

				    /* optionally iterate over data generation several times */
				    for( e = 1; e <= maxe; e++ ) {
					/*
					 * log a string, which is treated as
					 * opaque data by ITC, i.e. stored
					 * as it is
					 */
					VT_log_data( string, strlen(string) + 1, VT_NOSCL );

					/* the same with bytes from 0..255 */
					VT_log_data( data, sizeof(data), VT_NOSCL );


					/*
					 * log a message sent to process itself via VT_COMM_SELF
					 */
					VT_log_sendmsg( 0, 1, 1001, VT_COMM_SELF, VT_NOSCL );
					VT_log_recvmsg( 0, 1, 1001, VT_COMM_SELF, VT_NOSCL );

					/*
					 * exchange messages with process
					 * whose rank differs by one;
					 * process 0 must be excluded
					 */
					peer = (rank & 1) ? rank + 1 : rank - 1;
					MPI_Comm_size( MPI_COMM_WORLD, &numprocs );
					if( peer > 0 && peer < numprocs ) {
					    /*
					     * Send from process with lower rank first,
					     * then let the one with the higher rank reply.
					     * Always log the send/receive operations.
					     * Switch these roles for each iteration.
					     */
					    if( rank < peer ) {
						VT_log_sendmsg( peer, 0, e, VT_COMM_WORLD, VT_NOSCL );
						MPI_Send( NULL, 0, MPI_CHAR, peer, e, MPI_COMM_WORLD );
						MPI_Recv( NULL, 0, MPI_CHAR, peer, e, MPI_COMM_WORLD, &status );
						VT_log_recvmsg( status.MPI_SOURCE, 0, status.MPI_TAG,
								VT_COMM_WORLD, VT_NOSCL );
					    } else {
						MPI_Recv( NULL, 0, MPI_CHAR, peer, e, MPI_COMM_WORLD, &status );
						VT_log_recvmsg( status.MPI_SOURCE, 0, status.MPI_TAG,
								VT_COMM_WORLD, VT_NOSCL );
						VT_log_sendmsg( peer, 0, e, VT_COMM_WORLD, VT_NOSCL );
						MPI_Send( NULL, 0, MPI_CHAR, peer, e, MPI_COMM_WORLD );
					    }
					}
				    }

				    /* send trace data to server */
				    VT_finalize();
	} else {
	    int res;
	    char file[80];

	    /* write a different file in each iteration */
	    sprintf( file, "simplecs_%d.stf", i );

	    /* tell VTserver to initialize and write trace */
	    sprintf( &buffer[strlen(buffer)],
		     " --logfile-name %s", file );
	    fprintf( stderr, "executing %s\n", buffer );
	    fflush( stderr );
	    res = system( buffer );
	    fprintf( stderr, "VTserver returned %d\n", res );
	    success = !res;
	}

	if( buffer ) {
	    free( buffer );
	}
    }

    MPI_Comm_free( &clients );
    MPI_Finalize();

    return 0;
}
