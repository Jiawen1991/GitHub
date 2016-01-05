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
!#      Example Program Text from Sample intro_sampleC
!#*****************************************************************************/

#include <stdio.h>
#include <offload.h>

// Sample 12 ..................................................................
// This sample demonstrates using mutliple target devices when 
// available
//
// This sample requires a target be available. When no target is
// available, the sample reports a failure and exits.

void sample12()
{
    int num_devices;
    int device_num, i; 
    int count; 
    int res = 0;
    int tres;

    // Check if target device(s) are installed and available
    #ifdef __INTEL_OFFLOAD
       num_devices = _Offload_number_of_devices();
    #else
       num_devices = 0;
    #endif

    if (num_devices == 0) {
        printf("*** FAIL Sample12 - target unavailable\n");
        return;
    }

    count = 10;
    for (i = 0; i < count; i++) {
    #pragma offload target(mic: i) optional in(i)
        {
            #ifdef __INTEL_OFFLOAD
               device_num = _Offload_get_device_number();
            #else
               device_num = -999;
            #endif

            if (device_num != (i % num_devices)) {
		tres = 1;
            }
            else {
		tres = 0;
            }
        }
        res += tres;
    }

    if (res == 0)
        printf("PASS Sample12\n");
    else
        printf("*** FAIL Sample12\n");

    return;
}
//...........................................................................12
