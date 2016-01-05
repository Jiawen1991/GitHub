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
 * This source file provides a function to test the performance
 * of the VT_timestamp() function. It is used by both the timertestcs
 * and vttimertest program.
 */
 
#include "config.h"
#include <VT.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* private API for access to raw clock ticks */
extern UINT64 VT_rawtimestamp( void );
extern FLOAT64 VT_rawclkperiod( void );

#define MIN_INCREASE       1    /**< lower bound for histogram in clock ticks */
#define MAX_INCREASE    5001    /**< upper bound for histogram in clock ticks */
#define BIN_SIZE         500    /**< size of one bin in clock ticks */
#define MAX_SAMPLES     1000    /**< maximum number of samples to collect for median/average */

const char *prettyprintseconds( double seconds, int width, int precision, char *buffer )
{
    static char localbuffer[80];
    double absseconds = fabs( seconds );

    if( !buffer ) {
        buffer = localbuffer;
    }

    if( absseconds < 1e-6 ) {
        sprintf( buffer, "%*.*fns", width, precision, seconds * 1e9 );
    } else if( absseconds < 1e-3 ) {
        sprintf( buffer, "%*.*fus", width, precision, seconds * 1e6 );
    } else if( absseconds < 1 ) {
        sprintf( buffer, "%*.*fms", width, precision, seconds * 1e3 );
    } else {
        sprintf( buffer, "%*.*fs", width, precision, seconds );
    }

    return buffer;
}

const char *printbar( int width )
{
    static char buffer[80];

    if( width > sizeof(buffer) - 1 ) {
        width = sizeof(buffer) - 1;
    }
    memset( buffer, '#', width );
    buffer[width] = 0;
    return buffer;
}

static int comparedouble( const void *a, const void *b )
{
    long double delta = *(const long double *)a - *(const long double *)b;

    return delta < 0 ? -1 :
        delta > 0 ? 1 :
        0;
}

void sortlongdouble( long double *array, int count )
{
    qsort( array, count, sizeof( *array ), comparedouble );
}

static int compareint64( const void *a, const void *b )
{
    INT64 delta = *(const INT64 *)a - *(const INT64 *)b;

    return delta < 0 ? -1 :
        delta > 0 ? 1 :
        0;
}

static void sortint64( INT64 *array, int count )
{
    qsort( array, count, sizeof( *array ), compareint64 );
}

/** a histogram of clock samples */
static unsigned long histogram[( MAX_INCREASE - MIN_INCREASE ) / BIN_SIZE + 3];

/** size of histogram */
#define NUMBINS ( sizeof( histogram ) / sizeof( histogram[0] ) )

/** the initial clock samples encounted */
static double samples[MAX_SAMPLES];

/** number of entries in samples array */
static unsigned long count;

/**
 * call timer source repeatedly and record delta between samples
 * in histogram and samples array
 *
 * @param rawtimestamp      the function which generates clock ticks
 * @param maxclockticks     maximum number of clock ticks for whole run
 * @param overhead          number of clock ticks to substract from each delta
 */
static void genhistogram( UINT64 (*rawtimestamp)( void ), UINT64 maxclockticks, UINT64 overhead )
{
    INT64 increase;
    UINT64 startticks, lastticks, nextticks;

    startticks = rawtimestamp();
    lastticks = 0;
    count = 0;
    do {
        nextticks = rawtimestamp() - startticks;
        increase = nextticks - lastticks;
        if( increase < 0 ) {
            histogram[0]++;
        } else if( increase > 0 ) {
            unsigned int index;

            increase -= overhead;
            if( count < MAX_SAMPLES ) {
                samples[count] = increase;
                count++;
            }

            if( increase < MIN_INCREASE ) {
                index = 1;
            } else {
                index = (unsigned int)( ( increase - MIN_INCREASE ) / BIN_SIZE ) + 2;
                if( index >= NUMBINS ) {
                    index = NUMBINS - 1;
                }
            }
            histogram[index]++;
        }
        lastticks = nextticks;
    } while( nextticks < maxclockticks );
}

static UINT64 dummytimestamp( void )
{
    static UINT64 counter;

    return ++counter;
}

/**
 * runs a timer performance test for the given duration
 *
 * @param duration    duration of test in seconds
 */
void timerperformance( const char *prefix, double duration )
{
    /* bins for <0, <MIN_INCREASE, >=MAX_INCREASE and for all intervals inbetween */
    UINT64 maxclockticks = duration / VT_rawclkperiod();
    UINT64 calibrateticks = 10000000;
    unsigned int i;
    FLOAT64 startts, endts;
    FLOAT64 measuredclkperiod;
    UINT64 startticks, nextticks;
    INT64 totalincrease;
    unsigned long max;
    char buffer[2][256];

    /* measure overhead */
    startts = VT_timeofday();
    startticks = VT_rawtimestamp();
    count = 0;
    do {
        count++;
        nextticks = VT_rawtimestamp();
    } while( ( nextticks - startticks ) < maxclockticks );
    endts = VT_timeofday();
    printf( "%sperformance: %lu calls in %s wall clock time = %s/call = %.0f calls/s\n",
            prefix,
            count, prettyprintseconds( ( endts - startts ), 0, 3, buffer[0] ),
            prettyprintseconds( ( endts - startts ) / count, 0, 3, buffer[1] ),
            count / ( endts - startts ) );

    /* calculate clock period based on wall clock time */
    measuredclkperiod = ( endts - startts ) / ( nextticks - startticks );
    printf( "%smeasured clock period/frequency vs. nominal: %s/%.3fMHz vs. %s/%.3fMHz\n",
            prefix,
            prettyprintseconds( measuredclkperiod, 0, 3, buffer[0] ),
            1 / measuredclkperiod / 1000000,
            prettyprintseconds( VT_rawclkperiod(), 0, 3, buffer[1] ),
            1 / VT_rawclkperiod() / 1000000 );

    /* calibrate loop */
    memset( histogram, 0, sizeof( histogram ) );
    memset( samples, 0, sizeof( samples ) );
    startticks = VT_rawtimestamp();
    genhistogram( dummytimestamp, calibrateticks, 0 );
    nextticks = VT_rawtimestamp();
    printf( "%soverhead for sampling loop: %.0f clock ticks (= %s) for %.0f iterations = %.0f ticks/iteration\n",
            prefix,
            (double)( nextticks - startticks ),
            prettyprintseconds( ( nextticks - startticks ) * measuredclkperiod, 0, 3, NULL ),
            (double)calibrateticks,
            (double)( ( nextticks - startticks ) / calibrateticks ) );

    /* do real measurements */
    memset( histogram, 0, sizeof( histogram ) );
    memset( samples, 0, sizeof( samples ) );
    genhistogram( VT_rawtimestamp, maxclockticks, ( nextticks - startticks ) / calibrateticks );

    /* print average and medium increase */
    for( i = 0, totalincrease = 0; i < count; i++ ) {
        totalincrease += samples[i];
    }
    printf( "%saverage increase: %.0f clock ticks = %s = %.3fMHz\n",
            prefix,
            (double)( totalincrease / count ),
            prettyprintseconds( measuredclkperiod * totalincrease / count, 0, 3, NULL ),
            1 / ( measuredclkperiod * totalincrease / count ) / 1e6 );
    sortint64( (INT64 *)samples, count );
    printf( "%s median increase: %.0f clock ticks = %s = %.3fMHz\n",
            prefix,
            (double)samples[count/2],
            prettyprintseconds( measuredclkperiod * samples[count/2], 0, 3, NULL ),
            1 / ( measuredclkperiod * samples[count/2] ) / 1e6 );
    for( i = 0, max = 0; i < NUMBINS; i++ ) {
        if( histogram[i] > max ) {
            max = histogram[i];
        }
    }
    printf( "%s <     0 ticks =   0.00s : %s %lu\n", prefix, printbar( histogram[0] * 20 / max ), histogram[0] );
    printf( "%s < %5d ticks = %s: %s %lu\n",
            prefix,
            MIN_INCREASE,
            prettyprintseconds( measuredclkperiod * MIN_INCREASE, 6, 2, NULL ),
            printbar( histogram[1] * 20 / max ),
            histogram[1] );
    for( i = 2; i < NUMBINS; i++ ) {
        printf( "%s>= %5d ticks = %s: %s %lu\n",
                prefix,
                ( i - 2 ) * BIN_SIZE + MIN_INCREASE,
                prettyprintseconds( measuredclkperiod * ( ( i - 2 ) * BIN_SIZE + MIN_INCREASE ), 6, 2, NULL ),
                printbar( histogram[i] * 20 / max ),
                histogram[i] );
    }
    printf( "\n" );
}
