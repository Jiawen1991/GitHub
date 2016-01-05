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

// shrd_ofld_vt01 ..............................................................
// This sample tests assignment to shared class field both inside a 
// constructor executed on the host and a function executed on the target

class
_Cilk_shared
SHRD_OFLD_VT1{
private:
   int field1;
   public:
_Cilk_shared
   virtual int vtfunc1();
};


_Cilk_shared
int SHRD_OFLD_VT1::vtfunc1()
{
#ifdef __MIC__
  return 9;
#else
  return 8;
#endif
}

_Cilk_shared SHRD_OFLD_VT1 cvt1;

void shrd_ofld_vt01()
{
  int res;

   // Offload the virtual function on target
   res = _Cilk_offload cvt1.vtfunc1();

  if (res != 9)
      printf("*** FAIL shrd_ofld_vt01\n");
  else
      printf("PASS shrd_ofld_vt01\n");
}
//.........................................................................vt01
