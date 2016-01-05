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
#include <math.h>

// shrd_ofld 01 ................................................................
// This sample uses a shared global variable and function on the target

// Shared variable declaration for test shrd_ofld01
_Cilk_shared float pi;

// Shared function declaration for test shrd_ofld01
_Cilk_shared compute_pi(int count)
{
    int i;

    for (i=0; i<count; i++)
    {
        float t = (float)((i+0.5f)/count);
        pi += 4.0f/(1.0f+t*t);
    }
}

void 
shrd_ofld01(void) {
    int count = 10000;
    //  Initialize shared global variables
    pi = 0.0f;

    //  Compute pi on target
    _Cilk_offload compute_pi(count);

    pi /= count;
    if (fabs(pi-3.14f) <= 0.01f)
        printf("PASS shrd_ofld01\n");
    else
        printf("*** FAIL shrd_ofld01\n");
}
//...........................................................................01
