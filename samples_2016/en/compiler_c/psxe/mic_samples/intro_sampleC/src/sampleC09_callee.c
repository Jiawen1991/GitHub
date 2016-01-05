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

// Sample 09 ..................................................................
// Sample 09 callee function
//
//  Functions called in offload code must have __declspec(target(mic)) -or-
//  __attribute__((target(mic)))
//
//  Global variables must be declared with __declspec(target(mic)) -or-
//  __attribute__((target(mic))) to ensure an instance is created in
//  the target image

__attribute__((target(mic))) int myglob;

__attribute__((target(mic))) int add_glob09(int k);

__attribute__((target(mic))) int sample09_callee(int i, int j)
{
  int res;

  #pragma noinline
  res = add_glob09(i+j);
  return res;
}

__attribute__((target(mic))) int add_glob09(int k)
{
  return k+myglob++;
}
//...........................................................................09
