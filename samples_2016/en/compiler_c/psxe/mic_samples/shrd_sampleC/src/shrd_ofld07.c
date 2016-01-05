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
#include <stdlib.h>

// shrd_ofld 07 ................................................................
// This sample uses Cilk_for for offloading

#define DIM 400
_Cilk_shared int on_host;

_Cilk_shared int in_arr[DIM];
_Cilk_shared int out_arr[DIM];
_Cilk_shared int scal_arr[DIM];
_Cilk_shared int add_arr[DIM];

int check_arr[DIM];

void init07(int *arr, int (*f)(int)) {
  int i;
  _Cilk_for (i = 0; i < DIM; i++) {
      arr[i] = f(i);
  }
}

int f1(int i) {
  return i;
}

int f2(int i) {
  return i / 25;
}

int f3(int i) {
  return (-i) + 10;
}

int f4(int i) {
  return (i*(i/25)-i+10);
}

void init_em_all()  {
  init07(&in_arr[0], f1);
  init07(&scal_arr[0], f2);
  init07(&add_arr[0], f3);
  init07(&check_arr[0], f4);
}

static
int check() {
  int i;
  int res = 0;
  for (i = 0; i < DIM; i++) {
    if (out_arr[i] != check_arr[i]) {
      res++;
      printf("Difference for %d: %d != %d \n", i, out_arr[i], check_arr[i]);
    }
  }
  return res;
}


void calc(int lo, int hi) {
  int i;
  _Cilk_offload _Cilk_for (i = lo; i < hi; i++) {
#ifdef __MIC__
  on_host = 0;
#endif
    out_arr[i] = (in_arr[i] * scal_arr[i]) + add_arr[i];
  }
}

void
shrd_ofld07(void) {
  int res;
  on_host = 1;

  init_em_all();

  calc(0, DIM);

  res = check();

  if ((res == 0) && (on_host == 0))
    printf("PASS shrd_ofld07\n");
  else
    printf("*** FAIL shrd_ofld07\n");
}
//...........................................................................07
