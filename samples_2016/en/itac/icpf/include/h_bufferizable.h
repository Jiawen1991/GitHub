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

#ifndef _H_BUFFERIZABLE_H_
#define _H_BUFFERIZABLE_H_

#include <h_config.h>

/** Serializable/bufferizable Object
    All buffers are/can be assumed to be aligned to 8 byte boundary
    alignment at end of buffer must not be included **/
class Bufferizable
{
 public:
    /// buffer to small
    enum { SMALL_BUFFER = -1 };
    /// returns number of bytes needed to bufferize object
    /// alignment at end of buffer must not be included
    virtual U4 bufferizedSize() const = 0;
    /// puts object into buffer and returns number of bytes used
    /// @param _buffer_ the buffer into which the object is to be stored
    /// @param _buffSize_ number of bytes available in buffer
    /// @return number of bytes stored or SMALL_BUFFER, alignment at end of buffer must not be included
    virtual U4 bufferize( unsigned char * _buffer_, U4 _buffSize_ ) const = 0;
    /// inverse of bufferize(): read buffer to build object and return number of bytes read
    virtual U4 debufferize( const unsigned char * _buffer_, U4 _buffSize_ ) = 0;
    /// same as debufferize(), but can use memory of buffer until detatch is called
    /// can be destroyed prior to call of detatch (don't free mem!)
    virtual U4 attach( const unsigned char * _buffer_, U4 _buffSize_ ) = 0;
    /// object is still needed, but buffer can no longer be used
    /// is invoked only after attach() has been called
    virtual void detach() = 0;

    virtual ~Bufferizable(){}
};

inline U4 align( U4 _s1_, U4 _s2_ = 8 )
{
    return ( ( _s1_ + _s2_ - 1 ) / _s2_ ) * _s2_;
}

#endif
