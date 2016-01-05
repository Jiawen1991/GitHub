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

#define _BSD_COMPAT
#ifndef _WIN32
# include <sys/time.h>
# include <time.h>
#else
# include <winsock2.h> /* select */
# undef small          /* defined in winsock2.h */
# include <windows.h>  /* Sleep  */
# ifndef sleep
#  define sleep( sec ) Sleep( 1000 * (sec) )
# endif
#endif
#include <stdlib.h>

#include "testutil.h"
#include "VT.h"

#ifndef _WIN32
/* Call clock_gettime() instead of VT_timeofday() for vnthreads.pin.tf and vnthreadssimple.pinprof.tf          */
/* because of the PIN 2.7 problem with functions using double args or retuning double values (CQ DPD200188304) */
#ifdef USE_CLOCK_GETTIME
double GetUsec()
{
  struct timespec tp;
  double res = -1;
  if( clock_gettime(CLOCK_REALTIME, &tp) == 0 )
    res = tp.tv_sec + tp.tv_nsec/1000000000;
  
  return res;
}
#endif
#endif

/** sleep for the given amount of micro seconds */
void VT_usleep( int usecs )
{
#ifdef _WIN32
    if ( usecs < 1000 ) { usecs = 1000; }
    Sleep( usecs / 1000 );
#else
    while ( usecs > 0 ) {
#ifndef USE_CLOCK_GETTIME
	double start = VT_timeofday();
#else
	double start = GetUsec();
#endif
        struct timeval timeout;

	timeout.tv_usec = usecs % 1000000;
	timeout.tv_sec  = usecs / 1000000;

	select( 0, NULL, NULL, NULL,
		&timeout );

#ifndef USE_CLOCK_GETTIME
	usecs -= (VT_timeofday( ) - start) * 1000000;
#else
	usecs -= (GetUsec( ) - start) * 1000000;
#endif
    }
#endif
}
