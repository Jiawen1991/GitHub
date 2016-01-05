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
!#      Offload Demonstration 
!#      Getting Started tutorial
!#*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>

   char Tname[] = "C/C++ Tutorial: Offload Demonstration";

   // Global variables used within offloaded code under the data 
   // marshalling method require declaration with the target(mic) 
   // attribute to ensure a copy appears in the offloaded image

   // Variables may be decorated as a group using the push/pop method
   
      #pragma offload_attribute(push, target(mic))
         int * all_Vals;
         int * E_vals;
         int numEs, numOs;
      #pragma offload_attribute(pop)

   // Or indvidually using either a __declspec or __attribute__

      __declspec (target(mic)) int *O_vals;
      __attribute__((target(mic))) int MAXSZ = 26;


   // Global variables used within offloaded code under the data sharing
   // method require declaration with the _Cilk_shared keyword to ensure
   // a copy appears in the offloaded image

   // Variables may be decorated as a group using the push/pop method
   
      #pragma offload_attribute(push, _Cilk_shared)
         int MAXSZ_shrd = 26;
         int numPs;
      #pragma offload_attribute(pop)

   // Or indvidually using _Cilk_shared

      int * _Cilk_shared all_Vals_shrd;
      int * _Cilk_shared P_vals;

   // NOTE: A variable may *not* be decorated with both the target(mic)
   //       attribute and _Cilk_shared keyword.


   // target_id designates the target CPU. -1 allows offload
   // execution on a target selected by the offload run-time library

   int target_id = -1;


// Subprograms referenced within offloaded code under the data marshalling 
// method must be declared with the target(mic) attribute to ensure a copy 
// appears in the offloaded image

__attribute__((target(mic))) void gather_odds()
{
  int k;

  // Gather odd numbered values into O_vals and count in numOs

  numOs = 0;
  for (k = 0; k < MAXSZ; k++) {
     if ( all_Vals[k]%2 != 0 )  {
        O_vals[numOs] = all_Vals[k];
        numOs++;
     }
  }
}


// Subprograms referenced from within offloaded code under the data 
// sharing method must be declared with the _Cilk_shared keyword to 
// ensure a copy appears in the offloaded image

_Cilk_shared void gather_primes()
{
  int k, l, sqrt_tmp;
  int is_prime;
   
  // Gather prime numbered values into P_vals and count in numPs

  numPs = 0;
  for (k = 0; k < MAXSZ_shrd; k++) {
     is_prime = 1;

     if ( all_Vals_shrd[k] > 1) {
        sqrt_tmp = sqrt ( all_Vals_shrd[k] );

        for (l = 2; l <= sqrt_tmp; l++) {
           if ( all_Vals_shrd[k] % l == 0 ) is_prime = 0;
        }

        if ( is_prime ) {
           P_vals[numPs] = all_Vals_shrd[k];
           numPs++;
        }
     }
  }
}


int main( )

  //  C/C++ Tutorial
  //    A basic demonstration of the Language Extension for Offload (LEO)

{
  int k;
  int num_devices = 0;


  printf("\n  %s\n\n",Tname);

  // MAXSZ must be evenly divisible
  if ( MAXSZ % 2 != 0 ) {
     printf("    *** FAIL - value for MAXSZ not usable, must be even value\n\n");
     return(1);
  }

  // Check for and report number of target devices installed when offload 
  // compilation enabled

  // When no target device is available, target_id = -1 forces a run-time
  // error

  // __INTEL_OFFLOAD is predefined only when offload compilation is enabled

   #ifdef __INTEL_OFFLOAD
      printf("  Checking for Intel(R) Xeon Phi(TM) (Target CPU) devices...\n\n");
      num_devices = _Offload_number_of_devices();
      printf("    Number of Target devices installed: %d\n\n",num_devices);
   #endif


  // allocate all_Vals

  all_Vals = (int *) _mm_malloc(MAXSZ * sizeof(int),64);


  // Initialize all_Vals 

  // Demonstrate offloading an OpenMP parallel construct using the
  // offload pragma
  //
  // Limit OpenMP construct to two threads for demonstration purposes
  //
  // Offload IN/OUT/INOUT clauses:
  //    Transfer all_Vals new values "OUT" only

  #pragma offload target(mic : target_id) mandatory \
                                          out(all_Vals : length(MAXSZ))
  #pragma omp parallel for num_threads(2)
      for (k = 0; k < MAXSZ; k++) {
         all_Vals[k] = k + 1;
      }


  // allocate E_vals

  E_vals = (int *) _mm_malloc( (MAXSZ/2) * sizeof(int),64);


  // Gather even numbered values into E_vals and count in numEs

  // Demonstrate offloading a code block using the offload pragma
  //
  // Offload IN/OUT/INOUT clauses:
  //    Transfer all_Vals values "IN" only
  //    Transfer numEs initial value (on host) in and the final value
  //    (from target) out with "INOUT"
  //    Transfer E_vals new values "OUT" only

  numEs = 0;
  #pragma offload target(mic : target_id) mandatory \
                               in(all_Vals : length(MAXSZ)) \
                               inout(numEs) out(E_vals : length(MAXSZ/2))
  for (k=0; k < MAXSZ; k++) {
      if ( all_Vals[k] % 2 == 0 ) {
         E_vals[numEs] = all_Vals[k];
         numEs++;
      }
  }


  // allocate O_vals

  O_vals = (int *) _mm_malloc( (MAXSZ/2) * sizeof(int),64);

  // Gather odd numbered values into O_vals and count in numOs
  //
  // Demonstrate offloading a subprogram reference using the offload pragma

  //  The use of global variables all_Vals, O_vals, and numOs in 
  //  gather_odds() is outside the compiler's lexical scope; therefore, 
  //  to ensure the integrity of their values between the host and target
  //  CPUs before and after execution of the subprogram, explicit 
  //  IN/OUT/INOUT clauses are required to ensure their values are 
  //  transferred between the host and target CPUs accordingly.

  // Offload IN/OUT/INOUT clauses:
  //    Transfer all_Vals values "IN" only
  //    Transfer O_vals and numOs new values "OUT" only

  #pragma offload target(mic : target_id) mandatory \
                               in(all_Vals : length(MAXSZ)) \
                               out(numOs) out(O_vals : length(MAXSZ/2))
     gather_odds();


  // allocate P_vals

  // Shared method requires allocating variables within shared memory

  // __INTEL_OFFLOAD is predefined only when offload compilation is enabled

  #ifdef __INTEL_OFFLOAD
       // Allocate in shared memory (when offload compilation enabled)

       P_vals = (_Cilk_shared int *) _Offload_shared_aligned_malloc( \
                                     (MAXSZ/2) * sizeof(int),64);

       all_Vals_shrd = (_Cilk_shared int *) _Offload_shared_aligned_malloc( \
                                            MAXSZ * sizeof(int),64);
  #else
       // Allocate on host only (when compiling with -no-offload) 

       P_vals = (int *) _mm_malloc( (MAXSZ/2) * sizeof(int),64);

       all_Vals_shrd = (int *) _mm_malloc(MAXSZ * sizeof(int),64);
  #endif

  // Initialize all_Vals_shrd by copying all_Vals values into the 
  // shared memory array 

  all_Vals_shrd[0:MAXSZ] = all_Vals[0:MAXSZ];

  // Gather prime number values into P_vals and count in numPs

  // Demonstrate offloading a subprogram reference using the shared method
  // _Cilk_offload_to keyword to a specific target

  // The global variables P_vals and numPs used in gather_primes() are 
  // shared between the host and target CPUs via the shared memory area 
  // (as per their declaration with the _Cilk_shared keyword), and their 
  // values remain current between both CPUs automatically.

  _Cilk_offload_to (target_id) gather_primes();

  // Display results
   
  printf("  Unsorted original values...first twenty (20) values:\n");
  printf("    Evens and Odds:\n");
  printf("           ");

  for (k=0; k < ((MAXSZ <= 20) ? MAXSZ : 20) ; k++) {
      printf(" %4d", all_Vals[k]);
      if ( k == 9) printf("\n           ");
  }
  printf("\n\n");

  printf("  Sorted results...first ten (10) values each:\n");

  printf("    Evens: ");
  for (k=0; k < ((numEs < 10) ? numEs : 10) ; k++) {
      printf(" %4d", E_vals[k]);
  }
  printf("\n");

  printf("    Odds : ");
  for (k=0; k < ((numOs < 10) ? numOs : 10); k++) {
    printf(" %4d", O_vals[k]);
  }
  printf("\n");

  printf("   Primes: ");
  for (k=0; k < ((numPs < 10) ? numPs : 10); k++) {
    printf(" %4d", P_vals[k]);
  }
  printf("\n\n");


  // Deallocate

  _mm_free(O_vals);
  _mm_free(E_vals);
  _mm_free(all_Vals);

  // __INTEL_OFFLOAD is predefined only when offload compilation is enabled

  #ifdef __INTEL_OFFLOAD
      _Offload_shared_aligned_free(P_vals);
      _Offload_shared_aligned_free(all_Vals_shrd);
  #else
      _mm_free(P_vals);
      _mm_free(all_Vals_shrd);
  #endif

}
