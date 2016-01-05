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
#include <string.h>

// shrd_ofld 04 ................................................................
// This sample shows how to return a string from Target
//
// This sample intentionally reports a failure when the
// target is unavailable and the string is not returned
// from the target but instead the CPU.

#define V2 2
#define V3 3
#define V5 5

// Shared variable declarations for test shrd_ofld04
_Cilk_shared int res;
_Cilk_shared int val_2;
_Cilk_shared int val_3;
_Cilk_shared char str[21];

// Shared function declarations for test shrd_ofld04
_Cilk_shared int
int_ret_2() {
  val_2 = V2;
  return val_2;
}

_Cilk_shared int
int_ret_3() {
  val_3 = V3;
  return val_3;
}

char*
_Cilk_shared char_ret(int sum) {
#ifdef __MIC__
  if ((val_2+val_3) == sum) {
    memcpy(str,"PASS shrd_ofld04",16);
  }
  else {
    res++;
    memcpy(str,"*** FAIL shrd_ofld04",20);
  }
#else
  res++;
  memcpy(str,"*** FAIL shrd_ofld04",20);
#endif
  return str;
}

void
shrd_ofld04(void) {
  int sum;
  int sum1;
  char *res_str;
  res = 0;

  // Initialize on target
  sum1 = _Cilk_offload int_ret_2();
  sum =  _Cilk_offload int_ret_3();
  sum += sum1;

  // Return pass or fail from target
  res_str =  _Cilk_offload char_ret(sum);

  printf("%s\n", res_str);
}
//...........................................................................04
