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
#include <stdlib.h>

// Sample 10 ..................................................................
// This sample demonstrates using bit-wise copyable structs in offloaded
// code
//
// Bit-wise copyable structs or arrays of such structs are copied
// between the CPU and target like arrays of basic types

typedef struct
{
    float m1;
    float m2;
} bwcopyable;

void sample10()
{
    bwcopyable *in_data;
    bwcopyable *out_data;
    int elements = 100;
    int i;

    in_data = (bwcopyable*)malloc(sizeof(bwcopyable)*elements);
    out_data = (bwcopyable*)malloc(sizeof(bwcopyable)*elements);
    for (i=0; i<elements; i++)
    {
        in_data[i].m1 = i;
    }

    #pragma offload target(mic) optional  \
        in(in_data : length(elements))    \
        out(out_data : length(elements))
    {
        for (i=0; i<elements; i++)
        {
            out_data[i].m2 = in_data[i].m1;
        }
    }

    if (out_data[elements-1].m2 == in_data[elements-1].m1)
        printf("PASS Sample10\n");
    else
        printf("*** FAIL Sample10\n");

    free(in_data);
    free(out_data);
}
//...........................................................................10
