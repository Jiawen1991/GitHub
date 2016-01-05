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
!#      Example Program Text from Sample shrd_sampleCPP
!#*****************************************************************************/

#include <stdio.h>

void shrd_ofld00();
void shrd_ofld01();
void shrd_ofld02();
void shrd_ofld03();
void shrd_ofld04();
void shrd_ofld05();
void shrd_ofld06();
void shrd_ofld07();
void shrd_ofld08();
void shrd_ofld09();
void shrd_ofld10();
void shrd_ofld11();

void shrd_ofld_vt01();
void shrd_ofld_vt02();
void shrd_ofld_vt03();

void shrd_ofld_link();

int main(int argc, char* argv[])
{
    // Start calls to test functions
    printf("Samples started\n");

    // Invoke Samples
 
    shrd_ofld00();  // Check for targets and offload mode
    shrd_ofld01();  // Shared class member field access
    shrd_ofld02();  // Shared class member field/functions access
    shrd_ofld03();  // Shared class member field of different types
    shrd_ofld04();  // passing pointer to class object to target function
    shrd_ofld05();  // Single inheritance of class
    shrd_ofld06();  // Multiple inheritance of class
    shrd_ofld07();  // Allocating Shared memory dynamically
    shrd_ofld08();  // Allocating aligned Shared memory dynamically
    shrd_ofld09();  // Use pragma push/pop to mark region of code shared
    shrd_ofld10();  // Passing address Shared class object as argument
    shrd_ofld11();  // Shared class static field member test

    shrd_ofld_vt01(); // Shared class with virtual function member
    shrd_ofld_vt02(); // Shared class with virtual function member single 
		      // inheritance
    shrd_ofld_vt03(); // Shared class with virtual function member multiple 
		      // inheritance
   
    shrd_ofld_link(); // Link list program using many of above features

    // End of calls to test functions
    printf("Samples complete\n");
}
//.......................................................................driver
