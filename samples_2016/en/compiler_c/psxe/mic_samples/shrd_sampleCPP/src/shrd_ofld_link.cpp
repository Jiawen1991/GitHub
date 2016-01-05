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
#include <new>

// shrd_ofld_link ..............................................................
// This sample tests assignment to shared class field both inside a 
// constructor executed on the host and a function executed on the target

#define INITSTART  5 

// Shared class variable declaration
class
_Cilk_shared
SHRD_OFLD_LINK{
public:
   int data;
   SHRD_OFLD_LINK *next;
};


// Shared class object pointing to head of linked list
SHRD_OFLD_LINK *_Cilk_shared head;

// Target computation routine
_Cilk_shared
void caculate()
{
   SHRD_OFLD_LINK  *listnode;

   listnode = head;

   while(listnode != 0) {
     listnode->data += 10;
     listnode = listnode->next;
   }

   return;
}

void create() {

   SHRD_OFLD_LINK  *listnode;
   int       i = 0;


   head  = new( _Offload_shared_aligned_malloc(sizeof(SHRD_OFLD_LINK), 8)) _Cilk_shared SHRD_OFLD_LINK;
   listnode = head;
   while (i < 3) {
      listnode->next  = new( _Offload_shared_aligned_malloc(sizeof(SHRD_OFLD_LINK), 8)) _Cilk_shared SHRD_OFLD_LINK;
      listnode = listnode->next;
      i++;
   }
   listnode->next  = 0;
}

void initialize() {

   SHRD_OFLD_LINK  *listnode;
   int       i;

   listnode = head;

   i = INITSTART;
   while(listnode != 0) {
     listnode->data = i++;
     listnode = listnode->next;
   }
}

int validate(int val) {

   int       fail; 
   SHRD_OFLD_LINK  *listnode;
   int       i;

   i = INITSTART;

   listnode = head;
   fail  = 0;
   while(listnode != 0) {
     if (listnode->data != (i+val)) 
        fail = 1;
     i++;
     listnode = listnode->next;
   }

   return fail;
}

void shrd_ofld_link()
{
  int res;

  create();

  initialize();

  res = validate(0);

  _Cilk_offload caculate();

  res += validate(10);

  if (res != 0)
      printf("*** FAIL shrd_ofld_link\n");
  else
      printf("PASS shrd_ofld_link\n");
}
//.......................................................................link
