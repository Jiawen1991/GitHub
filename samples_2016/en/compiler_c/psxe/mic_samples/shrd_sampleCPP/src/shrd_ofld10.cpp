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

// shrd_ofld 10 ................................................................
// This sample tests passing address of shared class object as parameter

// Shared class variable declaration
class
_Cilk_shared
SHRD_OFLD10{
public:
   int field1;
   SHRD_OFLD10() {field1 = 5;};
};

// Shared class object definition
_Cilk_shared
SHRD_OFLD10 c10;

// Shared function definition
_Cilk_shared
void
assign10(SHRD_OFLD10 _Cilk_shared *c1) {
#ifdef __MIC__
   c1->field1 = 7;
#else
   c1->field1 = 6;
#endif
}

void shrd_ofld10()
{
  int res = 0;

  if (c10.field1 != 5)
     res++;

   _Cilk_offload assign10(&c10);

  if (c10.field1 != 7)
     res++;

  if (res != 0)
      printf("*** FAIL shrd_ofld10\n");
  else
      printf("PASS shrd_ofld10\n");
}
//...........................................................................10
