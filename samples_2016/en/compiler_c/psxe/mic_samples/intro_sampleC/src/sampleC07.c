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

// Sample 07 ..................................................................
// This sample demonstrates use of the __MIC__ predefined macro and data
// persistence.
//
// File-scope and function-local variables with storage class "static"
// are statically allocated.
//
// Static data will retain values across offloads as long as they are
// not over-written with new values.
//
// The macro __MIC__ can be used to determine target availability and
// whether a computation actually occurred on the target.
//
// When one computation on the target leaves some data behind for
// a subsequent computation to reuse, then you must ensure the
// first computation was offloaded
//
// This sample requires a target be available and that the same target
// is used. Use of the same target is required to guarantee the integrity
// of persistent data.
//
// This sample requires a target be available. When no target is
// available, the sample reports a failure and exits.

#define SIZE 1000

__attribute__((target(mic))) int array1[SIZE];
__attribute__((target(mic))) int send_array(int* p, int s);
__attribute__((target(mic))) void compute07(int* out, int siz);

void sample07()
{
    int in_data[SIZE], out_data[SIZE];
    int array_sent = 0;
    int num_devices;
    int i;

    // Check if target device(s) are installed and available
    #ifdef __INTEL_OFFLOAD
      num_devices = _Offload_number_of_devices();
    #else
      num_devices = 0;
    #endif

    if (num_devices == 0) {
        printf("*** FAIL Sample07 - target unavailable\n");
        return;
    }

    for (i=0; i< SIZE; i++)
    {
        in_data[i] = i + 1;
        out_data[i] = 0;
    }

    // Place static data on the target
    //
    // While offload is optional below, initialization on the CPU will
    // be detected in the setting of array_sent. Therefore, initialization
    // on the CPU will not yield the offload of compute07 which
    // would lead to incorrect results if run on the target

    #pragma offload target(mic : 0) optional
    array_sent = send_array(in_data, SIZE);

    // The if clause determines whether the construct is offloaded
    // It is checked ahead of enforcing the optional clause

    #pragma offload target(mic : 0) optional if(array_sent) out(out_data)
    compute07(out_data, SIZE);

    if (out_data[0] == 2 && out_data[SIZE-1] == (SIZE*2))
       printf("PASS Sample07\n");
    else
       printf("*** FAIL Sample07\n");
}

__attribute__((target(mic))) int send_array(int* p, int s)
{
    int retval;
    int i;

    for (i=0; i<s; i++)
    {
        array1[i] = p[i];
    }

#ifdef __MIC__
    retval = 1;
#else
    retval = 0;
#endif

    // Return 1 if array initialization was done on target
    return retval;
}

__attribute__((target(mic))) void compute07(int* out, int siz)
{
    int i;

    for (i=0; i<siz; i++)
    {
        out[i] = array1[i]*2;
    }
}
//...........................................................................07
