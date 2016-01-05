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

// Sample 14 ..................................................................
// This sample demonstrates using the alloc and into modifiers and 
// status clause
//
// The into modifier requires a target device be installed and available
// The test fails without a target device or when built with -offload=none

// Size must be >= 16 and evenly divisible by 4
#define SIZE 100
#define QTR SIZE/4
#define HALF SIZE/2
#define ThreeQTR SIZE*3/4

#pragma offload_attribute(push, target(mic))
static int inp_14[SIZE];
static int micp_14[SIZE];
static int outp_14[SIZE];

// Global offload status
_Offload_status offstat_14;
#pragma offload_attribute(pop)


void sample14()
{
   int   i;
   int   res;

   // Initialize input array
   for (i = 0; i < SIZE; i++)  {
       inp_14[i] = i + 10;
   }

   // The sample demonstrates use of the into and alloc modifiers

   // Given the following array sections
   //
   //           -----------------------
   // inp_14    | A  |  B  |  C  |  D  |
   //           -----------------------
   //
   //           -----------------------
   // micp_14   | E  |  F  |  G  |  H  |
   //           -----------------------
   //
   //           -----------------------
   // outp_14   | I  |  J  |  K  |  L  |
   //           -----------------------
   //
   // The offload pragma and associated code accomplish the following:
   //
   // 1. Allocates HALF (sections F & G ) of variable micp_14 on the Target CPU
   //
   // 2. Transfers one QUARTER of data (section A) of variable inp_14 from
   //    the host CPU INTO one QUARTER of the allocated HALF of variable 
   //    micp_14 (section G) on the Target CPU
   //
   // 3. Updates one QUARTER of data from variable micp_14 (section G) and 
   //    stores results into micp_14 (section F) on the Target CPU.
   //
   // 4. Transfers one QUARTER of data (section F) of variable micp_14 from
   //    the Target CPU INTO one QUARTER of the variable outp_14 (section L) 
   //    on the Host CPU
  
   // The into modifier requires a target device be installed and available
   //  
   // Use of into with the optional clause also requires using the status 
   // clause

   // Initialize the offload status
   OFFLOAD_STATUS_INIT(offstat_14);
   
   #pragma offload target(mic) optional status(offstat_14)              \
                               in( inp_14[0 : QTR] :                    \
                                   alloc( micp_14[ QTR : HALF ] )       \
                                   into( micp_14[ HALF : QTR ] ) )      \
                               out( micp_14[ QTR : QTR ] :              \
                                    into( outp_14[ ThreeQTR : QTR ]) ) 
   {  

      // Update one QUARTER of data from variable micp_14 (section G) 
      // and store results into micp_14 (section F) on the Target CPU

      for (i = QTR; i < HALF; i++) {
          micp_14[i] = micp_14[i + QTR] + 100;
      }
   }

   //  Verify results

   //  Non-zero offload status will trigger failure
   res = offstat_14.result;

   for (i = ThreeQTR; i < SIZE; i++) {
       res += (inp_14[i - ThreeQTR] != (outp_14[i] - 100) );
   } 

   printf("%s\n", ( res == 0 ) ? "PASS Sample14" : \
         ((offstat_14.result == OFFLOAD_UNAVAILABLE) ? \
         "*** FAIL Sample14 - target unavailable" : "*** FAIL Sample14"));

   return;
}
