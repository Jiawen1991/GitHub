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

#ifndef _H_HASHMAP_H_
#define _H_HASHMAP_H_

#include <h_hash.h>
#include <h_container.h>

template< class Result, class HashFunction >
struct maphash {
    inline static U4 genKey( const Result& __x ) {
        return HashFunction::genKey( __x.getKey() );
    }
    inline static bool equals( const Result& _a_, const Result& _b_ ) {
        return HashFunction::equals( _a_.getKey(), _b_.getKey() );
    }
};


template< class Key, class Data >
class KeyData
{
public:
    Data  _data;  ///< The key
    Key   _key;   ///< The actual data
    KeyData( const Key& _key_ ) : _data(), _key( _key_ ) {}
    KeyData( const Key& _key_, const Data& _data_ ) : _data( _data_ ), _key( _key_ ) {}
    KeyData( const KeyData< Key, Data >& _org_ ) : _data( _org_._data ), _key( _org_._key ) {}
    KeyData( ) : _data( ), _key( ) {}
    void copy( const Data& _data_ ) { _data = _data_; }
    void release() {}
    const Key &getKey() const { return _key; }
    const Data &getData() const { return _data; }
    Data &getData() { return _data; }
    //    const KeyData< Key, Data >& operator=( const KeyData< Key, Data >& _org_ )
    //        { _key = _org_._key; _data = _org_._data; return *this; }
};


template< class Key, class Data >
class KeyArrayPointer
{
public:
    Data  *_data;  ///< The key
    Key    _key;   ///< The actual data-pointer
    KeyArrayPointer( const Key& _key_, Data * const & _data_ = NULL ) : _data( _data_ ), _key( _key_ ) {}
    KeyArrayPointer( ) : _data( NULL ), _key( ) {}
    void copy( Data * const & _data_ ) { _data = _data_; }
    void release() { if( _data ) { delete [] _data; _data = NULL; } }
    const Key &getKey() const { return _key; }
    Data * const &getData() const { return _data; }
    Data * &getData() { return _data; }
    const KeyArrayPointer< Key, Data > & operator=( const KeyArrayPointer< Key, Data > & _r_ ) {
        release(); _data = _r_._data; _key = _r_._key; return *this; }
};


template< class Key, class Data >
class KeyObjectPointer
{
public:
    Data  *_data;  ///< The key
    Key    _key;   ///< The actual data-pointer
    KeyObjectPointer( const Key& _key_, const Data& _data_ )  : _data( new Data( _data_ ) ), _key( _key_ ) {}
    KeyObjectPointer( const Key& _key_, Data * _data_ = NULL ) : _data( _data_ ), _key( _key_ ) {}
    KeyObjectPointer( const KeyObjectPointer< Key, Data >& _org_ )  : _data( _org_._data ), _key( _org_._key ) {}
    KeyObjectPointer( ) : _data( NULL ), _key( ) {}
    void copy( Data * const & _data_ ) { _data = new Data( *_data_ ); }
    void release() { if( _data ) { delete _data; _data = NULL; } }
    const Key &getKey() const { return _key; }
    Data * const &getData() const { return _data; }
    Data * &getData() { return _data; }
    const KeyArrayPointer< Key, Data > & operator=( const KeyArrayPointer< Key, Data > & _r_ ) {
        release(); _data = _r_._data; _key = _r_._key; return *this; }
};


/// Hash map, maps key -> data
/// @param Key      the type which is used as the key
/// @param Data     the type which is used as data
/// @param Result    the Result type, either KeyData or KeyXXXPointer
///           Constructors _must_ copy Data
/// @param HashFunction a type defining ::genKey, returning U4 for Hashing
///
/// Use ObjectAllocator<Result> here if you use object as Key or Data
/// NOTE! Default implementation uses ClearAllocator which uses memset(p, 0, ..) to clean-up allocated KeyData array
template< class Key, class Data, class Result = KeyData< Key, Data >, class HashFunction = VT_hash< Key >, class Allocator = ClearAllocator< Result > >
class HashMap : protected Hash< Result, maphash< Result, HashFunction >, Allocator >
{
public:
    /// This is the type to what iterators point
    typedef Result Result_t;
    typedef Hash< Result, maphash< Result, HashFunction >, Allocator > BaseHash;

    typedef typename BaseHash::Bucket Bucket;
    typedef typename BaseHash::BucketList BucketList;
    /// iterator to iterate through hash, no non-const iterator available
    typedef typename BaseHash::Iterator Iterator;
    typedef typename BaseHash::ConstIterator ConstIterator;

    /// Constructor creating _size_ many buckets
    HashMap() : BaseHash() {}
    /// Constructor creating _size_ many buckets
    HashMap( unsigned int _size_ ) : BaseHash( _size_ ) {}
    /// call release on each Result_t-Entry
    ~HashMap();
    /// to be used for ConstIterators
    ConstIterator end() const { return BaseHash::end(); }
    /// to be used for Iterators
    Iterator end() { return BaseHash::end(); }
    /// to be used for ConstIterators
    ConstIterator begin() const { return BaseHash::begin(); }
    /// to be used for Iterators
    Iterator begin() { return BaseHash::begin(); }
    /// return pointer to const data or NULL if not found
    const Data* find( const Key& _key_ ) const;
    /// return pointer to data or NULL if not found
    Data* find( const Key& _key_ );
    /// reserve slot for key with empty data section, possibly overwrite existing entry
    Data* insert( const Key& _key_ );
    /// insert key and data, possibly overwrite existing entry
    Data* insert( const Key& _key_, const Data& _data_ );
    /// reserve slot for key with empty data section if not yet there
    Data* insertOnce( const Key& _key_ );
    /// insert key and data if not yet there
    Data* insertOnce( const Key& _key_, const Data& _data_ );
    /// empties hash
    void erase();
    /// empties hash but keeps bucket memory
    void reset();
    /// returns number of entries in hashmap
    unsigned int count() const { return BaseHash::count(); }
    /// resizes bucket array to given size
    void resize( U4 _size_ ) { return BaseHash::resize( _size_ ); }
};

template< class Key, class Data, class Result, class HashFunction, class Allocator >
void HashMap< Key, Data, Result, HashFunction, Allocator >::reset()
{
    for( Iterator i = begin(); i != end(); ++i ) {
        (*i).release();
    }
    BaseHash::reset();
}

template< class Key, class Data, class Result, class HashFunction, class Allocator >
void HashMap< Key, Data, Result, HashFunction, Allocator >::erase()
{
    for( Iterator i = begin(); i != end(); ++i ) {
        (*i).release();
    }
    BaseHash::erase();
}

template< class Key, class Data, class Result, class HashFunction, class Allocator >
HashMap< Key, Data, Result, HashFunction, Allocator >::~HashMap()
{
    for( Iterator i = begin(); i != end(); ++i ) {
        (*i).release();
    }
}

template< class Key, class Data, class Result, class HashFunction, class Allocator >
const Data* HashMap< Key, Data, Result, HashFunction, Allocator >::find( const Key& _key_ ) const
{
    if( count() == 0 ) return NULL;
    Result_t tmp_res( _key_ );
    const Result_t * tmp_ret = BaseHash::find( tmp_res );
    return tmp_ret ? &tmp_ret->getData() : NULL;
}

template< class Key, class Data, class Result, class HashFunction, class Allocator >
Data* HashMap< Key, Data, Result, HashFunction, Allocator >::find( const Key& _key_ )
{
    if( count() == 0 ) return NULL;
    Result_t tmp_res( _key_ );
    Result_t * tmp_ret = BaseHash::find( tmp_res );
    return tmp_ret ? &tmp_ret->getData() : NULL;
}

template< class Key, class Data, class Result, class HashFunction, class Allocator >
Data* HashMap< Key, Data, Result, HashFunction, Allocator >::insert( const Key& _key_ )
{
    Result_t tmp_res( _key_ );
    Result_t * tmp_ret = BaseHash::insert( tmp_res );
    return tmp_ret ? &tmp_ret->getData() : NULL;
}

template< class Key, class Data, class Result, class HashFunction, class Allocator >
Data* HashMap< Key, Data, Result, HashFunction, Allocator >::insert( const Key& _key_, const Data& _data_ )
{
    Result_t tmp_res( _key_, _data_ );
    Result_t * tmp_ret = BaseHash::insert( tmp_res );
    return tmp_ret ? &tmp_ret->getData() : NULL;
}

template< class Key, class Data, class Result, class HashFunction, class Allocator >
Data* HashMap< Key, Data, Result, HashFunction, Allocator >::insertOnce( const Key& _key_ )
{
    Result_t tmp_res( _key_ );
    Result_t * tmp_ret = BaseHash::insertOnce( tmp_res );
    return tmp_ret ? &tmp_ret->getData() : NULL;
}

template< class Key, class Data, class Result, class HashFunction, class Allocator >
Data* HashMap< Key, Data, Result, HashFunction, Allocator >::insertOnce( const Key& _key_, const Data& _data_ )
{
    Result_t tmp_res( _key_, _data_ );
    Result_t * tmp_ret = BaseHash::insertOnce( tmp_res );
    return tmp_ret ? &tmp_ret->getData() : NULL;
}

#endif
