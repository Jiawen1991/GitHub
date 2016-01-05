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

// shrd_ofld 06 ................................................................
// This sample uses Cilk Spawn to offload a function

_Cilk_shared int
foo(void) {
#ifdef __MIC__
  return 0;
#else
  return 1;
#endif
}

void
shrd_ofld06(void) {
  int res;

  res = _Cilk_spawn _Cilk_offload foo();
  _Cilk_sync;

  if (res == 0)
    printf("PASS shrd_ofld06\n");
  else
    printf("*** FAIL shrd_ofld06\n");
}
//...........................................................................06
