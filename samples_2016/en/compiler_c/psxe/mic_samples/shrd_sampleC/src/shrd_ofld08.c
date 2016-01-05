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
#include <stdlib.h>

// shrd_ofld 08 ................................................................
// This sample uses Cilk_spawn for offloading - all offloads are asynchronous

#include "shrd_ofld_util.h"

void
shrd_ofld08(void) {
  int res;
  sc = _Cilk_spawn _Cilk_offload foo_char(1);
  si = _Cilk_spawn _Cilk_offload foo_int(1);
  sl = _Cilk_spawn _Cilk_offload foo_long(1);
  sll = _Cilk_spawn _Cilk_offload foo_longlong(1);
  f = _Cilk_spawn _Cilk_offload foo_float(1);
  d = _Cilk_spawn _Cilk_offload foo_double(1);
  ld = _Cilk_spawn _Cilk_offload foo_longdouble(1);
  _Cilk_sync;

  res = check();

  if (res == 0)
    printf("PASS shrd_ofld08\n");
  else
    printf("*** FAIL shrd_ofld08\n");
}
//...........................................................................08
