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
#include <new>

// shrd_ofld 08 ................................................................
// This sample tests dynamic allocation of aligned shared object

// Shared class variable declaration
class
_Cilk_shared
SHRD_OFLD8{
public:
   int    fieldi;
   double fieldd;
   float  fieldf;
   char   fieldc;
   short  fields;
   SHRD_OFLD8() {
     fieldi = 5;
     fieldd = 8888.7;
     fieldf = 9.666;
     fieldc = 'A';
     fields = 127;
   }
};

_Cilk_shared int res8;

_Cilk_shared void assign8(SHRD_OFLD8 _Cilk_shared *c)
{
    c->fieldi = 6;
    c->fieldd = 9999.7;
    c->fieldf = 10.666;
    c->fieldc = 'b';
    c->fields = 120;
}


void check_before8(SHRD_OFLD8 _Cilk_shared *c) {
   if (c->fieldi != 5)
      res8++;

   if (c->fieldd != 8888.7)
      res8++;

   if (c->fieldf != 9.666F)
      res8++;

   if (c->fieldc != 'A')
      res8++;

   if (c->fields != 127)
      res8++;
};

void check_after8(SHRD_OFLD8 _Cilk_shared *c) {
   if (c->fieldi != 6)
      res8++;

   if (c->fieldd != 9999.7)
      res8++;

   if (c->fieldf != 10.666F)
      res8++;

   if (c->fieldc != 'b')
      res8++;

   if (c->fields != 120)
      res8++;
};

SHRD_OFLD8 _Cilk_shared *c8;
void shrd_ofld08()
{

  c8 = new( _Offload_shared_aligned_malloc(sizeof(SHRD_OFLD8), 8)) _Cilk_shared SHRD_OFLD8;

  res8 = 0;

  check_before8(c8);

  _Cilk_offload assign8(c8);

  check_after8(c8);

  _Offload_shared_aligned_free(c8);

  if (res8 != 0)
      printf("*** FAIL shrd_ofld08\n");
  else
      printf("PASS shrd_ofld08\n");
}
//...........................................................................08
