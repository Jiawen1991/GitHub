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

// shrd_ofld_vt02 ..............................................................
// This sample tests single virtual function inheritance

class
_Cilk_shared
SHRD_OFLD_VT2A{
public:
   int field1A;
   int field2A;
   SHRD_OFLD_VT2A() { 
      field1A = 5; 
      field2A = 6; 
   }
_Cilk_shared
   virtual void vtfunc1A();
_Cilk_shared
   virtual void vtfunc2A();
};

_Cilk_shared
void SHRD_OFLD_VT2A::vtfunc1A() {
    field1A += 10;
}

_Cilk_shared
void SHRD_OFLD_VT2A::vtfunc2A() {
    field2A += 10;
}

class
_Cilk_shared
SHRD_OFLD_VT2B : public SHRD_OFLD_VT2A{
public:
   int field1A;
   int field1B;
   int field2B;
   SHRD_OFLD_VT2B() { 
      field1A = 7; 
      field1B = 8; 
      field2B = 9; 
   }
_Cilk_shared
   virtual void vtfunc1A();
_Cilk_shared
   virtual void vtfunc1B();
};

_Cilk_shared
void SHRD_OFLD_VT2B::vtfunc1A() {
    field1A += 10;
}

_Cilk_shared
void SHRD_OFLD_VT2B::vtfunc1B() {
    field1B += 10;
    field2B += 10;
}

_Cilk_shared int resvt2;

_Cilk_shared SHRD_OFLD_VT2B cvt2;

void checkvt2(int i)
{
    if (cvt2.SHRD_OFLD_VT2A::field1A != (5+i))
        resvt2++;

    if (cvt2.SHRD_OFLD_VT2A::field2A != (6+i))
        resvt2++;

    if (cvt2.field1A != (7+i))
        resvt2++;

    if (cvt2.field1B != (8+i))
        resvt2++;

    if (cvt2.field2B != (9+i))
        resvt2++;
}

void shrd_ofld_vt02()
{
   resvt2 = 0;

   // Check if constructors are executed correctly
   checkvt2(0);

   // Offload the virtual function on target
   _Cilk_offload cvt2.SHRD_OFLD_VT2A::vtfunc1A();
   _Cilk_offload cvt2.SHRD_OFLD_VT2A::vtfunc2A();
   _Cilk_offload cvt2.vtfunc1A();
   _Cilk_offload cvt2.vtfunc1B();

   // Check if offload functions are executed correctly
   checkvt2(10);

  if (resvt2 != 0)
      printf("*** FAIL shrd_ofld_vt02\n");
  else
      printf("PASS shrd_ofld_vt02\n");
}
//.........................................................................vt02
