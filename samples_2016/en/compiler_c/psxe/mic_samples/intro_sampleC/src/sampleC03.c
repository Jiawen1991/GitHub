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

// Sample 03 ..................................................................
// This sample demonstrates how to avoid unnecessary copying of data values
// between the CPU and target for variables used in an offload region
//
// Input arrays used in the offload region are named in an "in" clause to
// ensure their values are only copied from the CPU to target before the
// offload region executes and not back to the CPU after the offload region
// executes
//
// The result array is named in an "out" clause because its initial values
// are not needed on the target. Only the final values calculated on the
// target need to copied back to the CPU after the offload region executes

typedef double T;

#define SIZE 1000

// Variables may be decorated as a group using the push/pop method

#pragma offload_attribute(push, target(mic))
static T in1_03[SIZE];
static T in2_03[SIZE];
static T res_03[SIZE];
#pragma offload_attribute(pop)

static void populate_03(T* a, int s);

void sample03()
{
    int i;

    populate_03(in1_03, SIZE);
    populate_03(in2_03, SIZE);

    #pragma offload target(mic) optional in(in1_03, in2_03) out(res_03)
    {
	for (i=0; i<SIZE; i++)
	{
	    res_03[i] = in1_03[i] + in2_03[i];
	}
    }

    if (res_03[0] == 0 && res_03[SIZE-1] == 2*(SIZE-1))
        printf("PASS Sample03\n");
    else
        printf("*** FAIL Sample03\n");
}

static void populate_03(T* a, int s)
{
    int i;

    for (i=0; i<s; i++)
    {
        a[i] = i;
    }
}
//...........................................................................03
