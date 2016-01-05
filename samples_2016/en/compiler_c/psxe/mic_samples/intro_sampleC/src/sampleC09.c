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

// Sample 09 ..................................................................
// This sample demonstrates using global data in functions called from
// an offload region
//
// When offloaded code calls other functions two things require attention:
//
// 1. The called functions must have __declspec(target(mic)) -or-
//    __attribute__((target(mic))) to ensure functions are included
//    in the target image
//
// 2. Global variables referenced in the called functions must be named
//    in in/out clauses to ensure their values are copied between the CPU
//    and target
//
// Global variables must be declared with __declspec(target(mic)) -or-
// __attribute__((target(mic))) to ensure an instance is created in
// the target image
//
// In this sample the offloaded code calls sample09_callee, which
// references the global variable "myglob". The variable "myglob" must
// be explicitly listed in an in/out clause to ensure its value is
// matained across the CPU and target

extern __attribute__((target(mic))) int myglob;
__attribute__((target(mic))) int sample09_callee(int i, int j);

void sample09()
{
    int result;
    myglob = 55;

#if 1
    #pragma offload target(mic) optional inout(myglob)
#else
    // Enable this pragma to see the effect of not using the 
    // explicit inout clause
    #pragma offload target(mic) optional
#endif
    result = sample09_callee(4, 5);

    if (result == 64)
        printf("PASS Sample09\n");
    else
        printf("*** FAIL Sample09\n");
}
//...........................................................................09
