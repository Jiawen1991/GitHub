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

// shrd_ofld 03 ................................................................
// This sample tests assignments to shared class field of various types 
// inside a constructor are correct on target 

// Shared class variable declaration
class
_Cilk_shared
SHRD_OFLD3{
public:
   int    fieldi;
   double fieldd;
   float  fieldf;
   char   fieldc;
   short  fields;
   SHRD_OFLD3() {
     fieldi = 5;
     fieldd = 8888.7;
     fieldf = 9.666;
     fieldc = 'A';
     fields = 127;
   }
};

// Shared class object definition
_Cilk_shared SHRD_OFLD3 c3;

_Cilk_shared int res;

void check() {

   if (c3.fieldi != 5)
      res++;

   if (c3.fieldd != 8888.7)
      res++;

   if (c3.fieldf != 9.666F)
      res++;

   if (c3.fieldc != 'A')
      res++;

   if (c3.fields != 127)
      res++;

};

void shrd_ofld03()
{
  res = 0;
  check();

  if (res != 0)
      printf("*** FAIL shrd_ofld03\n");
  else
      printf("PASS shrd_ofld03\n");
}
//...........................................................................03
