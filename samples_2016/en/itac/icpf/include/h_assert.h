/*********************************************************************
 * Copyright 2003-2014 Intel Corporation.
 * All Rights Reserved.
 * 
 * The source code contained or described herein and all documents
 * related to the source code ("Material") are owned by Intel
 * Corporation or its suppliers or licensors. Title to the Material
 * remains with Intel Corporation or its suppliers and licensors. The
 * Material contains trade secrets and proprietary and confidential
 * information of Intel or its suppliers and licensors. The Material
 * is protected by worldwide copyright and trade secret laws and
 * treaty provisions. No part of the Material may be used, copied,
 * reproduced, modified, published, uploaded, posted, transmitted,
 * distributed, or disclosed in any way without Intel's prior express
 * written permission.
 * 
 * No license under any patent, copyright, trade secret or other
 * intellectual property right is granted to or conferred upon you by
 * disclosure or delivery of the Materials, either expressly, by
 * implication, inducement, estoppel or otherwise. Any license under
 * such intellectual property rights must be express and approved by
 * Intel in writing.
 * 
 *********************************************************************/

#ifndef _H_ASSERT_H_
#define _H_ASSERT_H_

#include <iostream>
using namespace std;

#ifdef ASSERT
#  undef ASSERT
#endif

# define SEGFAULT *((volatile char *)0) = 0


# define ALWAYS_ASSERT( _x ) \
  (void) \
  ( ( !(_x) ) ? \
    ( cerr << "Assertion " << #_x << " failed in " << __FILE__ << " line " << __LINE__ << endl, \
      SEGFAULT ) : \
    0 \
  )

# define IMPOSSIBLE \
  (void) \
  ( cerr << "Something impossible happened in " << __FILE__ << " line " << __LINE__ << endl, \
      SEGFAULT )

#ifdef NOASSERT
# define ASSERT( _x )
#else
# define ASSERT( _x )  ALWAYS_ASSERT( _x )
#endif

#endif
