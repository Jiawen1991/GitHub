/***************************************************************************************
 *
 * Intel(R) Trace Collector MPI Profiling Library.
 *
 * Copyright 1997-2007 Intel Corporation.
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

/** 
 * mpiconstants
 *
 * This program prints as many information about MPI as necessary
 * to check whether an MPI implementation is binary-compatible with
 * a precompiled ITC library.
 *
 * You can compile it like this:
 *     <mpicc> -c mpiconstants.c
 *     <cc> -o mpiconstants mpiconstants.o
 * to get a normal application or
 *     <mpicc> -DMPI -o mpiconstants mpiconstants.c
 * to get a real MPI application (if it does not link without
 * MPI).
 *
 */

#include <stdio.h>
#include <mpi.h>



int main(int argc, char **argv) {
    MPI_Status *local_stat = NULL;
    int rank = 0;

#ifdef MPI
    MPI_Init(&argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    if( rank ) {
	MPI_Finalize();
	return 0;
    }

    /* only rank 0 prints */
#endif

    fprintf(stdout,"Datatypes:\n\n");
    fprintf(stdout,
	    "   sizeof(MPI_Datatype): %d\n"
	    "   sizeof(MPI_Comm)    : %d\n"
	    "   sizeof(MPI_Request) : %d\n",
	    (int)sizeof(MPI_Datatype),
	    (int)sizeof(MPI_Comm),
	    (int)sizeof(MPI_Request));
	    
    fprintf(stdout,"\nC constants: \n\n");
    fprintf(stdout,
	    "   MPI_CHAR            : %ld\n"
	    "   MPI_BYTE            : %ld\n"
	    "   MPI_SHORT           : %ld\n"
	    "   MPI_INT             : %ld\n"
	    "   MPI_FLOAT           : %ld\n"
	    "   MPI_DOUBLE          : %ld\n"
	    "   MPI_COMM_WORLD      : %ld\n"
	    "   MPI_COMM_SELF       : %ld\n",
	    (long)MPI_CHAR,
	    (long)MPI_BYTE,
	    (long)MPI_SHORT,
	    (long)MPI_INT,
	    (long)MPI_FLOAT,
	    (long)MPI_DOUBLE,
	    (long)MPI_COMM_WORLD,
	    (long)MPI_COMM_SELF);

    fprintf(stdout,"\nMPI_Status structure and byte offsets of members:\n\n");
    fprintf(stdout,"   MPI_STATUS_SIZE     : %d\n",(int)((sizeof(*local_stat))/(sizeof(int))));
    fprintf(stdout,"   MPI_SOURCE          : %ld\n",(long)&local_stat->MPI_SOURCE);
    fprintf(stdout,"   MPI_TAG             : %ld\n",(long)&local_stat->MPI_TAG);
    fprintf(stdout,"   MPI_ERROR           : %ld\n",(long)&local_stat->MPI_ERROR);

#ifdef MPI
    MPI_Finalize();
#endif

    return 0;
}
