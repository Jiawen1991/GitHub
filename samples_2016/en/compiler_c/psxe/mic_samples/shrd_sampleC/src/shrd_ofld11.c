/*******************************************************************************
!#
!#  Copyright (C) 2012-2015 Intel Corporation. All Rights Reserved.
!#
!#  The source code contained or described herein and all
!#  documents related to the source code ("Material") are owned by
!#  Intel Corporation or its suppliers or licensors. Title to the
!#  Material remains with Intel Corporation or its suppliers and
!#  licensors. The Material is protected by worldwide copyright
!#  laws and treaty provisions.  No part of the Material may be
!#  used, copied, reproduced, modified, published, uploaded,
!#  posted, transmitted, distributed,  or disclosed in any way
!#  except as expressly provided in the license provided with the
!#  Materials.  No license under any patent, copyright, trade
!#  secret or other intellectual property right is granted to or
!#  conferred upon you by disclosure or delivery of the Materials,
!#  either expressly, by implication, inducement, estoppel or
!#  otherwise, except as expressly provided in the license
!#  provided with the Materials.
!#
!#
!#******************************************************************************
!# Content:
!#      Example Program Text from Sample shrd_sampleC
!#*****************************************************************************/

#include <stdio.h>
#include <math.h>

// shrd_ofld 11 ................................................................
// This sample uses compiler intrinsic for allocating shared malloc
// All variables exchanged between CPU and target are lexically visible
// therefore, they do not need to be explicitly named

// Pointer to shared memory
int * _Cilk_shared ptr;

_Cilk_shared init11() {

#ifdef __MIC__
   *ptr = 9;
#else
   *ptr = 0;
#endif

}

void
shrd_ofld11(void) {

   // Malloc shared memory using compiler intrinsic
   #ifdef __INTEL_OFFLOAD
   ptr = (_Cilk_shared int *) _Offload_shared_malloc(4);
   #else
   ptr = (int *) malloc(4);
   #endif

  // Initialize shared memory
  _Cilk_offload init11();

  // Check results
  if (*ptr == 9)
  printf("PASS shrd_ofld11\n");
  else
  printf("*** FAIL shrd_ofld11\n");
  
  // Free shared memory
  #ifdef __INTEL_OFFLOAD
  _Offload_shared_free(ptr);
  #else
  free(ptr);
  #endif
}
//...........................................................................11
