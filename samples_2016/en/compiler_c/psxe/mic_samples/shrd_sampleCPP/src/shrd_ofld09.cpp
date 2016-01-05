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
!#      Example Program Text from Sample shrd_sampleCPP
!#*****************************************************************************/

#include <stdio.h>

// shd_ofld 09 ................................................................
// This sample tests push and pop pragma to mark a region of code as shared

// Begin Shared region
#pragma offload_attribute(push, _Cilk_shared)

class
SHRD_OFLD9{
public:
   int field1;
   SHRD_OFLD9() {field1 = 5;};
};

SHRD_OFLD9 c9;

void
assign9() {
#ifdef __MIC__
   c9.field1 = 7;
#else
   c9.field1 = 6;
#endif
}

// End Shared region
#pragma offload_attribute(pop)

void shrd_ofld09()
{
  int res = 0;

  if (c9.field1 != 5)
     res++;

   _Cilk_offload assign9();

  if (c9.field1 != 7)
     res++;

  if (res != 0)
      printf("*** FAIL shrd_ofld09\n");
  else
      printf("PASS shrd_ofld09\n");
}
//...........................................................................09
