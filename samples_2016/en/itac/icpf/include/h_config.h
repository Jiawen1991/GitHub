/*********************************************************************
 * Copyright (C) 2003-2014 Intel Corporation.  All Rights Reserved.
 *
 * The source code contained or described herein and all documents
 * related to the source code ("Material") are owned by Intel Corporation
 * or its suppliers or licensors.  Title to the Material remains with
 * Intel Corporation or its suppliers and licensors.  The Material is
 * protected by worldwide copyright and trade secret laws and treaty
 * provisions.  No part of the Material may be used, copied, reproduced,
 * modified, published, uploaded, posted, transmitted, distributed, or
 * disclosed in any way without Intel's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other
 * intellectual property right is granted to or conferred upon you by
 * disclosure or delivery of the Materials, either expressly, by
 * implication, inducement, estoppel or otherwise.  Any license under
 * such intellectual property rights must be express and approved by
 * Intel in writing.
 * 
 *********************************************************************/

#ifndef _H_CONFIG_H_
# define _H_CONFIG_H_

#ifndef _WIN32
    #ifndef __i386
        #ifndef __x86_64
            //IA64-LIN
            #define HAVE_INT64 long
            #define HAVE_POINTER_INT long
        #else
            //EM64T-LIN
            #define HAVE_INT64 long
            #define HAVE_POINTER_INT long
        #endif
    #else
        //IA32-LIN
        #define HAVE_INT64 long long int
        #define HAVE_POINTER_INT int
    #endif
#else
    #ifndef _WIN64
        //IA32-WIN
        #define HAVE_INT64 long long int
        #define HAVE_POINTER_INT int
    #else
        //EM64T-WIN
        #define HAVE_INT64 long long int
        #define HAVE_POINTER_INT long long int
    #endif
#endif

#define HAVE_INT32 int
#define HAVE_FLOAT32 float
#define HAVE_FLOAT64 double

/** signed 32 bit integer */
typedef int I4;

/** unsigned 32 bit integer */
typedef unsigned int U4;

/** signed 64 bit integer */
typedef HAVE_INT64 I8;

/** unsigned 64 bit integer */
typedef unsigned HAVE_INT64 U8;

/** signed int which is large enough to hold a pointer */
typedef HAVE_POINTER_INT IP;

/** unsigned int wich is large enough to hold a pointer */
typedef unsigned HAVE_POINTER_INT UP;

/** character */
typedef char C1;

/** signed 8 bit integer */
typedef signed char I1;

/** unsigned 8 bit integer */
typedef unsigned char U1;

typedef U8 UINT64;

/** 32 bit floating point */
typedef HAVE_FLOAT32 F4;

/** 64 bit floating point */
typedef HAVE_FLOAT64 F8;

/** if you want to print INT64, then do it like this:
    printf ( "%" INT64_PRINTF "d", ...), or "u" for unsigned INT64 */
#define INT64_PRINTF "l"

/** use a ## b to merge two names in preprocessor */
#define PAL_MERGE_TYPE_ANSI 1
#define HAVE_SYS_TIME_H 1
/* #undef HAVE_WINBASE_H */
#define HAVE_STDLIB_H 1
#define HAVE_UNISTD_H 1
#define STDC_HEADERS 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_STRTOK_R 1
#define HAVE_PREAD 1
#define HAVE_PWRITE 1
#define HAVE_ATOLL 1

/** if atoll is not available, take atol instead: */
#ifndef HAVE_ATOLL
#  define atoll atol 
#endif

#define PAL_STATIC_IN_HEADER( _x ) _x

/* cannot have workers without thread support, because util functions are not thread-safe then */
#ifdef PAL_WORKER
/** --enable-threads */
#  ifndef PAL_THREADS
#    define PAL_THREADS 1
#  endif
#endif
#endif
