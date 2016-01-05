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

// shrd_ofld 04 ................................................................
// This sample tests passing pointer to shared class object 
// to a target function

// Shared class variable declaration
class
_Cilk_shared
SHRD_OFLD4{
public:
   int    fieldi;
   double fieldd;
   float  fieldf;
   char   fieldc;
   short  fields;
   SHRD_OFLD4() {
     fieldi = 5;
     fieldd = 8888.7;
     fieldf = 9.666;
     fieldc = 'A';
     fields = 127;
   }
};

// Shared class object definition
_Cilk_shared SHRD_OFLD4 c4;

_Cilk_shared int res4;

void check(SHRD_OFLD4 *c) {

   if (c->fieldi != 5)
      res4++;

   if (c->fieldd != 8888.7)
      res4++;

   if (c->fieldf != 9.666F)
      res4++;

   if (c->fieldc != 'A')
      res4++;

   if (c->fields != 127)
      res4++;

};

void shrd_ofld04()
{
  res4 = 0;
  check(&c4);

  if (res4 != 0)
      printf("*** FAIL shrd_ofld04\n");
  else
      printf("PASS shrd_ofld04\n");
}
//...........................................................................04
