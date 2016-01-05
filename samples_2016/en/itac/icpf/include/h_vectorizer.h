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

#ifndef _H_VECTORIZER_H_
#define _H_VECTORIZER_H_

#include <h_config.h>
#include <h_bufferizable.h>

/// Bufferizer are helper classes to implement Bufferizable Vectors
/// This specialisation provides non recursive functions for bufferizing an array
/// e.g. for standard data types or class without pointers
template< class T >
class DefaultBufferizer
{
public:
    /// return bytes needed to bufferize array (also see Bufferizable)
    inline static U4 bufferizedSize( const T* _vec_, unsigned int _num_ ) {
        return _num_ * sizeof( *_vec_ );
    }

    /// bufferize array (also see Bufferizable)
    inline static U4 bufferize( const T* _vec_, unsigned int _num_, unsigned char* _buff_, U4 _buffSize_ ) {
        if( bufferizedSize( _vec_, _num_ ) > _buffSize_ ) return (U4)Bufferizable::SMALL_BUFFER;
        memcpy( _buff_, _vec_, bufferizedSize( _vec_, _num_ ) );
        return bufferizedSize( _vec_, _num_ );
    }

    /// debufferize array (also see Bufferizable)
    inline static U4 debufferize( T*& _vec_, unsigned int _num_, const unsigned char* _buff_, U4 _buffSize_ ) {
        if( _buffSize_ < _num_ * sizeof( *_vec_ ) ) return (U4)Bufferizable::SMALL_BUFFER;
        memcpy( _vec_, _buff_, _num_ * sizeof( *_vec_ ) );
        return _num_ * sizeof( *_vec_ );
    }

    /// attach array (also see Bufferizable)
    inline static U4 attach( T*& _vec_, unsigned int _num_, const unsigned char* _buff_, U4 _buffSize_ ) {
        if( _buffSize_ < _num_ * sizeof( *_vec_ ) ) return (U4)Bufferizable::SMALL_BUFFER;
        _vec_ = (T*)_buff_;
        return _num_ * sizeof( *_vec_ );
    }

    /// return whether the array is to be freed, even if attach() was called
    inline static bool freeBuffer() {
        return false;
    }
};

/// This specialisation provides recursive functions for bufferizing an array of classes of Bufferizables
template< class T >
class ObjectBufferizer
{
public:
    inline static U4 bufferizedSize( const T* _vec_, unsigned int _num_ ) {
        U4 _rb_sz = 0;
        for( unsigned int _rb = 0; _rb < _num_; ++_rb ) {
            _rb_sz += _vec_[_rb].bufferizedSize();
            if( _rb != _num_ - 1 ) {
                _rb_sz = align( _rb_sz );
            }
        }
        return _rb_sz;
    }

    inline static U4 bufferize( const T* _vec_, unsigned int _num_, unsigned char* _buff_, U4 _buffSize_ ) {
        U4 _rb_sz = 0;
        for( unsigned int _rb = 0; _rb < _num_; ++_rb ) {
            if( _buffSize_ < _rb_sz ) return (U4)Bufferizable::SMALL_BUFFER;
            U4 _rb_tmp = _vec_[_rb].bufferize( _buff_ + _rb_sz, _buffSize_ - _rb_sz );
            if( _rb_tmp == (U4)Bufferizable::SMALL_BUFFER ) return (U4)Bufferizable::SMALL_BUFFER;
            _rb_sz += _rb_tmp;
            if( _rb != _num_ - 1 ) {
                _rb_sz = align( _rb_sz );
            }
        }
        return _rb_sz;
    }

    inline static U4 debufferize( T*& _vec_, unsigned int _num_, const unsigned char* _buff_, U4 _buffSize_ ) {
        U4 _rb_sz = 0;
        for( unsigned int _rb = 0; _rb < _num_; ++_rb ) {
            if( _buffSize_ < _rb_sz ) return (U4)Bufferizable::SMALL_BUFFER;
            U4 _rb_tmp = _vec_[_rb].debufferize( _buff_ + _rb_sz, _buffSize_ - _rb_sz );
            if( _rb_tmp == (U4)Bufferizable::SMALL_BUFFER ) return (U4)Bufferizable::SMALL_BUFFER;
            _rb_sz += _rb_tmp;
            if( _rb != _num_ - 1 ) {
                _rb_sz = align( _rb_sz );
            }
        }
        return _rb_sz;
    }

    inline static U4 attach( T*& _vec_, unsigned int _num_, const unsigned char* _buff_, U4 _buffSize_ ) {
        U4 _rb_sz = 0;
        _vec_ = new T[_num_];
        for( unsigned int _rb = 0; _rb < _num_; ++_rb ) {
            if( _buffSize_ < _rb_sz ) return (U4)Bufferizable::SMALL_BUFFER;
            U4 _rb_tmp = _vec_[_rb].attach( _buff_ + _rb_sz, _buffSize_ - _rb_sz );
            if( _rb_tmp == (U4)Bufferizable::SMALL_BUFFER ) return (U4)Bufferizable::SMALL_BUFFER;
            _rb_sz += _rb_tmp;
            if( _rb != _num_ - 1 ) {
                _rb_sz = align( _rb_sz );
            }
        }
        return _rb_sz;        
    }

    inline static bool freeBuffer() {
        return true;
    }
};

/// This specialisation provides recursive functions for bufferizing an array of strings
class StringBufferizer
{
    /// TODO (the following (debufferize and detach) are not used/presented):
    /// 1. debufferize may result in memory leak due to absence of the corresponding delete. BVector doesn't do it.
    /// 2. For now detach is never called. After BVector calls its detach all string values must be copied too.
public:
    inline static U4 bufferizedSize( const char* const * _vec_, unsigned int _num_ ) {
        U4 _rb_sz = 0;
        for( unsigned int _rb = 0; _rb < _num_; ++_rb ) {
            _rb_sz += strlen(_vec_[_rb]) + sizeof(U4) + 1;
            if( _rb != _num_ - 1 ) {
                _rb_sz = align( _rb_sz );
            }
        }
        return _rb_sz;
    }

    inline static U4 bufferize( const char* const * _vec_, unsigned int _num_, unsigned char* _buff_, U4 _buffSize_ ) {
        U4 _rb_sz = 0;
        for( unsigned int _rb = 0; _rb < _num_; ++_rb ) {
            //if( _buffSize_ < _rb_sz ) return (U4)Bufferizable::SMALL_BUFFER;
            U4 _rb_tmp = strlen( _vec_[_rb] ) + 1;
            if( _buffSize_ < _rb_sz + _rb_tmp + sizeof(U4) ) return (U4)Bufferizable::SMALL_BUFFER;
            memcpy( _buff_ + _rb_sz, &_rb_tmp, sizeof(U4) );
            memcpy( _buff_ + _rb_sz + sizeof(U4), _vec_[_rb], _rb_tmp ); //_vec_[_rb].bufferize( _buff_ + _rb_sz, _buffSize_ - _rb_sz );
            //if( _rb_tmp == (U4)Bufferizable::SMALL_BUFFER ) return (U4)Bufferizable::SMALL_BUFFER;
            _rb_sz += _rb_tmp + sizeof(U4);
            if( _rb != _num_ - 1 ) {
                _rb_sz = align( _rb_sz );
            }
        }
        return _rb_sz;
    }

    inline static U4 debufferize( const char **& _vec_, unsigned int _num_, const unsigned char* _buff_, U4 _buffSize_ ) {
        U4 _rb_sz = 0;
        for( unsigned int _rb = 0; _rb < _num_; ++_rb ) {
            //if( _buffSize_ < _rb_sz ) return (U4)Bufferizable::SMALL_BUFFER;
            U4 _rb_tmp;
            memcpy( &_rb_tmp, _buff_ + _rb_sz, sizeof(U4) );
            if( _buffSize_ < _rb_sz + _rb_tmp + sizeof(U4) ) return (U4)Bufferizable::SMALL_BUFFER;
            char *tmp_str = new char[_rb_tmp + 1];
            //_vec_[_rb] = new char[_rb_tmp + 1];
            memset( tmp_str, 0, _rb_tmp + 1 );
            memcpy( tmp_str, _buff_ + _rb_sz + sizeof(U4), _rb_tmp );
            _vec_[_rb] = tmp_str;
            //U4 _rb_tmp = _vec_[_rb].debufferize( _buff_ + _rb_sz, _buffSize_ - _rb_sz );
            //if( _rb_tmp == (U4)Bufferizable::SMALL_BUFFER ) return (U4)Bufferizable::SMALL_BUFFER;
            _rb_sz += _rb_tmp + sizeof(U4);
            if( _rb != _num_ - 1 ) {
                _rb_sz = align( _rb_sz );
            }
        }
        return _rb_sz;
    }

    inline static U4 attach( const char **& _vec_, unsigned int _num_, const unsigned char* _buff_, U4 _buffSize_ ) {
        U4 _rb_sz = 0;
        _vec_ = new const char*[_num_];
        for( unsigned int _rb = 0; _rb < _num_; ++_rb ) {
            //if( _buffSize_ < _rb_sz ) return (U4)Bufferizable::SMALL_BUFFER;
            U4 _rb_tmp; // = _vec_[_rb].attach( _buff_ + _rb_sz, _buffSize_ - _rb_sz );
            memcpy( &_rb_tmp, _buff_ + _rb_sz, sizeof(U4) );
            if( _buffSize_ < _rb_sz + _rb_tmp + sizeof(U4) ) return (U4)Bufferizable::SMALL_BUFFER;
            _vec_[_rb] = ( const char *)(_buff_ + _rb_sz + sizeof(U4));
            //if( _rb_tmp == (U4)Bufferizable::SMALL_BUFFER ) return (U4)Bufferizable::SMALL_BUFFER;
            _rb_sz += _rb_tmp + sizeof(U4);
            if( _rb != _num_ - 1 ) {
                _rb_sz = align( _rb_sz );
            }
        }
        return _rb_sz;        
    }

    inline static bool freeBuffer() {
        return true;
    }
};

/// Not implemented
template< class T >
class NotImplBufferizer
{
public:
    inline static U4 bufferizedSize( const T* /*_vec_*/, unsigned int /*_num_*/ ) { ASSERT( 0 ); return 0; }
    inline static U4 bufferize( const T* /*_vec_*/, unsigned int /*_num_*/, unsigned char* /*_buff_*/, U4 /*_buffSize_*/ ) { ASSERT( 0 ); return 0; }
    inline static U4 debufferize( T*& /*_vec_*/, unsigned int /*_num_*/, const unsigned char* /*_buff_*/, U4 /*_buffSize_*/ ) { ASSERT( 0 ); return 0; }
    inline static U4 attach( T*& /*_vec_*/, unsigned int /*_num_*/, const unsigned char* /*_buff_*/, U4 /*_buffSize_*/ ) { ASSERT( 0 ); return 0; }
    inline static bool freeBuffer() { return true; }
};


// TODO We might need an Allocator, which assumes certain methods on the objects in the vector,
// such as resetting the data before beeing overwritten with memcpy (DefaultConstructors might allocate memory!)

/// default allocator for allocating, deallocating and moving memory
template< class T >
class DefaultAllocator {
public:
    inline static void alloc( T*& _vec_, unsigned int _num_ ) { _vec_ = new T[_num_]; }
    inline static void dealloc( T*& _vec_, unsigned int, unsigned int ) { if( _vec_ ) { delete [] _vec_; }; _vec_ = NULL; }
    inline static void moveTo( T* _vec_, const T* _org_, unsigned int _num_ ) { memcpy( _vec_, _org_, _num_ * sizeof( T ) ); }
    inline static void copy( T* _dest_, const T& _org_ ) { *_dest_ = _org_; }
    inline static void output( ostream& _os_, const T* _vec_, unsigned int _num_ ) { _os_.write( ( const char *) &_num_, sizeof( _num_ ) ); for( unsigned int i = 0; i < _num_; ++i ) _os_ << _vec_[i]; }
    inline static void input( istream& _is_, T*& _vec_, unsigned int& _sz_, unsigned int& _num_ ) { dealloc( _vec_, _sz_, _num_ ); if( _is_.read( ( char *) &_num_, sizeof( _num_ ) ) == NULL ) return; if( _sz_ = _num_ ) _vec_ = new T[_num_]; for( unsigned int i = 0; i < _num_; ++i ) if( _is_ >> _vec_[i] == NULL ) break; }
    template< class U > struct rebind { typedef DefaultAllocator< U > other; };
};

/// allocator for objects of 'virtual' classes (with vtbl) providing ::release (e.g. to release (not delete) pointers)
/// if no vtbl, might consider using ClearAllocator instead
template< class T >
class ObjectReleaseAllocator : public DefaultAllocator< T >
{
public:
    inline static void moveTo( T* _vec_, T* _org_, unsigned int _num_ ) {
        memcpy( _vec_, _org_, _num_ * sizeof( T ) );
        for( unsigned int _pva = 0; _pva < _num_; ++_pva ) _org_[_pva].release();
    }
    template< class U > struct rebind { typedef ObjectReleaseAllocator< U > other; };
};

/// you might want to set allocated memory to 0 -> use this allocator
/// dealloc also calls delete on all entries
template< class T >
class ClearAllocator : public DefaultAllocator< T > {
public:
    inline static void alloc( T*& _vec_, unsigned int _num_ ) { _vec_ = new T[_num_]; memset( _vec_, 0, _num_ * sizeof( T ) ); }
    inline static void dealloc( T*& _vec_, unsigned int, unsigned int ) { if( _vec_ ) { delete [] _vec_; _vec_ = NULL; } }
    inline static void moveTo( T* _vec_, T* _org_, unsigned int _num_ ) { memcpy( _vec_, _org_, _num_ * sizeof( T ) ); memset( _org_, 0, _num_ * sizeof( T ) ); }
    template< class U > struct rebind { typedef ClearAllocator< U > other; };
};


/// sophisticated objects cannot be memcpy'd => use operator=
/// less efficient!
template< class T >
class ObjectAllocator : public DefaultAllocator< T > {
public:
    inline static void moveTo( T* _vec_, T* _org_, unsigned int _num_ ) { 
        for( unsigned int i = 0; i < _num_; i++ ) {
            _vec_[ i ] = _org_[i];
        }
    }
    template< class U > struct rebind { typedef ObjectAllocator< U > other; };
};

/// for pointers to objects you might want to set allocated memory to 0 -> use this allocator
/// dealloc also calls delete on all entries
template< class T >
class ObjectPointerAllocator : public ClearAllocator< T > {
public:
    inline static void dealloc( T*& _vec_, unsigned int _num_, unsigned int ) {
        if( _vec_ ) {
            for( unsigned int _pva = 0; _pva < _num_; ++_pva ) if( _vec_[_pva] != NULL ) { delete _vec_[_pva]; }
            delete [] _vec_;
            _vec_ = NULL;
        }
    }
    template< class U > struct rebind { typedef ObjectPointerAllocator< U > other; };
};

/// for pointers to objects of classes derived from RefCounter
/// dealloc also calls delete on all entries
template< class T >
class RCObjectPointerAllocator : public ObjectPointerAllocator< T > {
public:
    inline static void dealloc( T*& _vec_, unsigned int _num_, unsigned int ) {
        if( _vec_ ) {
            for( unsigned int _pva = 0; _pva < _num_; ++_pva ) if( _vec_[_pva] && _vec_[_pva]->unref() < 0 ) { delete _vec_[_pva]; }
            delete [] _vec_;
            _vec_ = NULL;
        }
    }
    inline static void copy( T* _dest_, T& _org_ ) {
        _org_->ref();
        *_dest_ = _org_;
    }
    template< class U > struct rebind { typedef RCObjectPointerAllocator< U > other; };
};

/// for pointers to arrays you might want to set allocated memory to 0 -> use this allocator
/// dealloc also calls delete [] on all entries
template< class T >
class ArrayPointerAllocator : public ObjectPointerAllocator< T > {
public:
    inline static void dealloc( T*& _vec_, unsigned int _num_, unsigned int ) {
        if( _vec_ ) {
            for( unsigned int _pva = 0; _pva < _num_; ++_pva ) {
                if( _vec_[_pva] ) {
                    delete [] _vec_[_pva];
                }
            }
            delete [] _vec_;
            _vec_ = NULL;
        }
    }
    template< class U > struct rebind { typedef ArrayPointerAllocator< U > other; };
};

#endif

