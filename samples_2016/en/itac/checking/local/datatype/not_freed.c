/***************************************************************************************
 *
 * Intel(R) Trace Collector Library.
 *
 * Copyright 2004-2008 Intel Corporation.
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
 ***************************************************************************************/

#include <mpi.h>
#include <assert.h>

/**
 * This example covers datatype leak reporting. It is expected to be run with
 * one process and CHECK-MAX-DATATYPES 4.
 *
 * MPI_Type_lb() is used to check whether invalid and valid datatypes are properly
 * distinguished.
 */

int main (int argc, char **argv)
{
    int i, e;
    MPI_Aint lb;
    MPI_Datatype type, types[4];
    MPI_Aint indices[1];
    int blocklens[1];
    MPI_Datatype oldtypes[1];

    MPI_Init( &argc, &argv );

    indices[0] = 0;
    blocklens[0] = 2;
    oldtypes[0] = MPI_CHAR;

    /* generate four distinct datatypes - should trigger a leak report */
    for( i = 0; i < 4; i++ ) {
        MPI_Type_vector( 1, i+2, i+2, MPI_CHAR, types + i );
        MPI_Type_commit( types + i );
        MPI_Type_lb( types[i], &lb );
    }
    /* now free one and create it again - should not trigger a report */
    type = types[3];
    MPI_Type_free( types + 3 );
    MPI_Type_lb( type, &lb );
    MPI_Type_vector( 1, 3+2, 3+2, MPI_CHAR, types + 3 );
    MPI_Type_commit( types + 3 );

    /* creating the same type over and over again is expected to create new handles */
    for( i = 0; i < 2; i++ ) {
        MPI_Type_vector( 1, 10, 10, MPI_CHAR, types + i );
        MPI_Type_commit( types + i );
        assert( i == 0 || types[i] != types[0] );
    }
    /* each type has to be freed once */
    type = types[1];
    MPI_Type_free( types + 1 );
    MPI_Type_lb( type, &lb );
    MPI_Type_lb( types[0], &lb );
    /* now trigger leak report */
    for( i = 1; i < 4; i++ ) {
        MPI_Type_vector( 1, 10, 10, MPI_CHAR, types + i );
        MPI_Type_commit( types + i );
        assert( i == 0 || types[i] != types[0] );
    }

    /* create the same type (two chars) in four other ways, thus trigger a leak report */
    MPI_Type_hvector( 1, 2, 2, MPI_CHAR, &type );
    MPI_Type_commit( &type );
    MPI_Type_commit( &type ); /* should have no effect */
    MPI_Type_contiguous( 2, MPI_CHAR, &type );
    /* MPI_Type_commit( &type ); */ /* intentionally missing - should be included in leak report anyway */
    MPI_Type_hindexed( 1, blocklens, indices, MPI_CHAR, &type );
    MPI_Type_commit( &type );
    MPI_Type_struct( 1, blocklens, indices, oldtypes, &type );
    MPI_Type_commit( &type );

    /*
     * Create a custom type, then "duplicate" it by creating
     * a type derived from it which is identical to it
     * (in MPI-2 one could use MPI_Type_dup()). First create
     * and free without commit, then with.
     *
     * None of this should trigger an error. 
     */
    MPI_Type_contiguous( 2, MPI_CHAR, types + 0 );
    MPI_Type_contiguous( 1, types[0], types + 1 );
    MPI_Type_free( types + 0 );
    MPI_Type_free( types + 1 );

    MPI_Type_contiguous( 2, MPI_CHAR, types + 0 );
    MPI_Type_commit( types + 0);
    MPI_Type_contiguous( 1, types[0], types + 1 );
    MPI_Type_commit( types + 1 );
    MPI_Type_free( types + 0 );
    MPI_Type_free( types + 1 );
    

    MPI_Finalize( );

    return 0;
}
