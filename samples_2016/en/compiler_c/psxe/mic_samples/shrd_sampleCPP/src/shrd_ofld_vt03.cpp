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

// shrd_ofld_vt03 ..............................................................
// This sample tests single virtual function multiple inheritance

class
_Cilk_shared
SHRD_OFLD_VT3A{
public:
   int field1;
   int field2;
   SHRD_OFLD_VT3A() { 
      field1 = 2; 
      field2 = 3; 
   }
_Cilk_shared
   virtual void vtfuncA();
_Cilk_shared
   virtual void vtfuncC();
};

_Cilk_shared
void SHRD_OFLD_VT3A::vtfuncA() {
    field1 += 10;
}

_Cilk_shared
void SHRD_OFLD_VT3A::vtfuncC() {
    field2 += 10;
}

class
_Cilk_shared
SHRD_OFLD_VT3B{
public:
   int field1;
   int field2;
   SHRD_OFLD_VT3B() { 
      field1 = 4; 
      field2 = 5; 
   }
_Cilk_shared
   virtual void vtfuncB();
_Cilk_shared
   virtual void vtfuncC();
};

_Cilk_shared
void SHRD_OFLD_VT3B::vtfuncB() {
    field1 += 10;
}

_Cilk_shared
void SHRD_OFLD_VT3B::vtfuncC() {
    field2 += 10;
}

class
_Cilk_shared
SHRD_OFLD_VT3C : public SHRD_OFLD_VT3A, public SHRD_OFLD_VT3B{
public:
   int field1;
   int field2;
   int field3;
   SHRD_OFLD_VT3C() { 
      field1 = 6; 
      field2 = 7; 
      field3 = 8; 
   }
_Cilk_shared
   virtual void vtfuncA();
_Cilk_shared
   virtual void vtfuncB();
_Cilk_shared
   virtual void vtfuncC();
};

_Cilk_shared
void SHRD_OFLD_VT3C::vtfuncA() {
    field1 += 10;
}

_Cilk_shared
void SHRD_OFLD_VT3C::vtfuncB() {
    field2 += 10;
}

_Cilk_shared
void SHRD_OFLD_VT3C::vtfuncC() {
    field3 += 10;
}

_Cilk_shared int resvt3;

_Cilk_shared SHRD_OFLD_VT3C cvt3;

void checkvt3(int i)
{
    if (cvt3.field1 != (6+i))
        resvt3++;

    if (cvt3.field2 != (7+i))
        resvt3++;

    if (cvt3.field3 != (8+i))
        resvt3++;
}

void shrd_ofld_vt03()
{
   resvt3 = 0;

   // Check if constructors are executed correctly
   checkvt3(0);

   // Offload the virtual function on target
   _Cilk_offload cvt3.vtfuncA();
   _Cilk_offload cvt3.vtfuncB();
   _Cilk_offload cvt3.vtfuncC();

   // Check if offload functions are executed correctly
   checkvt3(10);

  if (resvt3 != 0)
      printf("*** FAIL shrd_ofld_vt03\n");
  else
      printf("PASS shrd_ofld_vt03\n");
}
//.........................................................................vt03
