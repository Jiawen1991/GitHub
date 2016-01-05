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
#include <offload.h>

// shrd_ofld 12 ................................................................
// This sample demonstrates using mutliple cards with _Cilk_offload_to
//
// This sample requires a target be available. When no target
// is available, execution does not default to the CPU and the
// sample reports a failure.
//

// Shared variable declaration for test shrd_ofld12
_Cilk_shared int num_devs;  

// Shared function declaration for test shrd_ofld12
int _Cilk_shared device(int i)  
{
    int device_num;

    #ifdef __INTEL_OFFLOAD
    device_num = _Offload_get_device_number();
    #else
    device_num = -999;
    #endif

    if (device_num != (i % num_devs)) {
       return 1;
    }
    else {
       return 0;
    }
}

void 
shrd_ofld12(void) {
    int device_num, i;
    int count;
    int res = 0;
    int tres;

    // Check if target device(s) are installed and available
    #ifdef __INTEL_OFFLOAD
    num_devs = _Offload_number_of_devices();
    #else
    num_devs = 0;
    #endif

    if (num_devs == 0) {
        printf("*** FAIL shrd_ofld12 - target unavailable\n");
        return;
    }

    count = 10;
    for (i = 0; i < count; i++) {
       // Test for valid device number on each device
       tres = _Cilk_offload_to(i) device(i);
       res += tres;
    }
    
    if (res == 0)
       printf("PASS shrd_ofld12\n");
    else
       printf("*** FAIL shrd_ofld12\n");
}
//...........................................................................12
