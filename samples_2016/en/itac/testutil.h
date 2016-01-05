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

#ifndef _ITC_UTIL_H_
#define _ITC_UTIL_H_

#ifdef V_T

#include <VT.h>
#include <stdlib.h>

#define CHECK_RES( function, name, res ) { int _res = function; if( (res) != _res ) { fprintf( stderr, "[line %d] " #function " returned %d, not %s\n", __LINE__, _res, #res ); exit(10); } }
#define CHECK_FAIL( function, name ) { int e = function; if(e != VT_ERR_NOTINITIALIZED) { fprintf( stderr, "[line %d] " name " returned %d and not VT_ERR_NOTINITIALIZED\n", __LINE__, e); exit(10); } }
#define LOCDEF()  { int loc; VT_scldef( __FILE__, __LINE__, &loc ); VT_thisloc( loc ); }

#define VT_Clock( ) VT_timestamp()

#else

#define LOCDEF()
#define VT_symdef( a, b, c )
#define VT_begin( a )
#define VT_end( a )
#define VT_log_comment( a )
#define VT_flush( )
#define VT_Clock( ) VT_Wtime()
#define VT_traceoff()
#define VT_traceon()
#define VT_registerthread(x) 0
#define VT_getthrank( x )
#define VT_funcdef( a, b, c )
#define VT_initialize( a, b )
#define VT_finalize()
#define VT_enter( a, b )
#define VT_leave( a )

#define CHECK_RES( function, name, res ) function
#define CHECK_FAIL( function, name )     function

#endif

#define CHECK( function, name ) CHECK_RES( function, name, VT_OK )

/* timing functions from VT_usleep.c */
void VT_usleep( int usecs );
#define VT_sleep( seconds ) VT_usleep( 1000000 * (seconds) )

/* from timerperformance.c */
void timerperformance( const char *prefix, double duration );
const char *printbar( int width );
const char *prettyprintseconds( double seconds, int width, int precision, char *buffer );
void sortlongdouble( long double *array, int count );

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

/* 
 *  Adjustments for Windows: 
 */
#ifdef _WIN32

/* some system functions: */
# include <io.h>     /* open  */
# include <direct.h> /* mkdir */
# define mkdir( path, mode ) mkdir( path )

/* emulation of setenv: */
#include <stdlib.h>
#include <stdio.h>
static int setenv( const char *name, const char *value, int overwrite ) {
    char buffer[1024];
    if ( ! overwrite && getenv( name ) ) { return 0; }
    _snprintf( buffer, sizeof( buffer ), "%s=%s", name, value );
    return _putenv( buffer );
}

#endif

/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#endif /* _ITC_UTIL_H_ */
