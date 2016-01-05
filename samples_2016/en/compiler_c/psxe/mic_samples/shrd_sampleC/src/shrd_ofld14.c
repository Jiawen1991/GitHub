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

// shrd_ofld 14 ................................................................
// This sample shows how to use multiple targets and return a 
// string from one target
//
// This sample requires a target be available. When no target
// is available, execution does not default to the CPU and the
// sample reports a failure.
//

#define V2 2
#define V3 3
#define V5 5

// Shared variable declaration for test shrd_ofld14
_Cilk_shared int res;

// Pointer to shared memory
int _Cilk_shared * pres;

_Cilk_shared int val_2_multi;
_Cilk_shared int val_3_multi;
_Cilk_shared char str_multi[21];

// Shared function declarations for test shrd_ofld14
_Cilk_shared int
int_ret_2_multi() {
  val_2_multi = V2;
  return val_2_multi;
}

_Cilk_shared int
int_ret_3_multi() {
  val_3_multi = V3;
  return val_3_multi;
}

char*
_Cilk_shared char_ret_multi(int sum) {

#ifdef __MIC__
  if (((val_2_multi+val_3_multi)*2) == sum) {
    memcpy(str_multi,"PASS shrd_ofld14",16);
  }
  else {
    res++;
    memcpy(str_multi,"*** FAIL shrd_ofld14",20);
  }
#else
  res++;
  memcpy(str_multi,"*** FAIL shrd_ofld14",20);
#endif

  return str_multi;
}

void
shrd_ofld14(void) {
  int sum = 0;
  int sum1;
  char *res_str;
  res = 0;

  // Check if target device(s) are installed and available
  #ifdef __INTEL_OFFLOAD
  int num_devices = _Offload_number_of_devices();
  #else
  int num_devices = 0;
  #endif

  if (num_devices == 0) {
     printf("*** FAIL shrd_ofld14 - target unavailable\n");
     return;
  }

  // Initialize on multiple targets
  sum1 = _Cilk_offload_to(0) int_ret_2_multi();
  sum += sum1;
  sum1 =  _Cilk_offload_to(1)  int_ret_2_multi();
  sum += sum1;
  sum1 = _Cilk_offload_to(2) int_ret_3_multi();
  sum += sum1;
  sum1 =  _Cilk_offload_to(3)  int_ret_3_multi();
  sum += sum1;

  // Return pass or fail string from target
  res_str =  _Cilk_offload char_ret_multi(sum);

  printf("%s\n", res_str);
}
//...........................................................................14
