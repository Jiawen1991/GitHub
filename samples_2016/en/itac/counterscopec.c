/*******************************************************************
 * 
 * Copyright 2003-2007 Intel Corporation.
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

#include <VT.h>
#include "config.h"
#include "testutil.h"

#include "VT_usleep.c"

#include <math.h>
#include <stdlib.h>

int main( int argc, char **argv )
{
    /* the program executes a loop a fixed amount of times */
    int loop;

    /*
     * To demonstrate and visualize the different semantic
     * behind counter scopes, four integer counters will
     * be defined, one for each scope, and the same value
     * will be logged for each one.
     *
     * The counter class name is "scope", the counter names
     * are "before", "after", "point", "sample".
     */
    int scopecounters[4], scopeclass;

    /*
     * Here are some real-world examples for different scopes,
     * logged with counter class name "examples". The data for
     * many of these counters is faked, though.
     */
    int exampleclass;
    int timecounter;    /* samples current time */
    int memcounter;     /* whenever memory is allocated, the
			   counter is updated to the new value
			   (valid after the time it is logged) */
    int deltacounter;   /* at the end of each loop iteration
			   a value is calculated and then logged,
			   in this case simply the duration of
			   the iteration (valid before) */
    int msgcounter;     /* the program handles incoming messages,
			   and for each message receival the size
			   of the message is logged */
    int samplesine;     /* the values for the message lengths follow
			   a sine curve with period 1s and amplitude 1024 -
			   okay, the negative values don't make much sense,
			   but the look nice, especially when declaring the
			   values as samples of a curve :-) */
    int epsilon;        /* a decreasing value that describes the accuracy
			   of the current solution for the fictive mathematical
                           problem the program works on */
    int solution;       /* the solution to the problem, random negative values */

    /* VT_COUNT_INTEGER uses high/low UINT32 pairs for each counter value */
    static const UINT32 ibounds[4] = { 0,0,         /* lower bound */
				       0,0xFFFFFFFF /* upper bound */ };
    /* VT_COUNT_FLOAT just uses a pair of 64 bit floats */
    static const FLOAT64 fbounds[2] = { 0,   /* lower bound */
					1e20 /* upper bound */ };
    /* VT_COUNT_INTEGER64 uses a pair of 64 bit integers */
    static const INT64 i64bounds[2] = { 0,              /* lower bound */
					((INT64)1)<<63  /* upper bound */ };

    VT_initialize( &argc, &argv );
    VT_classdef( "scope", &scopeclass );
    VT_countdef( "before", scopeclass,
		 VT_COUNT_INTEGER|VT_COUNT_ABSVAL|VT_COUNT_VALID_BEFORE,
		 VT_ME, /* each process gets its own instance of the counter */
		 ibounds, "#", scopecounters + 0 );
    VT_countdef( "after", scopeclass,
		 VT_COUNT_INTEGER|VT_COUNT_ABSVAL|VT_COUNT_VALID_AFTER,
		 VT_ME,
		 ibounds, "#", scopecounters + 1 );
    VT_countdef( "point", scopeclass,
		 VT_COUNT_INTEGER|VT_COUNT_ABSVAL|VT_COUNT_VALID_POINT,
		 VT_ME,
		 ibounds, "#", scopecounters + 2 );
    VT_countdef( "sample", scopeclass,
		 VT_COUNT_INTEGER|VT_COUNT_ABSVAL|VT_COUNT_VALID_SAMPLE,
		 VT_ME,
		 ibounds, "#", scopecounters + 3 );

    VT_classdef( "examples", &exampleclass );
    VT_countdef( "time", exampleclass,
		 VT_COUNT_FLOAT|VT_COUNT_ABSVAL|VT_COUNT_VALID_SAMPLE,
		 VT_ME, fbounds, "s", &timecounter );
    VT_countdef( "mem", exampleclass,
		 VT_COUNT_INTEGER64|VT_COUNT_ABSVAL|VT_COUNT_VALID_AFTER,
		 VT_ME, i64bounds, "bytes", &memcounter );
    VT_countdef( "delta", exampleclass,
		 VT_COUNT_FLOAT|VT_COUNT_ABSVAL|VT_COUNT_VALID_BEFORE,
		 VT_ME, fbounds, "s", &deltacounter );
    VT_countdef( "msg", exampleclass,
		 VT_COUNT_FLOAT|VT_COUNT_ABSVAL|VT_COUNT_VALID_POINT,
		 VT_ME, fbounds, "bytes", &msgcounter );
    VT_countdef( "sine", exampleclass,
		 VT_COUNT_FLOAT|VT_COUNT_ABSVAL|VT_COUNT_VALID_SAMPLE,
		 VT_ME, fbounds, "bytes", &samplesine );
    VT_countdef( "epsilon", exampleclass,
		 VT_COUNT_FLOAT|VT_COUNT_ABSVAL|VT_COUNT_VALID_AFTER,
		 VT_ME, fbounds, "", &epsilon );
    VT_countdef( "solution", exampleclass,
		 VT_COUNT_FLOAT|VT_COUNT_ABSVAL|VT_COUNT_VALID_AFTER,
		 VT_ME, fbounds, "", &solution );

    /* main loop of the program, with a fixed delay of 0.01 seconds */
    for( loop = 0; loop < 16; loop++ ) {
	UINT32 ivalues[4*2];
	FLOAT64 fvalues[2];
	int counters[2];
	FLOAT64 fvalue;
	INT64 i64value;
	double starttime, endtime;
	int i;
	
	starttime = VT_timestamp();

	/* the scope counters simply log the loop iteration,
	   which has to be repeated four times, once for each counter */
	for( i = 0; i < 4; i++ ) {
	    ivalues[2 * i + 0] = 0;            /* high UINT32 */
	    ivalues[2 * i + 1] = (UINT32)loop; /* low UINT32 */
	}
	VT_countval( 4, scopecounters, ivalues );

	/* log the current time */
	fvalue = VT_timestamp();
	VT_countval( 1, &timecounter, &fvalue );

	/* fake a _huge_ memory leak of 2^59 bytes per iteration,
	   leading to a total memory consumption of
           16 * 2^59 = 2^4 * 2^59 = 2^63 >:-> */
	i64value = loop * ((INT64)1)<<59;
	VT_countval( 1, &memcounter, &i64value );

	/* fake values for the fictive mathematical problem */
	fvalues[0] = 1e6 / (loop+1);
	counters[0] = epsilon;
	fvalues[1] = -rand();
	counters[1] = solution;
	VT_countval( 2, counters, fvalues );

	for( i = 0; i < 16; i++ ) {
	    VT_usleep( 1e6 * 0.001 );

            /* fake 16 message receives per iteration, with message
               sizes that follow a sinus curve with amplitude 1024
               and period 0.1s */
	    fvalues[0] = 1024 * sin( 2 * 3.14159 * (VT_timestamp() - VT_timestart()) / 0.1 );
	    counters[0] = msgcounter;
	    fvalues[1] = fvalues[0];
	    counters[1] = samplesine;
	    VT_countval( 2, counters, fvalues );
	}

	endtime = VT_timestamp();

	/* log duration of last iteration */
	fvalue = endtime - starttime;
	VT_countval( 1, &deltacounter, &fvalue );
    }

    VT_finalize();

    return 0;
}
