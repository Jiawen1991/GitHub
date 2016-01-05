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
 * This is a program that generates a certain callstack
 * without actually executing any of the functions that
 * it logs.
 *
 */

#include <VT.h>
#include <ctype.h>
#include <string.h>
#include "VT_usleep.c"

int main( int argc, char **argv )
{
    /* a very simple description of the callstack that we want to generate */
    char *callstack[] = {
	"init",
	" lib1:lib1_main",
	"  sys:open",
	"  sys:read",
	"  sys:close",
	"  lib1:lib1_util",
	" lib1:lib1_fini",
	"lib4:lib4_log",
	" sys:write",
	"work",
	" lib2:lib2_setup",
	"  lib4:lib4_log",
	"   sys:write",
	"  lib3:lib3_get",
	"   sys:read",
	" lib4:lib4_log",
	"  sys:write",
	"finalize",
	" lib2:lib2_end"
    };
    int i, level = 0;

    VT_initialize( &argc, &argv );

    for( i = 0; i < sizeof(callstack)/sizeof(callstack[0]); i++ ) {
	int newlevel;
	char *class, *function;
	int classid, functionid;
	char buffer[80];

	for( newlevel = 0, class = callstack[i];
	     isspace( *class );
	     newlevel++, class++ )
	    ;

	/* get out of previous functions until we are at the right level */
	while( newlevel < level ) {
            VT_usleep( 1000000 );
	    VT_leave( VT_NOSCL );
	    level--;
	}

	/* define the new function and enter it */
	function = strchr( class, ':' );
	if( function ) {
	    strncpy( buffer, class, function - class );
	    buffer[function - class] = 0;
	    function++;
	    class = buffer;
	} else {
	    function = class;
	    class = "Application";
	}
	VT_classdef( class, &classid );
	VT_funcdef( function, classid, &functionid );
        VT_usleep( 1000000 );
	VT_enter( functionid, VT_NOSCL );
	level++;
    }
    while( level ) {
        VT_usleep( 1000000 );
	VT_leave( VT_NOSCL );
	level--;
    }

    VT_finalize();
    return 0;
}
