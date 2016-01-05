/*
 * Copyright 2012-2015 Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents
 * related to the source code ("Material") are owned by Intel
 * Corporation or its suppliers or licensors. Title to the Material
 * remains with Intel Corporation or its suppliers and licensors. The
 * Material may contain trade secrets and proprietary and confidential
 * information of Intel Corporation and its suppliers and licensors,
 * and is protected by worldwide copyright and trade secret laws and
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
 * Unless otherwise agreed by Intel in writing, you may not remove or
 * alter this notice or any other notice embedded in Materials by Intel
 * or Intel's suppliers or licensors in any way.
 */

/* Example demonstrating how to embed Intel(R) Cluster Checker
 * functionality into an MPI program to verify the cluster before
 * running the main code.
 */

#include <mpi.h>

#include <iostream>
#include <sstream>

extern "C" bool clck_check(bool, const char *, size_t, size_t);

int main (int argc, char *argv[]) {
  char name[MPI_MAX_PROCESSOR_NAME], *namelist;
  int namelen, rank, size;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Get_processor_name(name, &namelen);

  if (rank == 0) {
    namelist = (char *)malloc(sizeof(char)*size*MPI_MAX_PROCESSOR_NAME);
  }

  MPI_Gather(&name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR,
             namelist, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 0, MPI_COMM_WORLD);

  if (rank == 0) {
    /* call Intel(R) Cluster Checker on the first rank */
    /* for more verbose output, set the argument to true */
    bool rc = clck_check(false, namelist, MPI_MAX_PROCESSOR_NAME, size);
    free(namelist);
    if (rc == false) {
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
  }

  MPI_Barrier(MPI_COMM_WORLD);

  std::stringstream h;

  h << "Hello world from rank " << rank << " of " << size << std::endl;
  std::cout << h.str();

  MPI_Finalize();

  return 0;
}
