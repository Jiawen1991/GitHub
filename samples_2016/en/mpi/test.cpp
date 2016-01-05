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
#include <iostream>

int main (int argc, char *argv[])
{
    int i, rank, size, namelen;
    char name[MPI_MAX_PROCESSOR_NAME];
    MPI::Status stat;

    MPI::Init (argc, argv);

    size = MPI::COMM_WORLD.Get_size ();
    rank = MPI::COMM_WORLD.Get_rank ();
    MPI::Get_processor_name (name, namelen);

    if (rank == 0) {

	std::cout << "Hello world: rank " << rank << " of " << size << " running on " << name << "\n";

	for (i = 1; i < size; i++) {
	    MPI::COMM_WORLD.Recv (&rank, 1, MPI_INT, i, 1, stat);
	    MPI::COMM_WORLD.Recv (&size, 1, MPI_INT, i, 1, stat);
	    MPI::COMM_WORLD.Recv (&namelen, 1, MPI_INT, i, 1, stat);
	    MPI::COMM_WORLD.Recv (name, namelen + 1, MPI_CHAR, i, 1, stat);
	    std::cout << "Hello world: rank " << rank << " of " << size << " running on " << name << "\n";
	}

    } else {

	MPI::COMM_WORLD.Send (&rank, 1, MPI_INT, 0, 1);
	MPI::COMM_WORLD.Send (&size, 1, MPI_INT, 0, 1);
	MPI::COMM_WORLD.Send (&namelen, 1, MPI_INT, 0, 1);
	MPI::COMM_WORLD.Send (name, namelen + 1, MPI_CHAR, 0, 1);

    }

    MPI::Finalize ();

    return (0);
}
