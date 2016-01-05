/*
    Copyright 2003-2013 Intel Corporation.  All Rights Reserved.

    The source code contained or described herein and all documents
    related to the source code ("Material") are owned by Intel Corporation
    or its suppliers or licensors.  Title to the Material remains with
    Intel Corporation or its suppliers and licensors.  The Material is
    protected by worldwide copyright and trade secret laws and treaty
    provisions.  No part of the Material may be used, copied, reproduced,
    modified, published, uploaded, posted, transmitted, distributed, or
    disclosed in any way without Intel's prior express written permission.

    No license under any patent, copyright, trade secret or other
    intellectual property right is granted to or conferred upon you by
    disclosure or delivery of the Materials, either expressly, by
    implication, inducement, estoppel or otherwise.  Any license under
    such intellectual property rights must be express and approved by
    Intel in writing.
*/
#include "mpi.h"
#include <stdio.h>
#include <string.h>

int
main (int argc, char *argv[])
{
    int i, rank, size, namelen;
    char name[MPI_MAX_PROCESSOR_NAME];
    MPI_Status stat;

    MPI_Init (&argc, &argv);

    MPI_Comm_size (MPI_COMM_WORLD, &size);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);
    MPI_Get_processor_name (name, &namelen);

    if (rank == 0) {

	printf ("Hello world: rank %d of %d running on %s\n", rank, size, name);

	for (i = 1; i < size; i++) {
	    MPI_Recv (&rank, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &stat);
	    MPI_Recv (&size, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &stat);
	    MPI_Recv (&namelen, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &stat);
	    MPI_Recv (name, namelen + 1, MPI_CHAR, i, 1, MPI_COMM_WORLD, &stat);
	    printf ("Hello world: rank %d of %d running on %s\n", rank, size, name);
	}

    } else {

	MPI_Send (&rank, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
	MPI_Send (&size, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
	MPI_Send (&namelen, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
	MPI_Send (name, namelen + 1, MPI_CHAR, 0, 1, MPI_COMM_WORLD);

    }

    MPI_Finalize ();

    return (0);
}
