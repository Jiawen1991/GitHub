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

// shrd_ofld 02 ................................................................
// This sample uses shared arrays and shared function
//
// The arrays are allocated in shared memory therefore 
// they do not need to be passed explicitly
//
// The compiler generates the necessary code to send and 
// receive data implicitly between host and target

typedef double T;

#define SIZE 1000

// Shared variable declaration for test shrd_ofld02
_Cilk_shared T in1[SIZE];
_Cilk_shared T in2[SIZE];
_Cilk_shared T res[SIZE];

// Shared function declaration for test shrd_ofld02
_Cilk_shared void compute_sum();

static void populate(T* a, int s);

void
shrd_ofld02(void) {
    int i;

    populate(in1, SIZE);
    populate(in2, SIZE);

    // Call compute sum on Target
    _Cilk_offload compute_sum();

    if (res[0] == 0 && res[SIZE-1] == 2*(SIZE-1))
        printf("PASS shrd_ofld02\n");
    else
        printf("*** FAIL shrd_ofld02\n");
}

static void populate(T* a, int s)
{
    int i;

    for (i=0; i<s; i++) {
        a[i] = i;
    }
}

_Cilk_shared void compute_sum()
{
    int i;
    for (i=0; i<SIZE; i++) {
        res[i] = in1[i] + in2[i];
    }
}
//...........................................................................02
