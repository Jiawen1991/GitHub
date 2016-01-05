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

#include <mpi.h>
#include <VT.h>

#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

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
 * This MPI application actually simulates error handling and artificially
 * triggers several error conditions. It also executes VT_initialize()
 * and the VTserver program without blocking the calling process.
 *
 */


/**
 * This calls VT_initialize().
 * Can be executed as a separate thread.
 * @param done   actually points to an int which is set to 1 once we exit;
 *               it doesn't matter that this is not really thread safe,
 *               as long as the main thread eventually sees the 1
 * @return the return code of VT_initialize().
 */
static void * initialize( void *done )
{
    int ret;

    ret = VT_initialize( NULL, NULL );
    *((int *)done) = 1;
    return (void *)ret;
}


/*
 * Code for spawning and waiting for processes is system specific.
 * We use small wrapper functions around CreateProcess/fork/exec.
 */
#ifdef HAVE_WINBASE_H

# include <winbase.h>

typedef HANDLE pid_t;
#define pid_t_null NULL



/**
 * Check whether a process has terminated or not,
 * without blocking.
 *
 * @param pid   handle returned by spawn()
 * @return 0 if not terminated,
 *         1 if successful,
 *         2 if failed;
 *         in cases 1 and 2 an explanation is printed
 */
static int checkprocess( pid_t pid )
{
    DWORD status;

    if( !GetExitCodeProcess( pid, &status ) ) {
	assert( 0 );
    }
    if( status != STILL_ACTIVE ) {
	fprintf( stderr, "VTserver has terminated: return code %d\n",
		 status );
	return status ? 2 : 1;
    }
    return 0;
}

/**
 * Spawn the new process and return handle to it.
 * @return pid_t_null in case of failure, in which case an error is printed
 */
pid_t myspawn( char *cmdname, char** args )
{
    PROCESS_INFORMATION procinfo;
    STARTUPINFO startinfo;
    char *buffer, *tmp;
    int len, i;

    /* build one long command line (not quoting white spaces for simplicity) */
    for( i = 0, len = 0; args[i]; i++ ) {
	len += strlen( args[i] ) + 1;
    }
    buffer = malloc( len );
    for( i = 0, tmp = buffer; args[i]; i++ ) {
	tmp += sprintf( tmp, "%s ", args[i] );
    }

    procinfo.hProcess = NULL;
    memset( &startinfo, 0, sizeof(startinfo) );
    if( !buffer ||
	!CreateProcess( cmdname, buffer,
			NULL, NULL, FALSE, 0,
			NULL, NULL,
			&startinfo, &procinfo ) ) {
	void *errstr;

	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		       NULL, GetLastError(),
		       MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
		       (LPTSTR)&errstr, 0, NULL );
	fprintf( stderr, errstr );
	LocalFree( errstr );
	return pid_t_null;
    }

    if( buffer ) {
	free( buffer );
    }
    return procinfo.hProcess;
}

/**
 * kill the process
 */
void mykill( pid_t pid )
{
    TerminateProcess( pid, 1 );
}


#else

#include <sys/wait.h>
#include <unistd.h>

#define pid_t_null -1

static int checkprocess( pid_t pid )
{
    int status;

    if( waitpid( pid, &status, WNOHANG ) == pid ) {
	fprintf( stderr, "VTserver has terminated: " );
	if( WIFEXITED( status ) ) {
	    fprintf( stderr, "return code %d\n", WEXITSTATUS( status ) );
	    if( !WEXITSTATUS( status ) ) {
		/* terminated without error */
		return 1;
	    }
	} else if( WIFSIGNALED( status ) ) {
	    fprintf( stderr, "killed by signal %d\n", WTERMSIG( status ) );
	} else {
	    fprintf( stderr, "unknown reason for termination\n" );
	}
	return 2;
    } else {
	return 0;
    }
}

#define _P_NOWAIT 0

pid_t myspawn( char *cmdname, char **args )
{
    pid_t pid;

    pid =  fork();
    if( !pid ) {
	/* new process, exec command */
	execvp( cmdname, args );

	/* shouldn't have returned */
	perror( "execvp" );
	_exit( -VT_ERR_NOTINITIALIZED );
    }

    if( pid == -1 ) {
	perror( "fork" );
    }

    return pid;
}

void mykill( pid_t pid )
{
    kill( pid, SIGKILL );
}

#endif



int main( int argc, char **argv )
{
    int rank, numclients;
    char clientname[MPI_MAX_PROCESSOR_NAME + 1];
    int i;
    int error, abort_local, abort_global;
    MPI_Comm clients;

    MPI_Init( &argc, &argv );

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
     * We simulate different error scenarios in each iteration:
     * 1. VT_clientinit() fails on one client
     * 2. VT_initialize() fails on one client
     * 3. forking VTserver fails
     * 4. VTserver itself fails
     * 5.-6. everything works
     *
     * If MPI communication fails, then the whole MPI application is
     * aborted; a real application might want to be more robust.
     *
     * Server and clients continiously exchange status information:
     * abort == 1 means that the clients are ready to proceed to tracing,
     * abort == 2 means the iteration has to be aborted for some reason,
     * abort == 3 means the server has terminated and everything is ready to proceed
     *
     */
    for( i = 1; i <= 6; i++ ) {
	MPI_Barrier( MPI_COMM_WORLD );
	if( rank ) {
	    /* client */
	    const char *contact = NULL;
	    pthread_t thread;
	    int threadrunning = 0;
	    void *ret;
	    int done;
	    
	    if( i == 1 && rank == 1 ) {
		/* simulate a failed VT_clientinit() */
		error = VT_ERR_NOTINITIALIZED;
	    } else {
		error = VT_clientinit( rank, clientname, &contact );
	    }

	    /*
	     * check whether any process wants us to abort:
	     * MPI_Allreduce() will set abort_global to 1
	     * if any of the local abort variables is 1
	     */
	    abort_local = (error != VT_OK);
	    MPI_Allreduce( &abort_local, &abort_global, 1, MPI_INT,
			   MPI_MAX, MPI_COMM_WORLD );
	    if( abort_global ) {
		VT_reset();
		continue;
	    }

	    /* gather contact infos on root */
	    MPI_Send( (void *)contact, strlen( contact ) + 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD );

	    if( i == 2 && rank == 1 ) {
		/* simulate a failed VT_initialize() */
		error = VT_ERR_NOTINITIALIZED;
	    } else {
		/* run initialization concurrently */
		done = 0;
		pthread_create( &thread, NULL, initialize, &done );
		threadrunning = 1;
		error = VT_OK;
	    }
	    
	    /*
	     * talk with server continously until VT_finalize returns;
	     * if there is an error somewhere then abort_global
	     * will be set to 2. It is set to 1 if the clients
	     * are ready to proceed.
	     */
	    do {
		if( error == VT_OK ) {
		    /*
		     * check if our VT_initialize() has failed;
		     * if it has, then abort, else all have succeeded
		     * and we may proceed
		     */
		    if( done ) {
			pthread_join( thread, &ret );
			error = (int)ret;
			threadrunning = 0;
			if( error == VT_OK ) {
			    abort_local = 1;
			} else {
			    abort_local = 2;
			}
		    }
		} else {
		    abort_local = 2;
		}

		MPI_Allreduce( &abort_local, &abort_global, 1, MPI_INT,
			       MPI_MAX, MPI_COMM_WORLD );
	    } while( !abort_global );

	    if( abort_global > 1 ) {
		/*
		 * we ignore the error code, so it is okay to call it even if
		 * no thread is currently running
		 */
		VT_abort();
	    }

	    if( threadrunning ) {
		/* wait for spawned thread */
		pthread_join( thread, &ret );
		error = (int)ret;
	    } else {
		/* no thread spawned in the first place */
	    }

	    /* initialization may have completed even if we tried to abort */
	    if( error == VT_OK && abort_global < 2 ) {
		/*
		 * Now we could trace some data.
		 * Please see simplecs.c for examples how to actually trace data.
		 *
		 * Error handling there is missing, so in a real application
		 * one would have to check the return code of each API function.
		 * If this return code is not VT_OK, then the trace event was
		 * not logged. It might still be possible to generate a trace
		 * file, but if you do that instead of just aborting the
		 * tracing with VT_reset(), then VTserver will not indicate
		 * that the trace might be incomplete unless the collection
		 * of the incomplete data fails, too.
		 */

		/*
		 * finalize;
		 * could be placed into a separate thread similar to initialize above
		 */
		VT_finalize();
	    } else {
		VT_reset();
	    }

	    /* wait till server is ready to proceed */
	    while( abort_global < 3 ) {
		MPI_Allreduce( &abort_local, &abort_global, 1, MPI_INT,
			       MPI_MAX, MPI_COMM_WORLD );
	    }
	} else {
	    /* server */
	    char **args, *errorname;
	    int numcontacts = 0;
	    int curarg = 0, e;
	    int success = 0;
	    MPI_Status status;
	    pid_t pid;

	    fprintf( stderr, "\n\niteration %d\n", i );

	    abort_local = 0;
	    MPI_Allreduce( &abort_local, &abort_global, 1, MPI_INT,
			   MPI_MAX, MPI_COMM_WORLD );
	    if( abort_global ) {
		fprintf( stderr, "VT_clientinit() failed\n" );
		continue;
	    }

	    /*
	     * allocate argument pointer array, with enough room for:
	     * - VTserver
	     * - all contact infos
	     * - --logfile-name
	     * - clientserver<iteration>.stf
	     * - any arguments given to the application itself
	     * - NULL
	     */
	    args = malloc( (1 + numclients + 2 + 2 + argc - 1 + 1) * sizeof(*args) );
	    args[curarg++] = strdup( "VTserver" );
	    
	    /* collect contact infos */
	    while( numcontacts < numclients ) {
		int len;

		MPI_Probe( MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status );
		MPI_Get_count( &status, MPI_CHAR, &len );
		args[curarg] = malloc( len );
		MPI_Recv( args[curarg++], len, MPI_CHAR, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status );
		numcontacts++;
	    }

	    /* build command line */
	    args[curarg++] = strdup( "--logfile-name" );
	    args[curarg] = malloc( 80 );
	    sprintf( args[curarg++], "clientserver_%d.stf", i );
	    args[curarg++] = strdup( "--errorfile-name" );
	    errorname =
		args[curarg] = malloc( 80 );
	    sprintf( args[curarg++], "clientserver_%d.errors", i );
	    for( e = 1; e < argc; e++ ) {
		args[curarg++] = strdup(argv[e]);
	    }
	    args[curarg++] = NULL;

	    /*
	     * fork and execute VTserver in the new process;
	     * simulate failure of spawn in iteration 3,
	     * and failure to execute in iteration 4
	     */
	    switch( i ) {
	    case 3:
		fprintf( stderr, "simulating failure to create process for VTserver\n" );
		pid = pid_t_null;
		break;
	    case 4:
		fprintf( stderr, "simulating failure to run VTserver\n" );
		pid = myspawn( "no such program", args );
		break;
	    default:
		fprintf( stderr, "starting VTserver as:" );
		for( e = 0; args[e]; e++ ) {
		    fprintf( stderr, " %s", args[e] );
		}
		fprintf( stderr, "\n" );
		pid = myspawn( "VTserver", args );
		break;
	    }

	    fflush( stderr );

	    /*
	     * parent process, with or without a child process;
	     * iterate until VTserver has terminated,
	     * killing it if necessary
	     */
	    do {
		if( pid == pid_t_null ) {
		    abort_local = 3;
		} else {
		    /* although we have to poll to remain system independent,
		       at least don't do it continously */
		    sleep(1);

		    switch( checkprocess( pid ) ) {
		    case 0:
			/* not done yet */
			abort_local = 0;
			break;
		    case 1:
			/* finished successfully */
			success = 1;
			abort_local = 3;
			break;
		    case 2:
			/* error occurred, but done */
			success = 0;
			abort_local = 3;
			break;
		    default:
			assert( 0 );
			break;
		    }
		}
		MPI_Allreduce( &abort_local, &abort_global, 1, MPI_INT,
			       MPI_MAX, MPI_COMM_WORLD );

		if( abort_global == 2 ) {
		    /* client failed, abort server */
		    mykill( pid );
		    fprintf( stderr, "client failed, killing server\n" );
		}
	    } while( abort_global < 3 );

	    if( success ) {
		/*
		 * Look at STF file.
		 * Again, please refer to simplecs.c.
		 */
	    }

	    for( e = 0; args[e]; e++ ) {
		free( args[e] );
	    }
	    free( args );
	}
    }

    MPI_Comm_free( &clients );
    MPI_Finalize();

    return 0;
}
