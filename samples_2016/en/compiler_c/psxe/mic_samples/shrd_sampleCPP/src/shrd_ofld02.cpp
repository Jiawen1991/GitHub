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

// shrd_ofld 02 ................................................................
// This sample tests assignment to shared class field inside a constructor
// on host and a field assignment executed on the target

// Shared class variable declaration
class
_Cilk_shared
SHRD_OFLD2{
public:
   int field1;
   int field2;
   SHRD_OFLD2() {field1 = 3; field2 = 4;};
   void assign1();
   void assign2() {
#ifdef __MIC__
       field2 = 5;
#else
       field2 = 6;
#endif
   }
};

// Shared class object definition
_Cilk_shared
SHRD_OFLD2 c2;

// Shared class member function definition outside class
void
SHRD_OFLD2::assign1() {
#ifdef __MIC__
   field1 = 7;
#else
   field1 = 8;
#endif
}

void shrd_ofld02()
{
  int res = 0;

  if (c2.field1 != 3)
     res++;
  if (c2.field2 != 4)
     res++;

   _Cilk_offload c2.assign1();

  if (c2.field1 != 7)
     res++;

   _Cilk_offload c2.assign2();

  if (c2.field2 != 5)
     res++;

  if (res != 0)
      printf("*** FAIL shrd_ofld02\n");
  else
      printf("PASS shrd_ofld02\n");
}
//...........................................................................02
