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

// shrd_ofld 05 ................................................................
// This sample tests single inheritance of shared class objects

// Shared class variable declaration
class
_Cilk_shared
SHRD_OFLD5A{
public:
   int    fieldA;
   SHRD_OFLD5A() {
     fieldA = 5;
   }
};

class
_Cilk_shared
SHRD_OFLD5B : public SHRD_OFLD5A{
public:
   int    fieldA;
   int    fieldB;
   SHRD_OFLD5B() {
     fieldA = 5;
     fieldB = 7;
   }
};

// Shared class object definition
_Cilk_shared SHRD_OFLD5B c5;

_Cilk_shared int res5;

_Cilk_shared void assign5()
{
#ifdef __MIC__
    c5.fieldA = 6;
#else
    c5.fieldA = 5;
#endif
}

void check5() {
   if (c5.SHRD_OFLD5A::fieldA != 5)
      res5++;

   if (c5.fieldA != 6)
      res5++;

   if (c5.fieldB != 7)
      res5++;
};

void shrd_ofld05()
{
  res5 = 0;
  _Cilk_offload assign5();
  check5();

  if (res5 != 0)
      printf("*** FAIL shrd_ofld05\n");
  else
      printf("PASS shrd_ofld05\n");
}
//...........................................................................05
