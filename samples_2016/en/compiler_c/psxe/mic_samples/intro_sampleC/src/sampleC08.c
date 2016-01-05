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
#include <math.h>

// For heterogeneous features include offload.h
#include <offload.h>

// Sample 08 ..................................................................
// This sample demonstrates how #pragma offload placed in front of 
// an OpenMP construct enables OpenMP on the target
//
// Effectively, this is heterogeneous OpenMP

void sample08()
{
    float pi = 0.0f;
    int count = 10000;
    int i;

    #pragma offload target (mic) optional
    #pragma omp parallel for reduction(+:pi)
    for (i=0; i<count; i++)
    {
        float t = (float)((i+0.5f)/count);
        pi += 4.0f/(1.0f+t*t);
    }
    pi /= count;


    if (fabs(pi-3.14f) <= 0.01f)
#ifdef DEBUG
        printf("PASS Sample08 Pi = %f\n", pi);
    else
        printf("*** FAIL Sample08 Pi = %f\n", pi);
#else
        printf("PASS Sample08\n");
    else
        printf("*** FAIL Sample08\n");
#endif
}
//...........................................................................08
