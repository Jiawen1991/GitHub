! Copyright 2012-2015 Intel Corporation All Rights Reserved.
!
! The source code contained or described herein and all documents
! related to the source code ("Material") are owned by Intel
! Corporation or its suppliers or licensors. Title to the Material
! remains with Intel Corporation or its suppliers and licensors. The
! Material may contain trade secrets and proprietary and confidential
! information of Intel Corporation and its suppliers and licensors,
! and is protected by worldwide copyright and trade secret laws and
! treaty provisions. No part of the Material may be used, copied,
! reproduced, modified, published, uploaded, posted, transmitted,
! distributed, or disclosed in any way without Intel's prior express
! written permission.
!
! No license under any patent, copyright, trade secret or other
! intellectual property right is granted to or conferred upon you by
! disclosure or delivery of the Materials, either expressly, by
! implication, inducement, estoppel or otherwise. Any license under
! such intellectual property rights must be express and approved by
! Intel in writing.
!
! Unless otherwise agreed by Intel in writing, you may not remove or
! alter this notice or any other notice embedded in Materials by Intel
! or Intel's suppliers or licensors in any way.

! Example demonstrating how to embed Intel(R) Cluster Checker
! functionality into an MPI program to verify the cluster before
! running the main code.

      program main
      implicit none
      include 'mpif.h'

      integer ierr, maxranks, namelen, rank, size
      parameter (maxranks=1024)
      character(MPI_MAX_PROCESSOR_NAME) name
      character(MPI_MAX_PROCESSOR_NAME) namelist(0:maxranks)

      call MPI_Init(ierr)

      call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
      call MPI_Comm_size(MPI_COMM_WORLD, size, ierr)
      call MPI_Get_processor_name(name, namelen, ierr)

      call MPI_Gather(name, MPI_MAX_PROCESSOR_NAME, MPI_CHARACTER,
     &                namelist, MPI_MAX_PROCESSOR_NAME, MPI_CHARACTER,
     &                0, MPI_COMM_WORLD, ierr)

      if (rank .eq. 0) then
!     call Intel(R) Cluster Checker on the first rank
!     for more verbose output, set the argument to true
         call clck_check(.false., namelist, size, ierr)
         if (ierr .ne. 0) then
            call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
         endif
      endif

      call MPI_Barrier(MPI_COMM_WORLD, ierr)

      print *, 'Hello world from rank ', rank, ' of ', size

      call MPI_Finalize(ierr)

      end
