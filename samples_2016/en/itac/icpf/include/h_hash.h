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

#ifndef _H_HASH_H_
#define _H_HASH_H_

#include <h_config.h>
#include <h_vector.h>
#include <h_container.h>

template< class _Key > struct VT_hash {};

/// Hash map, maps key -> data
/// @param Key      the type which is used as the key
/// @param Data     the type which is used as data
/// @param DataHasher a type defining ::genKey, returning U4 for Hashing, and ::equals
template< class Data, class DataHasher = VT_hash< Data >, class Allocator = ClearAllocator< Data > >
class Hash
{
public:
    /// This is the type to what iterators point
    typedef Vector< Data, Allocator > Bucket;
    typedef Vector< Bucket, ClearAllocator< Bucket > > BucketList;
    /// iterator to iterate through hash, no non-const iterator available
    class Iterator {
    private:
        const Hash< Data, DataHasher, Allocator >* _hash;
        typename BucketList::Iterator _i;
        typename Bucket::Iterator _j;
    public:
        Iterator( const Hash< Data, DataHasher, Allocator >& _hash_ ) 
            : _hash( &_hash_ ) {;}
        Iterator( const Hash< Data, DataHasher, Allocator >& _hash_, const typename BucketList::Iterator& _i_ )
            : _hash( &_hash_ ), _i( _i_ ) {}
        Iterator( const Hash< Data, DataHasher, Allocator >& _hash_, const typename BucketList::Iterator& _i_, const typename Bucket::Iterator& _j_ )
            : _hash( &_hash_ ), _i( _i_ ), _j( _j_ ) {;}
        /// Allow to declare variables of this type:
        Iterator() {_hash = 0;}
        Iterator& operator++();
        bool operator==( const Iterator& _it_ ) const;
        bool operator!=( const Iterator& _it_ ) const { return !operator==( _it_ ); }
        Data& operator*() { return (*_j); }
        void stabilize();
    };
    class ConstIterator
    {
    private:
        const Hash< Data, DataHasher, Allocator >* _hash;
        typename BucketList::ConstIterator _i;
        typename Bucket::ConstIterator _j;
    public:
        ConstIterator() {}
        ConstIterator( const Hash< Data, DataHasher, Allocator >& _hash_ ) 
            : _hash( &_hash_ ) {}
        ConstIterator( const Hash< Data, DataHasher, Allocator >& _hash_, const typename BucketList::ConstIterator& _i_ )
            : _hash( &_hash_ ), _i( _i_ ) {}
        ConstIterator( const Hash< Data, DataHasher, Allocator >& _hash_, const typename BucketList::ConstIterator& _i_, const typename Bucket::ConstIterator& _j_ )
            : _hash( &_hash_ ), _i( _i_ ), _j( _j_ ) {}
        ConstIterator& operator++();
        bool operator==( const ConstIterator& _it_ ) const;
        bool operator!=( const ConstIterator& _it_ ) const { return !operator==( _it_ ); }
        const Data& operator*() { return (*_j); }
        void stabilize();
    };
    friend class Iterator;
    friend class ConstIterator;
private:
    BucketList _buckets;
    unsigned int _count;
    /// returns pointer to data or NULL if not found, accesses only key-part of _key_
    Data* findIt( const Data& _key_ ) const;
public:
    /// Constructor creating _size_ many buckets
    Hash() : _buckets( 0 ), _count( 0 ) { }
    /// Constructor creating _size_ many buckets
    Hash( unsigned int _size_ ) : _buckets( _size_ ), _count( 0 ) { _buckets.resize( _size_ ); }
    /// to be used for ConstIterators
    ConstIterator end() const { return ConstIterator( *this, ((const BucketList&)_buckets).end() ); }
    /// to be used for Iterators
    Iterator end() { return Iterator( *this, _buckets.end() ); }
    /// to be used for ConstIterators
    ConstIterator begin() const;
    /// to be used for Iterators
    Iterator begin();
    /// return pointer to const data or NULL if not found, accesses only key-part of _key_
    const Data* find( const Data& _key_ ) const { return findIt( _key_ ); }
    /// return pointer to data or NULL if not found
    Data* find( const Data& _key_ ) { return findIt( _key_ ); }
    /// reserve slot for key with empty data section, possibly overwrite existing entry
    Data* insert( const Data& _key_ );
    /// reserve slot for key with empty data section if not yet there
    Data* insertOnce( const Data& _key_ );
    /// delete entry, given pointer must have been retrieved through same hash
    /// iterators on the same element become invalid!
    /// handle with care
    Data* remove( Data * _entry_ );
    /// empties hash
    void erase();
    /// empties hash but keeps bucket memory
    void reset();
    /// go back to initial state, without doing anything with its items
    void release();
    /// returns number of entries in hash
    unsigned int count() const { return _count; }
    /// current capacity of bucket, might be greater than count()
    U4 capacity() { return _buckets.count(); }
    /// resizes bucket array to given size
    void resize( U4 _size_ ) { ASSERT( _count == 0 ); _buckets.resize( _size_ ); }
    /// output hash to a stream
    template< class D, class DH, class A > friend ostream& operator<<( ostream& _os_, const Hash< D, DH, A >& _hash_ );
    /// input hash from a stream
    template< class D, class DH, class A > friend istream& operator>>( istream& _is_, Hash< D, DH, A >& _hash_ );
};

extern U4 getHashPrime( U4 _n_ );

template< class Data, class DataHasher, class Allocator >
typename Hash< Data, DataHasher, Allocator >::Iterator&
Hash< Data, DataHasher, Allocator >::Iterator::operator++()
{
    if( ++_j == (*_i).end() ) {
        typename BucketList::ConstIterator hmo_e = _hash->_buckets.end();
        while( ++_i != hmo_e && (*_i).count() == 0 );
        if( _i != hmo_e ) _j = (*_i).begin();
    }
    return *this;
}

template< class Data, class DataHasher, class Allocator >
bool Hash< Data, DataHasher, Allocator >::Iterator::operator==( const Iterator& _it_ ) const
{
    return ( ( _it_._i == _hash->_buckets.end() && _i == _hash->_buckets.end() ) 
             ? true 
             : _it_._i == _i && _it_._j == _j );
}

template< class Data, class DataHasher, class Allocator >
void Hash< Data, DataHasher, Allocator >::Iterator::stabilize()
{
    if( _i == _hash->_buckets.end() ) return;
    if( _j == (*_i).end() ) {
        --_j; // dirty dirty, we know how it works :(
        operator++();
    }
}

template< class Data, class DataHasher, class Allocator >
typename Hash< Data, DataHasher, Allocator >::ConstIterator&
Hash< Data, DataHasher, Allocator >::ConstIterator::operator++()
{
    if( ++_j == (*_i).end() ) {
        typename BucketList::ConstIterator hmoc_e = _hash->_buckets.end();
        while( ++_i != hmoc_e && (*_i).count() == 0 );
        if( _i != hmoc_e ) _j = (*_i).begin();
    }
    return *this;
}

template< class Data, class DataHasher, class Allocator >
bool Hash< Data, DataHasher, Allocator >::ConstIterator::operator==( const ConstIterator& _it_ ) const
{
    return ( ( _it_._i == _hash->_buckets.end() && _i == _hash->_buckets.end() ) 
             ? true 
             : _it_._i == _i && _it_._j == _j );
}

template< class Data, class DataHasher, class Allocator >
typename Hash< Data, DataHasher, Allocator >::ConstIterator
Hash< Data, DataHasher, Allocator >::begin() const
{
    const typename BucketList::ConstIterator hmbc_e = _buckets.end();
    for( typename BucketList::ConstIterator hmbc_i = _buckets.begin(); hmbc_i != hmbc_e; ++hmbc_i ) {
        if( (*hmbc_i).count() ) return ConstIterator( *this, hmbc_i, (*hmbc_i).begin() );
    }
    return end();
}

template< class Data, class DataHasher, class Allocator >
void Hash< Data, DataHasher, Allocator >::ConstIterator::stabilize()
{
    if( _i == _hash->_buckets.end() ) return;
    if( _j == (*_i).end() ) {
        --_j; // dirty dirty, we know how it works :(
        operator++();
    }
}

template< class Data, class DataHasher, class Allocator >
typename Hash< Data, DataHasher, Allocator >::Iterator
Hash< Data, DataHasher, Allocator >::begin()
{
    typename BucketList::Iterator hmb_e = _buckets.end();
    for(typename BucketList::Iterator i = _buckets.begin(); i != hmb_e; ++i ) {
        if( (*i).count() ) return Iterator( *this, i, (*i).begin() );
    }
    return end();
}

template< class Data, class DataHasher, class Allocator >
Data* Hash< Data, DataHasher, Allocator >::findIt( const Data& _key_ ) const
{
    if( _count == 0 ) return NULL;
    U4 key = DataHasher::genKey( _key_ ) % _buckets.count();
    const Bucket& bucket = _buckets[key];
    typename Bucket::ConstIterator e = bucket.end();
    for( typename Bucket::ConstIterator i = bucket.begin(); i != e; ++i ) { 
        if( DataHasher::equals( *i, _key_ ) ) return (Data*)&(*i);
    }
    return NULL;
} 

template< class Data, class DataHasher, class Allocator >
Data* Hash< Data, DataHasher, Allocator >::insert( const Data& _key_ )
{
    if( _buckets.count() == 0 ) {
        _buckets.resize( 31 );
    }
    U4 key = DataHasher::genKey( _key_ ) % _buckets.count();
    Bucket& _hmi_bucket = _buckets[key];
    typename Bucket::Iterator _hmi_e = _hmi_bucket.end();
    for( typename Bucket::Iterator _hmi_i = _hmi_bucket.begin(); _hmi_i != _hmi_e; ++_hmi_i ) { 
        if( DataHasher::equals( *_hmi_i, _key_ ) ) {
            *_hmi_i = _key_;
            return &(*_hmi_i);
        }
    }
    ++_count;
    return _hmi_bucket.pushBack( _key_ );
}

template< class Data, class DataHasher, class Allocator >
Data* Hash< Data, DataHasher, Allocator >::insertOnce( const Data& _key_ )
{
    if( _buckets.count() == 0 ) {
        _buckets.resize( 31 );
    }
    U4 key = DataHasher::genKey( _key_ ) % _buckets.count();
    Bucket& _hmio_bucket = _buckets[key];
    typename Bucket::Iterator _hmio_e = _hmio_bucket.end();
    for( typename Bucket::Iterator _hmio_i = _hmio_bucket.begin(); _hmio_i != _hmio_e; ++_hmio_i ) { 
        if( DataHasher::equals( *_hmio_i, _key_ ) ) return &(*_hmio_i);
    }
    ++_count;
    return _hmio_bucket.pushBack( _key_ );
}

template< class Data, class DataHasher, class Allocator >
Data * Hash< Data, DataHasher, Allocator >::remove( Data * _entry_ )
{
    U4 key = DataHasher::genKey( *_entry_ ) % _buckets.count();
    Bucket& bucket = _buckets[key];
    // DANGER DANGER DANGER
    // now we are getting seriously dirty
    // but it's fast anyway ;-)
    // hopefully the compiler tells us if Vector stuff changes in an incompatible way
    --_count;
    return bucket.popAt( _entry_ ); // design win!
}

template< class Data, class DataHasher, class Allocator >
void Hash< Data, DataHasher, Allocator >::erase()
{
    for(typename BucketList::Iterator j = _buckets.begin(); j != _buckets.end(); ++j ) {
        (*j).resize( 0 );
    }
    _count = 0;
}

template< class Data, class DataHasher, class Allocator >
void Hash< Data, DataHasher, Allocator >::reset()
{
    for( typename BucketList::Iterator j = _buckets.begin(); j != _buckets.end(); ++j ) {
        (*j).erase();
    }
    _count = 0;
}

template< class Data, class DataHasher, class Allocator >
void Hash< Data, DataHasher, Allocator >::release()
{
    for( typename BucketList::Iterator i = _buckets.begin(); i !=  _buckets.end(); ++i ) {
        (*i).release();
    }
    _count = 0;
}


template< class Data, class DataHasher, class Allocator >
ostream& operator<<( ostream& _os_, const Hash< Data, DataHasher, Allocator >& _hash_ )
{
    _os_.write( ( const char * ) &( _hash_._count ), sizeof( _hash_._count ) );
    for( typename Hash< Data, DataHasher, Allocator >::ConstIterator it = _hash_.begin(); it != _hash_.end(); ++it ) _os_ << *it;
    return _os_;
}

template< class Data, class DataHasher, class Allocator >
istream& operator>>( istream& _is_, Hash< Data, DataHasher, Allocator >& _hash_ )
{
    unsigned int count;
    if( _is_.read( ( char *) &count, sizeof( count ) ) == NULL ) return _is_;
    for( unsigned int i=0; i < count; ++i ) {
        Data data;
        if( _is_ >> data == NULL ) {
            _hash_.erase();
            return _is_;
        }
        _hash_.insert( data );
    }
    return _is_;
}

template<> struct VT_hash< I1 > {
    inline static U4 genKey(I1 _x_) { return static_cast<U4>(_x_); }
    inline static bool equals( const I1& _a_, const I1& _b_ ) { return _a_ == _b_; }
};

template<> struct VT_hash< U1 > {
    inline static U4 genKey(U1 _x_) { return _x_; }
    inline static bool equals( const U1& _a_, const U1& _b_ ) { return _a_ == _b_; }
};

template<> struct VT_hash< I4 > {
    inline static U4 genKey(I4 _x_) { return static_cast<U4>(_x_); }
    inline static bool equals( const I4& _a_, const I4& _b_ ) { return _a_ == _b_; }
};

template<> struct VT_hash< U4 > {
    inline static U4 genKey(U4 _x_) { return _x_; }
    inline static bool equals( const U4& _a_, const U4& _b_ ) { return _a_ == _b_; }
};

template<> struct VT_hash< I8 > {
    inline static U4 genKey(I8 _x_) { return static_cast<U4>(_x_); }
    inline static bool equals( const I8& _a_, const I8& _b_ ) { return _a_ == _b_; }
};

template<> struct VT_hash< U8 > {
    inline static U4 genKey(U8 _x_) { return static_cast<U4>(_x_); }
    inline static bool equals( const U8& _a_, const U8& _b_ ) { return _a_ == _b_; }
};

template<> struct VT_hash< void* > {
    inline static U4 genKey(void* _x_) { return (*reinterpret_cast<U4*>(&_x_) & 0xffff ); }

    inline static bool equals( const void*& _a_, const void*& _b_ ) { return _a_ == _b_; }
};

template<> struct VT_hash< const void* > {
    inline static U4 genKey( const void * _x_ ) { return (*reinterpret_cast<U4*>(&_x_) & 0xffff); }

    inline static bool equals( const void * const & _a_, const void * const & _b_ ) { return _a_ == _b_; }
};

#endif
