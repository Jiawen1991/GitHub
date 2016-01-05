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

#ifndef _H_CONTAINER_H_
#define _H_CONTAINER_H_

template< class T >
class Container
{
public:
    /// get first element in container
    /// to nest lopps, increase nesting level _level_ for each loop
    /// @param _level_  nesting level for loop, default: 0
    /// @return const pointer to first element, 0 if empty
    virtual const T* first( int _level_ = 0 ) const = 0;
    /// get next element in container for loops started with a call to first 
    /// to nest lopps, increase nesting level _level_ for each loop
    /// @param _level_  nesting level for loop, corresponds to _level_ passed to first, default: 0
    /// @return const pointer to next element, 0 if no further element
    virtual const T* next( int _level_ = 0 ) const = 0;
    ///
    /// @return the number of elements in the container
    virtual unsigned int count( void ) const = 0;
    virtual ~Container() {}
};

#endif
