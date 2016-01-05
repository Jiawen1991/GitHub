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
!#      Example Program Text from Sample shrd_sampleC
!#*****************************************************************************/

#include <stdio.h>

// shrd_ofld 03 ................................................................
// This sample uses shared structs

#define VSC 0x70
#define VSI 0x70000000
#define VSL 0x7000000000000000l
#define VSLL 0x7000000000000000ll
#define VF 10.0
#define VD 10101010.0
#define VLD 1010101010101010.0

typedef struct s {
  char        sc;
  int         si;
  long        sl;
  long long   sll;
  float       f;
  double      d;
  long double ld;
} shrd_struct;

// Shared variable declaration for test shrd_ofld03
_Cilk_shared shrd_struct SCI;

// Shared function declaration for test shrd_ofld03
_Cilk_shared
void init (void) {
  SCI.sc   = VSC;
  SCI.si   = VSI;
  SCI.sl   = VSL;
  SCI.sll  = VSLL;
  SCI.f    = VF;
  SCI.d    = VD;
  SCI.ld   = VLD;
}


static int
check(void) {
  int res = 0;
  if (SCI.sc != VSC)   res++;
  if (SCI.si != VSI)   res++;
  if (SCI.sl != VSL)   res++;
  if (SCI.sll != VSLL) res++;
  if (SCI.f != VF)     res++;
  if (SCI.d != VD)     res++;
  if (SCI.ld != VLD)   res++;
  return res;
}

void
shrd_ofld03(void) {
  int res;

  _Cilk_offload init();

  res = check();

  if (res == 0)
    printf("PASS shrd_ofld03\n");
  else
    printf("*** FAIL shrd_ofld03\n");
}
//...........................................................................03
