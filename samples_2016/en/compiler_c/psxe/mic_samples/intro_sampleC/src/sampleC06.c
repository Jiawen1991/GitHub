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
#include <immintrin.h>

// Sample 06 ..................................................................
// This sample demonstrates using the __MIC__ predefined macro to 
// conditional compile target-specific code
//
// __MIC__ is only defined for the target compilation during compilation 
// of a source file containing language extensions for offload

void __attribute__((target(mic))) compute06(int n, float* a);

float __attribute__((target(mic))) data[16] = 
{ 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f,
  9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f };

void sample06()
{
    #pragma offload target(mic) optional
    compute06(16, data);
    
    if (fabs(data[0]-2.0f) <= 0.1f && fabs(data[15]-32.0f) < 0.1f)
        printf("PASS Sample06\n");
    else
        printf("*** FAIL Sample06\n");
}

void __attribute__((target(mic))) compute06 (int n, float* a) 
{

#ifdef __MIC__

  // This code block contains target specific code. The intrinsic
  // functions are only valid for the target
  //
  // The __MIC__ macro guards against host compilation of this
  // block because that macro is only defined for the target compilation

  int  i, tail, mask, nn;
  __m512 _val, _yy;

  _val = _mm512_setzero_ps();
  _yy  = _mm512_setzero_ps();

  nn   = 16 * (n / 16);
  tail = n % 16;
  mask = (1 << tail)  - 1;

  for (i = 0; i < nn; i+=16)
  {
      _val =  _mm512_loadunpacklo_ps (_val, (void*)(&a[i])  );
      _val =  _mm512_loadunpackhi_ps (_val, (void*)(&a[i+16]) );

      _yy  = _mm512_add_ps (_val, _val);

      _mm512_packstorelo_ps ((void*)(&a[i])   , _yy );
      _mm512_packstorehi_ps ((void*)(&a[i+16]), _yy );
  }
  if (tail != 0) 
  {
      _val =  _mm512_loadunpacklo_ps (_val, (void*)(&a[i])  );
      _val =  _mm512_loadunpackhi_ps (_val, (void*)(&a[i+16]) );

      _yy  = _mm512_add_ps (_val, _val);

      _mm512_packstorelo_ps ((void*)(&a[i])   , _yy );
      _mm512_packstorehi_ps ((void*)(&a[i+16]), _yy );
  }

#else

  // Generic code block compiled during the host compilation

  int i;

  for (i = 0; i < n; i++)
  {
      a[i] += a[i];
  }

#endif
}
//...........................................................................06
