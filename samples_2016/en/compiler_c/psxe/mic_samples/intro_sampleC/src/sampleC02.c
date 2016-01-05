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

// Sample 02 ..................................................................
// This sample uses scalars and named arrays
//
// The entire offload region is visible to the compiler since there
// are no function calls; therefore, the referenced arrays do not need to 
// be listed in in/out/inout clauses.
//
// By default the values of all variables referenced in the offload region
// are copied from the CPU to the target before the offload region executes
// and from target to CPU after the offload region executes.

typedef double T;

#define SIZE 1000

// Variables may be decorated as a group using the push/pop method

#pragma offload_attribute(push, target(mic))
static T in1_02[SIZE];
static T in2_02[SIZE];
static T res_02[SIZE];
#pragma offload_attribute(pop)

static void populate_02(T* a, int s);

void sample02()
{
    int i;
    populate_02(in1_02, SIZE);
    populate_02(in2_02, SIZE);

    #pragma offload target(mic) optional
    {
    for (i=0; i<SIZE; i++)
	{
	    res_02[i] = in1_02[i] + in2_02[i];
	}
    }

    if (res_02[0] == 0 && res_02[SIZE-1] == 2*(SIZE-1))
        printf("PASS Sample02\n");
    else
        printf("*** FAIL Sample02\n");
}

static void populate_02(T* a, int s)
{
    int i;
	
    for (i=0; i<s; i++)
	{
	      a[i] = i;
	}
}
//...........................................................................02
