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

// shrd_ofld 11 ................................................................
// This sample tests static member of a shared class is same on host 
// and target

// Shared class variable declaration
class
_Cilk_shared
SHRD_OFLD11{
public:
   static int no_objs_count;
   SHRD_OFLD11() {
      no_objs_count++;
   };
};

// Initialize static member field
int SHRD_OFLD11::no_objs_count = 0;

// Shared class object definition
_Cilk_shared
SHRD_OFLD11 c11;

_Cilk_shared int res11 = 0;

// Shared function definition
_Cilk_shared
void
testtarget() {
  SHRD_OFLD11 c3;
  // count should be 3
  if (c3.no_objs_count != 3)
     res11++;
}

void
testhost() {
  SHRD_OFLD11 c2;
  // count should be 2
  if (c2.no_objs_count != 2)
     res11++;
}

void shrd_ofld11()
{
   
  // count should 1
  if (c11.no_objs_count != 1)
     res11++;

   // test static field member on host
   testhost();

   // test static field member on target 
   _Cilk_offload testtarget();

  if (res11 != 0)
      printf("*** FAIL shrd_ofld11\n");
  else
      printf("PASS shrd_ofld11\n");
}
//...........................................................................11
