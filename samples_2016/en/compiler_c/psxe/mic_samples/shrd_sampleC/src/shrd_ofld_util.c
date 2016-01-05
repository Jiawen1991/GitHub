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

// util routines ............................................................
// Define routines used in other samples

#define VSC 0x70
#define VSI 0x70000000
#define VSL 0x7000000000000000l
#define VSLL 0x7000000000000000ll
#define VF 10.0
#define VD 10101010.0
#define VLD 1010101010101010.0

char        sc;
int         si;
long        sl;
long long   sll;
float       f;
double      d;
long double ld;

_Cilk_shared char
foo_char(char a) {
  return a+VSC;
}

_Cilk_shared int
foo_int(int a) {
  return a+VSI;
}

_Cilk_shared long
foo_long(long a) {
  return a+VSL;
}

_Cilk_shared long long
foo_longlong(long long a) {
  return a+VSLL;
}

_Cilk_shared float
foo_float(float a) {
  return a+VF;
}

_Cilk_shared double
foo_double(double a) {
  return a+VD;
}

_Cilk_shared long double
foo_longdouble(long double a) {
  return a+VLD;
}

int check(void) {
    int res = 0;
    if (sc != VSC+1) {
      res++;
    }
    if (si != VSI+1) {
      res++;
    }
    if (sl != VSL+1) {
      res++;
    }
    if (sll != VSLL+1) {
      res++;
    }
   if (f != VF+1) {
      res++;
    }
    if (d != VD+1) {
      res++;
    }
    if (ld != VLD+1) {
      res++;
    }
    return res;
}
//.........................................................................util
