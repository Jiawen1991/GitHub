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

// shrd_ofld 13 ................................................................
// This sample uses Cilk Spawn and _Cilk_offload_to() to offload a 
// function to multiple devices
//
// This sample requires a target be available. When no target
// is available, execution does not default to the CPU and the
// sample reports a failure.
//

// Shared variable declaration for test shrd_ofld13
_Cilk_shared float pi_multi[2];

// Shared function declaration for test shrd_ofld13
_Cilk_shared compute_pi_multi(int count, int card_no)
{
    int i;

    for (i=0; i<count; i++)
    {
        float t = (float)((i+0.5f)/count);
        pi_multi[card_no] += 4.0f/(1.0f+t*t);
    }
}

void 
shrd_ofld13(void) {
    int count = 10000;

    // Check if target device(s) are installed and available
    #ifdef __INTEL_OFFLOAD
    int num_devices = _Offload_number_of_devices();
    #else
    int num_devices = 0;
    #endif

    if (num_devices == 0) {
        printf("*** FAIL shrd_ofld13 - target unavailable\n");
        return;
    }

    //  Initialize shared global variables
    int fail = 0;
    pi_multi[0] = 0.0f;
    pi_multi[1] = 0.0f;

    //  Compute pi on targets
    _Cilk_spawn _Cilk_offload_to(0) compute_pi_multi(count,0);
    if (num_devices > 1)
        _Cilk_spawn _Cilk_offload_to(1) compute_pi_multi(count,1);

    _Cilk_sync;

    //  Check results
    pi_multi[0] /= count;
    if (fabs(pi_multi[0] -3.14f) > 0.01f)
        fail = 1;

    if (num_devices > 1) {
        pi_multi[1] /= count;
        if (fabs(pi_multi[1] -3.14f) > 0.01f)
	    fail = 1;
    }

    if (fail == 0)
        printf("PASS shrd_ofld13\n");
    else
        printf("*** FAIL shrd_ofld13\n");
}
//...........................................................................13
