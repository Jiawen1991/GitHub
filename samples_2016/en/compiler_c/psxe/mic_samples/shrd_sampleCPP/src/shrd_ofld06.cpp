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

// shrd_ofld 06 ................................................................
// This sample tests multiple inheritance of shared class objects

// Shared class variable declaration
class
_Cilk_shared
SHRD_OFLD6A{
public:
   int    fieldC;
   int    fieldA;
   SHRD_OFLD6A() {
     fieldA = 17;
   }
};

class
_Cilk_shared
SHRD_OFLD6B{
public:
   int    fieldC;
   int    fieldB;
   SHRD_OFLD6B() {
     fieldB = 27;
   }
};

class
_Cilk_shared
SHRD_OFLD6C : public SHRD_OFLD6A, public SHRD_OFLD6B{
public:
   int    fieldC;
   int    fieldA;
   int    fieldB;
   int    fieldC1;
   SHRD_OFLD6C() {
     fieldC1 = 39;
   }
};

// Shared class object definition
_Cilk_shared SHRD_OFLD6C c6;

_Cilk_shared int res6;

_Cilk_shared void assign6()
{
    c6.SHRD_OFLD6A::fieldC = 16;
    c6.SHRD_OFLD6B::fieldC = 26;
    c6.fieldC = 36;
    c6.fieldA = 37;
    c6.fieldB = 38;
}

void check6() {
   if (c6.SHRD_OFLD6A::fieldC != 16)
      res6++;

   if (c6.SHRD_OFLD6A::fieldA != 17)
      res6++;

   if (c6.SHRD_OFLD6B::fieldC != 26)
      res6++;

   if (c6.SHRD_OFLD6B::fieldB != 27)
      res6++;

   if (c6.fieldC != 36)
      res6++;

   if (c6.fieldA != 37)
      res6++;

   if (c6.fieldB != 38)
      res6++;

   if (c6.fieldC1 != 39)
      res6++;
};

void shrd_ofld06()
{
  res6 = 0;
  _Cilk_offload assign6();
  check6();

  if (res6 != 0)
      printf("*** FAIL shrd_ofld06\n");
  else
      printf("PASS shrd_ofld06\n");
}
//...........................................................................06
