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
 * This program tests the timers provided by libVT.a.
 * See ITC user's guide for details.
 */ 

#include "config.h"

#include <VT.h>
#include <mpi.h>
#include "testutil.h"

#ifndef _WIN32
# include <unistd.h>
#else
# include <windows.h>  /* Sleep  */
# define sleep( sec ) Sleep( 1000 * (sec) )
# define popen  _popen
# define pclose _pclose
# define pclose _pclose
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <ctype.h>
#include <sys/stat.h>

#define MSG_CNT      1000         /**< number of message to be sent back and forth */
#define NUM_BINS     11           /**< number of bins for histogram */
#define LATENCY_TEST 10           /**< number of seconds that each message latency measurement is supposed to run */


/** duration of one clock tick */
static long double clkperiod = 0;

/** throw away these many lower bits of a time stamp to get clock ticks */
static int eventbits = 0;

/**
 * utility function: read lines from ASCII trace until either events or the expected declaration is found
 *
 * @param  trace    open trace file
 * @param  pattern  a sscanf pattern with one or two conversion parameter
 * @retval param    result of sscanf conversion
 * @return 1 iff parameter(s) were found
 */
static int parseheader( FILE *trace, const char *pattern, void *param1, void *param2 )
{
    int result = 0;

    while( !result ) {
        int character = fgetc( trace );
        if( character == EOF ) {
            /* end of file */
            break;
        }
        ungetc( character, trace );
        if( isdigit( character ) || character == '+' || character == '-' ) {
            /* start of event data */
            break;
        }
        result = fscanf( trace, pattern, param1, param2 );
        do {
            /* skip to next line */
            character = fgetc( trace );
        } while( character != EOF && character != '\n' );
    }

    return result;
}

/** private API function to get raw clock ticks */
extern INT64 VT_rawtimestamp( void );

/** do ping ping */
static void pingpong( int source, int target, int tag, int num )
{
    MPI_Status status;
    int i, rank;
    char buffer[1];

    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    memset( buffer, 0, sizeof(buffer) );
    for( i = 0; i < num; i++ ) {
        if( rank == target ) {
            MPI_Recv( buffer, 0, MPI_BYTE, source, tag, MPI_COMM_WORLD, &status );
            MPI_Send( buffer, 0, MPI_BYTE, source, tag, MPI_COMM_WORLD );
        } else if( rank == source ) {
            MPI_Send( buffer, 0, MPI_BYTE, target, tag, MPI_COMM_WORLD );
            MPI_Recv( buffer, 0, MPI_BYTE, target, tag, MPI_COMM_WORLD, &status );
        }
    }
}

/** print samples of one iterations, return average delta */
static long double printsamples( FILE *report, const char *title, const char *shorttitle, long double *times, int count )
{
    int i, start, end;
    long double average, deviation, min, max, median;
    unsigned int maxcount, histogram[NUM_BINS], bin;
    
    /*
     * calculate min, max, average and empirical (n-1) standard deviation,
     * ignoring the 10% of the samples at borders
     */
    memset( histogram, 0, sizeof(histogram) );
    sortlongdouble( times, count );
    start = count * 5 / 100;
    end = count * 95 / 100;
    min = times[start];
    max = times[end - 1];
    for( i = start, average = 0, deviation = 0, bin = 0, maxcount = 0;
         i < end;
         i++ ) {
        if( bin + 1 < NUM_BINS &&
            times[i] >= min + (bin + 1) * (max - min) / NUM_BINS ) {
            if( histogram[bin] > maxcount ) {
                maxcount = histogram[bin];
            }
            bin++;
        }
        histogram[bin]++;
        average += times[i];
        deviation += times[i] * times[i];
    }
    if( histogram[bin] > maxcount ) {
        maxcount = histogram[bin];
    }
    average /= end - start;
    deviation = sqrt( ( deviation - average * average ) / ( end - 1 - start ) );
    median = times[(end - start) / 2];

    if( shorttitle ) {
        printf( "   %s: %s\n", shorttitle, prettyprintseconds( average, 0, 3, NULL ) );
    }
    if( report ) {
        fprintf( report, "      %s\n", title );
        fprintf( report, "         min: %s\n", prettyprintseconds( min, 0, 3, NULL ) );
        fprintf( report, "         average: %s\n", prettyprintseconds( average, 0, 3, NULL ) );
        fprintf( report, "         median: %s\n", prettyprintseconds( median, 0, 3, NULL ) );
        fprintf( report, "         max: %s\n", prettyprintseconds( max, 0, 3, NULL ) );
        fprintf( report, "         deviation: %s\n", prettyprintseconds( deviation, 0, 3, NULL ) );
        for( i = 0; i < NUM_BINS; i++ ) {
            fprintf( report, "         >= %s: %s %u\n",
                     prettyprintseconds( min + i * ( max - min ) / NUM_BINS, 8, 3, NULL ),
                     printbar( 40 * histogram[i] / maxcount ),
                     histogram[i] );
        }
    }

    return average;
}

/** print results for one iteration */
static void printresults( FILE *report, FILE *offsets,
                          long double *minaverage, long double *maxaverage,
                          int source, int target, int iteration,
                          long double startts, long double endts,
                          long long pingticks[MSG_CNT], long long pongticks[MSG_CNT], int pingcnt, int pongcnt )
{
    char shorttitle[80];

    if( report ) {
        fprintf( report, "   %d. iteration at time stamps %.0f-%.0f\n",
                 iteration + 1, (double)startts, (double)endts );
    }
    sprintf( shorttitle, "%d. time stamps %.0f-%.0f",
             iteration + 1, (double)startts, (double)endts );

    if( pingcnt != pongcnt ) {
        if( report ) {
            fprintf( report, "      error: send and receive counts do not match (%d != %d)\n", pingcnt, pongcnt );
        }
    } else if( !pingcnt ) {
        if( report ) {
            fprintf( report, "      no data\n" );
        }
    } else {
        int i;
        long double deltats[MSG_CNT];
        long double averagets = 0;

        for( i = 0; i < pingcnt; i++ ) {
            deltats[i] = (pingticks[i] - pongticks[i]) * clkperiod;
        }
        averagets = printsamples( report, "delta", shorttitle, deltats, pingcnt );
        if( !iteration || averagets < *minaverage ) {
            *minaverage = averagets;
        }
        if( !iteration || averagets > *maxaverage ) {
            *maxaverage = averagets;
        }
        if( offsets ) {
            fprintf( offsets, "%.0f %.20e\n",
                     (double)( ( startts / ( 1 << eventbits ) + endts / ( 1 << eventbits ) ) / 2 ),
                     (double)( averagets / clkperiod ) );
        }
    }
}

/** print a time range into the given file */
static void prettyprintrange( FILE *output, const char *prefix, double min, double max )
{
    char buffer[3][80];

    fprintf( output, "%smin %s, max %s -> range %s\n",
             prefix,
             prettyprintseconds( min, 0, 3, buffer[0] ),
             prettyprintseconds( max, 0, 3, buffer[1] ),
             prettyprintseconds( max - min, 0, 3, buffer[2] ) );
}

/** used by qsort for sorting based on the local round-trip time */
static int cmpsamples( const void *a, const void *b )
{
    const long long *samplea = a, *sampleb = b;
    long long durationa = samplea[3] - samplea[0],
        durationb = sampleb[3] - sampleb[0];

    return durationa < durationb ? -1 :
        durationa == durationb ? 0 :
        -1;
}

/** dump the message data after removing the 10% outliers with the highest round-trip time */
static void dumpsamples( FILE *msgs, long long samples[MSG_CNT][4], int cnt )
{
    int i;

    if( msgs ) {
        qsort( samples, cnt, sizeof(long long) * 4, cmpsamples );
        
        for( i = 0; i < cnt * 50 / 100; i++ ) {
            fprintf( msgs,
                     "%.0f %.0f %.0f %.0f\n",
                     (double)samples[i][0],
                     (double)samples[i][1],
                     (double)samples[i][2],
                     (double)samples[i][3] );
        }
    }
}

/**
 * Convert STF file to ASCII VTF via stftool,
 * parse that to find the message between the given source/target process,
 * then calculate and print statistics about each message exchange.
 *
 * Updates maximum deviation from expected outcome and which source/target pair,
 * returns total runtime in seconds.
 */
static double scanresults( FILE *report, FILE *offsets, FILE *msgs, const char *filename, int source, int target,
                           int firstpair, double *maxdeviation, int *avgsource, int *avgtarget )
{
    char cmd[256];
    double duration = 0;
    FILE *trace;

    sprintf( cmd, "stftool \"%s\" --verbose 0 --dump --matched-vtf --request MESSAGES,%d", filename, source );
    trace = popen( cmd, "r" );
    if( trace ) {
        int iteration = -1;
        long long pingticks[MSG_CNT], pongticks[MSG_CNT];
        long long samples[MSG_CNT][4];
        long double startts, endts;
        long double itminaverage, itmaxaverage;
        int pingcnt, pongcnt, cnt;

        printf( "processes %d <-> %d\n", source, target );
        if( offsets ) {
            fprintf( offsets, "\n\n# processes %d <-> %d\n", source, target );
        }
        if( msgs ) {
            fprintf( msgs, "\n\n# processes %d <-> %d\n", source, target );
        }
        if( report ) {
            fprintf( report, "processes %d <-> %d\n", source, target );
        }
        parseheader( trace, "CLKPERIOD %Le", &clkperiod, NULL );
        parseheader( trace, "DURATION %Le %Le", &startts, &endts );
        duration = ( endts - startts ) * clkperiod;

        if( !parseheader( trace, "EVENTBITS %d", &eventbits, NULL ) ) {
            eventbits = 0;
        } else {
            clkperiod *= 1 << eventbits;
        }
        pingcnt = 0;
        pongcnt = 0;
        cnt = 0;
        while( !feof( trace ) ) {
            long double ts, duration;
            long long ticks;
            int sender, receiver, tag, comm, len;
            int scanned = 0;

            do {
                char buffer[256];
                fgets( buffer, sizeof(buffer), trace );
                scanned = sscanf( buffer,
                                  "%Lf ONETOONE CPU %d TO %d DELTA %Lf COM %d LEN %d TAG %d",
                                  &ts, &sender, &receiver, &duration, &comm, &len, &tag );
            } while( scanned != 7 && !feof( trace ) );

            if( scanned == 7 ) {
                if( tag != iteration ) {
                    if( iteration != -1 ) {
                        printresults( report, offsets, &itminaverage, &itmaxaverage,
                                      source, target, iteration,
                                      startts, endts, pingticks, pongticks, pingcnt, pongcnt );
                        dumpsamples( msgs, samples, cnt );
                    }
                    pingcnt = 0;
                    pongcnt = 0;
                    cnt = 0;
                    iteration = tag;
                    startts = 0;
                }

                ticks = ( ( (long long)ts + (long long)duration ) >> eventbits ) -
                    ( (long long)ts >> eventbits );
                sender--;
                receiver--;
                if( sender == source ) {
                    if( receiver == target && pingcnt < MSG_CNT ) {
                        pingticks[pingcnt++] = ticks;
                        endts = ts + (long long)duration;
                        samples[cnt][0] = (long long)ts >> eventbits;
                        samples[cnt][1] = (long long)endts >> eventbits;
                        if( !startts ) {
                            startts = ts;
                        }
                    }
                } else if( sender == target ) {
                    if( receiver == source && pongcnt < MSG_CNT ) {
                        pongticks[pongcnt++] = ticks;
                        endts = ts + duration;
                        samples[cnt][2] = (long long)ts >> eventbits;
                        samples[cnt][3] = (long long)endts >> eventbits;
                        cnt++;
                        if( !startts ) {
                            startts = ts;
                        }
                    }
                }
            }
        }
        printresults( report, offsets, &itminaverage, &itmaxaverage,
                      source, target, iteration, 
                      startts, endts, pingticks, pongticks, pingcnt, pongcnt );
        dumpsamples( msgs, samples, cnt );
        prettyprintrange( stdout, "   ", itminaverage, itmaxaverage );
        if( report ) {
            prettyprintrange( report, "   ", itminaverage, itmaxaverage );
            fprintf( report, "\n" );
        }

        if( firstpair ||
            fabs( itmaxaverage ) > fabs( *maxdeviation ) ||
            fabs( itminaverage ) >  fabs( *maxdeviation ) ) {
            *maxdeviation = fabs( itmaxaverage ) > fabs( itminaverage ) ? itmaxaverage : itminaverage;
            *avgsource = source;
            *avgtarget = target;
        }

        pclose( trace );
    } else {
        perror( cmd );
    }

    return duration;
}

/**
 * Do MPI ping-pong experiment and write timertest.stf.
 * The only command line argument is the total runtime in seconds.
 *
 * @retval mpisize    set to the number of MPI processes
 * @retval mpirank    set to the local MPI rank
 * @retval latencies  allocates an array and stores the message latencies between
 *                    each process and rank zero in it (first entry for rank zero is empty)
 */
static void dopingpong( int argc, char **argv, int *mpisize, int *mpirank, double **latencies )
{
    double duration = 5;         /**< duration of timer performance test in seconds */
    double msgduration = 30;     /**< total duration of message exchange in seconds */
    double delay = 10;           /**< pause between message exchanges in seconds */
    int rank = -1, size = -1;
    int i, e, iteration;

    /* override trace file writing */
    setenv( "VT_LOGFILE_NAME", "timertest.stf", 0 );
    setenv( "VT_VERBOSE", "0", 0 );

    /* ask for generation of clocksample.* files */
    setenv( "VT_TIMER_DUMP", "on", 1 );

    /* ignore virtually all intermediate VT_timesync() results */
    setenv( "VT_TIMER_SKIP", "100000", 0 );

    if( VT_initialize( &argc, &argv ) == VT_OK ) {
        double startts, ts;
        char prefix[40];
        char hostname[20], *dot;

        /* setup, simplify hostname */
        MPI_Comm_rank( MPI_COMM_WORLD, &rank );
        MPI_Comm_size( MPI_COMM_WORLD, &size );
        gethostname( hostname, sizeof(hostname) );
        hostname[sizeof(hostname) - 1] = 0;
        dot = strchr( hostname, '.' );
        if( dot ) {
            *dot = 0;
        }
        sprintf( prefix, "[%d (%s)] ", rank, hostname );

        /*
         * measure accuracy of individual clocks:
         * the code is unaware of MPI and thus
         * each process prints directly to stdout
         */
        timerperformance( prefix, duration );

        /* synchronize stdout flushing - not perfect... */
        for( i = 0; i < size; i++ ) {
            MPI_Barrier( MPI_COMM_WORLD );
            if( rank == i ) {
                fflush( stdout );
            }
        }

        /* override default runtime? */
        if( argc > 1 ) {
            msgduration = atof( argv[1] );
        }
        if( argc > 2 ) {
            delay = atof( argv[2] );
        }

        /* record messages for later checking of synchronization */
        startts = VT_timeofday();
        iteration = 0;
        do {
            if( iteration ) {
                /*
                 * Synchronize clocks between message exchange
                 * and sleeping. ITC will sample automatically before
                 * the first iteration and after the last one.
                 */
                VT_timesync();
                sleep( delay );
            }

            for( i = 0; i < size; i++ ) {
                for( e = i + 1; e < size; e++ ) {
                    /* barrier */
                    VT_traceoff();
                    MPI_Barrier( MPI_COMM_WORLD );
                    pingpong( i, e, 0, 5 );
                    VT_traceon();

                    /* record real messages */
                    if( rank == 0 ) {
                        printf( "%d. recording messages %d <-> %d...\n", iteration, i, e );
                        fflush( stdout );
                    }
                    pingpong( i, e, iteration, MSG_CNT );
               }
            }
            iteration++;
        } while( VT_timeofday() - startts + delay < msgduration );

        /* measure message latency without tracing */
        VT_traceoff();
        *latencies = malloc( size * sizeof( **latencies ) );
        for( i = 0; i < size; i++ ) {
            for( e = i + 1; e < size; e++ ) {
                int num;

                if( rank == 0 ) {
                    printf( "message latency %d <-> %d: ", i, e );
                    fflush( stdout );
                }
                /*
                 * number of messages is tuned for latencies of around 10us and
                 * a total runtime for each test of 0.1s
                 */
                num = 0.1 / 10e-6;

                /* barrier */
                MPI_Barrier( MPI_COMM_WORLD );
                pingpong( i, e, 0, 5 );

                /* real measurement */
                startts = VT_timeofday();
                pingpong( i, e, 0, num );
                ts = VT_timeofday() - startts;
                MPI_Bcast( &ts, 1, MPI_DOUBLE, i, MPI_COMM_WORLD );
                (*latencies)[e] = ts / num / 2;

                if( rank == 0 ) {
                    char buffer[2][80];
                    printf( "%s (%s/%d)\n",
                            prettyprintseconds( ts / num / 2, 0, 3, buffer[0] ),
                            prettyprintseconds( ts, 0, 3, buffer[1] ),
                            2 * num );
                    fflush( stdout );
                }
            }
        }
    }

    printf( "\n" );
    VT_finalize();
    printf( "\n" );

    *mpisize = size;
    *mpirank = rank;
}
   


int main( int argc, char **argv )
{
    int rank = 0, size = 0;
    const char *filename;
    struct stat buf;
    double *latencies = NULL;
    char buffer[256], buffers[2][256];

    /*
     * Invoke with existing STF file as parameter to skip the ping-pong test.
     * If file cannot be parsed, silently proceed with running the test because
     * we might have misinterpreted an MPI command line argument.
     */
    if( argc > 1 && !stat( argv[1], &buf ) ) {
        FILE *trace;

        filename = argv[1];
        sprintf( buffer, "stftool \"%s\" --verbose 0 --dump", filename );
        trace = popen( buffer, "r" );
        if( trace ) {
            parseheader( trace, "NCPUS %d", &size, NULL );
            pclose( trace );
        }
    }

    /* run experiment now if we don't have a valid file yet */
    if( !size ) {
        dopingpong( argc, argv, &size, &rank, &latencies );
        filename = getenv( "VT_LOGFILE_NAME" );
    }

    /* process #0 does post-processing */
    if( rank == 0 && size >= 2 ) {
        int i, e;
        FILE *report, *offsets, *msgs;
        double maxshift;
        int source, target;
        double duration = 0;

        /* setup output files */
        report = fopen( "timertest.report", "w" );
        if( !report ) {
            perror( "timertest.report" );
        }
        offsets = fopen( "timertest.offsets.dat", "w" );
        if( !offsets ) {
            perror( "timertest.offsets.dat" );
        } else {
            fprintf( offsets, "# <average clock tick of iteration> <offset between processes in clock ticks>" );
        }
        msgs = fopen( "timertest.msgs.dat", "w" );
        if( !msgs ) {
            perror( "timertest.msgs.dat" );
        } else {
            fprintf( msgs, "# <master send> <slave recv> <slave send> <master recv> [clock ticks]" );
        }

        /* scan .stf file for messages */
        for( i = 0; i < size; i++ ) {
            for( e = i + 1; e < size; e++ ) {
                duration = scanresults( report, i == 0 ? offsets : NULL, i == 0 ? msgs : 0, filename, i, e,
                                        i == 0 && e == 1,
                                        &maxshift, &source, &target );
            }
        }

        sprintf( buffer, "\nmaximum clock offset during run:\n   %d <-> %d %s (latency %s)\n\n",
                 source, target,
                 prettyprintseconds( maxshift, 0, 3, buffers[1] ),
                 latencies ? prettyprintseconds( latencies[target], 0, 3, buffers[0] ) : "unknown" );
        printf( "%s", buffer );
        if( report ) {
            fprintf( report, "%s", buffer );
        }

        /* prepare gnuplot files which visualize the results */
        if( offsets && msgs ) {
            FILE *plotall;

            plotall = fopen( "timertest.gnuplot", "w" );
            if( !plotall ) {
                perror( "timertest.gnuplot" );
            }
            
            if( plotall ) {
                const char *func[] = { "constrained" };
                int type;

                printf( "to produce graph showing trace timing, run: gnuplot timertest.gnuplot\n" );
                fprintf( plotall,
                         "set title 'Application Run'\n"
                         "set output 'timertest.app.eps'\n"
                         "set key below\n"
                         "set xzeroaxis\n"
                         "set xlabel 'runtime [seconds]'\n"
                         "set ylabel 'deviation from ideal linear clock [seconds]'\n"
                         "clkperiod = %e\n"
                         "# spread out the x coordinates of clock samples to avoid\n"
                         "# having data of different processes on top of each other\n"
                         "spread = %e\n",
                         (double)clkperiod,
                         duration / 600 );
                for( i = 1; i < size; i++ ) {
                    fprintf( plotall, "%s 'timertest.offsets.dat' index %d ", i == 1 ? "plot" : ", ", i - 1 );
                    fprintf( plotall, "using ($1 * clkperiod):($2 * clkperiod) " );
                    fprintf( plotall, "title 'ping-pong offsets #%d (latency %s)' with lines",
                             i, latencies ? prettyprintseconds( latencies[i], 0, 3, NULL ) : "unknown" );
                }
                fprintf( plotall, "\npause -1 'Press return'\n" );

                fprintf( plotall,
                         "set title 'Clock Transformation'\n"
                         "set output 'timertest.clock.eps'\n"
                         "set xlabel 'local runtime [seconds]'\n"
                         "set ylabel 'master runtime - local runtime transformed [seconds]'\n" );
                for( i = 1; i < size; i++ ) {
                    fprintf( plotall,
                             "load 'clocksamples.%d.spline.dat'\n",
                             i );
                }
                for( type = 0; type < sizeof( func ) / sizeof( func[0] ); type++ ) {
                    for( i = 1; i < size; i++ ) {
                        fprintf( plotall, "%s 'clocksamples.%d.spline.dat' ", i == 1 ? "plot" : ", ", i );
                        fprintf( plotall,
                                 "using ( $1 * clkperiod%d ):( ( $2 - $1 ) * clkperiod%d + "
                                 "( $1 - linear%d( $1 ) ) * clkperiod%d ) 'x=%%lf; y=%%lf' ",
                                 i, i, i, i );
                        fprintf( plotall, "title 'clock samples #%d', ", i );

                        fprintf( plotall, "%sspline%d( x / clkperiod%d ) * clkperiod%d - linear%d( x / clkperiod%d ) * clkperiod%d ",
                                 func[type], i, i, i, i, i, i );
                        fprintf( plotall, "title '%s spline correction #%d'", func[type], i );
                    }
                    fprintf( plotall, "\npause -1 'Press return'\n" );
                }

                fprintf( plotall,
                         "set title 'Interpolation Error'\n"
                         "set output 'timertest.error.eps'\n" );
                for( i = 1; i < size; i++ ) {
                    fprintf( plotall, "%s 'clocksamples.%d.spline.dat' ", i == 1 ? "plot" : ", ", i );
                    fprintf( plotall,
                             "using ( $1 * clkperiod%d + %d * spread ):( abs( $2 - linearspline%d( $1 ) ) * clkperiod%d ) 'x=%%lf; y=%%lf' ",
                             i, i - 1, i, i );
                    fprintf( plotall, "title 'error for linear interpolation #%d' with impulses, ", i );

                    fprintf( plotall, "'clocksamples.%d.spline.dat' ", i );
                    fprintf( plotall,
                             "using ( $1 * clkperiod%d + %d * spread ):( -abs( $2 - constrainedspline%d( $1 ) ) * clkperiod%d ) 'x=%%lf; y=%%lf' ",
                             i, i - 1, i, i );
                    fprintf( plotall, "title 'error for constrained spline interpolation #%d' with impulses", i );
                }
                fprintf( plotall, "\npause -1 'Press return'\n" );
                fprintf( plotall, "set output '/dev/null'\n" );

                for( i = 1; i < size; i++ ) {
                    INT32 e;

                    for( e = 0; e < 3; e++ ) {
                        INT32 every;
                        for( every = 0; every < 2; every++ ) {
                            fprintf( plotall,
                                     "\nset title '%s raw clock samples of process #%d, iteration #%d'\n"
                                     "set output 'timertest.raw.%d.%d.eps'\n"
                                     "set xlabel 'master runtime [seconds]'\n"
                                     "set ylabel 'local runtime - master runtime [seconds]'\n",
                                     every ? "all" : "best 50",
                                     i, e,
                                     i, e );
                            fprintf( plotall,
                                     "plot 'clocksamples.%d.raw' index %d%s"
                                     "using ($3 * clkperiod%d):(( ($1 + $2) / 2 - $3 ) * clkperiod%d):(($1 - $3) * clkperiod%d):(($2 - $3) * clkperiod%d) notitle with errorbars\n",
                                     i, e, every ? "" : " every 1:1:0:0:50 ",
                                     i, i, i, i );
                            fprintf( plotall, "\npause -1 'Press return'\n" );
                            fprintf( plotall, "set output '/dev/null'\n" );
                        }
                    }
                }

                fclose( plotall );
            }

            if( msgs ) {
                fclose( msgs );
            }
            if( offsets ) {
                fclose( offsets );
            }
        }
    }

    return 0;
}
