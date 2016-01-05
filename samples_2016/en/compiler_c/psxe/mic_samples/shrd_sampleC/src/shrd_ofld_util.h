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

#include <stdlib.h>
#include <stdio.h>

// util header files..........................................................

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

_Cilk_shared char foo_char(char a);

_Cilk_shared int foo_int(int a);

_Cilk_shared long foo_long(long a);

_Cilk_shared long long foo_longlong(long long a);

_Cilk_shared float foo_float(float a);

_Cilk_shared double foo_double(double a);

_Cilk_shared long double foo_longdouble(long double a);

int check(void);
//.........................................................................util
