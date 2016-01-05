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

#ifndef _H_VECTOR_H_
#define _H_VECTOR_H_

#include <h_assert.h>
#include <string.h>
#include <stdlib.h>
#include <h_bufferizable.h>
#include <h_vectorizer.h>

/// Template for arrays/vectors which are readable and writeable
/// @param T         type of which array/vector is requested
/// @param Allocator class providing functions for de/allocation
template< class T, class Allocator = DefaultAllocator< T > >
class Vector
{
public:
    typedef const T* ConstIterator;
    typedef       T* Iterator;

    ///
    /// @param _size_ reserve enough memory for @a _size_ elements, doesn't resize(), only reserve()
    Vector( int _size_ = 0 );
    /// copies Vector
    Vector( const Vector< T, Allocator >& _org_ ) :_data( 0 ), _size( 0 ), _end( 0 ) { *this = _org_; }
    /// copies buffer
    Vector( const T* _array_, U4 _n_ );
    ~Vector();

    /// returns false if at least one element is stored in it
    bool empty() const { return this->_end == 0; }
    /// returns number of elements stored in it
    U4 count() const { return this->_end; }
    /// current capacity of vector, might be greater than count()
    U4 capacity() { return this->_size; }
    /// copies vector
    const Vector< T, Allocator >& operator=( const Vector< T, Allocator >& _org_ );
    bool operator==( const Vector< T, Allocator >& _org_ ) const;

    /// returns const reference to element at position @a _i_, no bounds checks
    const T& operator[]( int _i_ ) const { return this->_data[_i_]; }
    /// returns ConstIterator pointing to begining of vector
    ConstIterator begin() const { return this->_data; }
    /// returns ConstIterator representing the end of the vector, cannot be dereferenced
    ConstIterator end() const { return this->_data + this->_end; }
    /// returns const element at end of vector
    const T& back() const { return this->_data[this->_end - 1]; }

    /// reserve memory for at least @a _size_ many elements, never decreases current size
    void reserve( U4 _size_ );

    /// resize vector to @a _size_

    /// If count() currently smaller, adds default elements. If bigger, truncates elements.
    void resize( U4 _size_ );
    /// sets current count, max. to its size
    void setCount( U4 _cnt_ );
    /// returns reference to element at position @a _i_, no bounds checks
    T& operator[]( int _i_ ) { return this->_data[_i_]; }
    /// returns Iterator pointing to begining of vector
    Iterator begin() { return this->_data; }
    /// returns Iterator representing the end of the vector, cannot be dereferenced
    Iterator end() { return this->_data + this->_end; }
    /// adds given element at end of vector, increments its size
    T* pushBack( const T& _elem_ ) { ASSERT( this->_size >= this->_end ); return pushBack_( _elem_ ); }
    /// searches for data element and returns pointer to it or if not found, pushBack()
    T* findOrInsert( const T* _elem_ );
    /// insert element at position _pos_
    T* insertAt( const T* _elem_, U4 _pos_ );
    /// insert vector at position _pos_
    T* insertAt( const Vector< T > *_elems_, U4 _pos_ );
    /// insert new item into sorted vector; result is again sorted
    T* insertSorted( const T& _elem_, int (*compare)( const T &, const T & ) ); // = Vector< T, Allocator >::lessThan );
    /// removes and returns const element at end of vector, decrements its size
    T& popBack() { ASSERT( this->_size >= this->_end && this->_end ); return this->_data[--this->_end]; }
    /// removes and returns const element at given position, decrements its size
    T* popAt( U4 _pos_ );
    /// removes and returns const element at given position, decrements its size
    T* popAt( Iterator _it_ );
    /// returns element at end of vector
    T& back() { return this->_data[this->_end - 1]; }
    /// sets its count to 0
    void erase() { ASSERT( this->_size >= this->_end ); this->_end = 0; }
    /// go back to initial state, without doing anything with its items
    void release();
    /// return max entry using < and > operators
    const T* getMax();
    /// return min entry using < and > operators
    const T* getMin();
    /// sort vector using given compare function (see qsort)
    void sort( int (*comp)(const T *, const T *) );

    /// sort vector using < and > operators

    /// @param _descending_ if false (default) sort ascending, otherwise descending
    void sort( bool _descending_ = false );

    /// binary search in an ascending vector

    /// If found then true is returned @a _index_ gets the position, else false is returned.
    /// If there could be multiple matches in the vector it is undefined which one will be returned.
    bool binarySearch( const T * _elem_, unsigned * _index_ = 0 ) const;

    /// search for the first occurence

    /// If found then true is returned @a _index_ gets the position, else false is returned. The first match is returned.
    bool contains( const T * _elem_, unsigned * _index_ = 0 ) const;

    /// output vector to a stream
    template< class TT, class AA > friend ostream& operator<<( ostream& _os_, const Vector< TT, AA >& _vector_ );
    /// input vector from a stream
    template< class TT, class AA > friend istream& operator>>( istream& _is_, Vector< TT, AA >& _vector_ );

protected:
    T* _data;
    U4 _size;
    U4 _end;
    T* pushBack_( const T& _elem_ );

private:
    /// comparison function for sort()

    /// @note icc 9.1 on IA64 had problems with this function when it
    ///       was implemented outside the Vector class.
    static int lessThan( const T* a, const T* b )    { return *a < *b ? -1 : ( *b < *a ? 1 : 0 ); }

    /// comparison function for sort()

    /// @note icc 9.1 on IA64 had problems with this function when it
    ///       was implemented outside the Vector class.
    static int greaterThan( const T* a, const T* b ) { return *a > *b ? -1 : ( *b > *a ? 1 : 0 ); }
    /// swap the content of 2 elements
    static void swap( T & _a_, T & _b_ ) { T _c( _a_ ); _a_ = _b_; _b_ = _c; }
    /// edit this to switch back to libc qsort
    void qSort( U4 _start_, U4 _end_, int (*comp)(const T *, const T *) );
};

/// Use this if you'd like a 2-dimensional Vector
template< class T, class Allocator = DefaultAllocator< T > >
class Vector2D : public Vector< Vector< T, Allocator >, ClearAllocator< Vector < T, Allocator > > >
{
public:
    typedef Vector< Vector< T, Allocator >, ClearAllocator< Vector < T, Allocator > > > Parent_t;
    /// Iterator for outer loops
    typedef typename Parent_t::Iterator OIterator;
    /// ConstIterator for outer loops
    typedef typename Parent_t::ConstIterator OConstIterator;
    /// Iterator for inner loops
    typedef typename Vector< T, Allocator >::Iterator      IIterator;
    /// ConstIterator for inner loops
    typedef typename Vector< T, Allocator >::ConstIterator IConstIterator;
    /// type of inner Vector
    typedef Vector< T, Allocator > InnerVec_t;
    /// sets its counts to 0
    void erase() {
        for( OIterator _v2d_i = this->begin(); _v2d_i != this->end(); ++_v2d_i ) (*_v2d_i).resize(0); Parent_t::erase(); }

    Vector2D( int _size_ = 0 ) : Vector< Vector< T, Allocator >, ClearAllocator< Vector< T, Allocator > > >( _size_ ) {}
};


/// Template for arrays/vectors which are readable, writeable, and bufferizable.
/// It is illegal to use non-const methods between attach() and detach().
/// @param T     type of which array/vector is requested
/// @param Allocator class providing functions for de/allocation (see DefaultAllocator)
/// @param Bufferaize class prividing function for bufferization (see DefaultBufferizer)
template< class T, class Allocator = DefaultAllocator< T >, class Bufferizer = DefaultBufferizer< T > >
class BVector : public Vector< T, Allocator >, public Bufferizable
{
public:
    ///
    /// @param _size_ reserve enough memory for @a _size_ elements, doesn't resize(), only reserve()
    BVector( int _size_ = 0 ) : Vector< T, Allocator >( _size_ ), Bufferizable() {}
    /// copies Vector
    BVector( const Vector< T, Allocator >& _org_ ) : Vector< T, Allocator >( _org_ ), Bufferizable() {}
    /// copies buffer
    BVector( const T* _array_, U4 _n_ ) : Vector< T, Allocator >( _array_, _n_ ), Bufferizable() {}
    /// don't free mem if attached to buffer
    virtual ~BVector() {
        if( this->_size || Bufferizer::freeBuffer() ) Allocator::dealloc( this->_data, this->_size ? this->_size : this->_end, this->_end ); this->_data = NULL; }

    /// returns number of bytes needed to bufferize object
    virtual U4 bufferizedSize() const;
    /// puts object into buffer and returns number of bytes used
    virtual U4 bufferize( unsigned char * _buffer_, U4 _buffSize_ ) const;
    /// inverse of bufferize(): read buffer to build object and return number of bytes read
    virtual U4 debufferize( const unsigned char * _buffer_, U4 _buffSize_ );
    /// same as debufferize(), but can use memory of buffer until detach is called
    virtual U4 attach( const unsigned char * _buffer_, U4 _buffSize_ );
    /// object is still needed, but buffer can no longer be used
    virtual void detach();
};

template< class T, class Allocator >
Vector< T, Allocator >::~Vector()
{
    Allocator::dealloc( this->_data, this->_size ? this->_size : this->_end, this->_end );
    ASSERT( this->_data == NULL );
}

template< class T, class Allocator >
T* Vector< T, Allocator >::pushBack_( const T& _elem_ )
{
    ASSERT( this->_size >= this->_end );
    if( this->_size == 0 ) {
        reserve( 4 );
    }
    if( this->_end == this->_size ) {
        reserve( this->_size * 2 );
    }
    T* _vpb_res = &this->_data[this->_end];
    ++this->_end;
    *_vpb_res = _elem_;
    return _vpb_res;
}

template< class T, class Allocator >
T* Vector< T, Allocator >::findOrInsert( const T* _elem_ )
{
    for( U4 _vfoi_i = 0; _vfoi_i < this->_end; ++_vfoi_i ) {
        if( this->_data[_vfoi_i] == *_elem_ ) return &this->_data[_vfoi_i];
    }
    return pushBack_( *_elem_ );
}

template< class T, class Allocator >
T* Vector< T, Allocator >::insertAt( const T* _elem_, U4 _pos_ )
{
    if( _pos_ >= count() ) return pushBack_( *_elem_ );
    reserve( count() + 1 );
    memmove( this->_data + _pos_ + 1, this->_data + _pos_, ( count() - _pos_ ) * sizeof( T ) );
    memset( this->_data + _pos_, 0, sizeof( T ) );
    *(this->_data + _pos_) = *_elem_;
    this->_end++;
    return this->_data + _pos_;
}

template< class T, class Allocator >
T* Vector< T, Allocator >::insertAt( const Vector< T > *_elems_, U4 _pos_ )
{
    reserve( count() + _elems_->count() );
    if( _pos_ < count() ) {
        memmove( this->_data + _pos_ + _elems_->count(), this->_data + _pos_, ( count() - _pos_ ) * sizeof( T ) );
        memset( this->_data + _pos_, 0, _elems_->count() * sizeof( T ) );
    } else {
        _pos_ = count();
    }
    for( U4 _via_i = 0; _via_i < _elems_->count(); ++_via_i ) {
        *(this->_data + _pos_ + _via_i) = (*_elems_)[_via_i];
    }
    this->_end += _elems_->count();
    return this->_data + _pos_;
}

template< class T, class Allocator >
T* Vector< T, Allocator >::popAt( U4 _pos_ )
{
    ASSERT( _pos_ < count() );
    if( _pos_ == this->_end - 1 ) return &popBack();
    T _vpa_tmp;
    memcpy( &_vpa_tmp, this->_data + _pos_, sizeof( T ) );
    memmove( this->_data + _pos_, this->_data + _pos_ + 1, ( this->_end - _pos_ - 1 ) * sizeof( T ) );
    --this->_end;
    memcpy( this->_data + this->_end, &_vpa_tmp, sizeof( T ) );
    return this->_data + this->_end;
}

template< class T, class Allocator >
T* Vector< T, Allocator >::popAt( typename Vector< T, Allocator >::Iterator _it_ )
{
    ASSERT( _it_ >= this->_data );
    return popAt( (U4)(_it_ - this->_data) );
}

template< class T, class Allocator >
Vector< T, Allocator >::Vector( int _size_ )
    : _data( 0 ), _size( 0 ), _end( 0 )
{
    if( _size_ ) reserve( _size_ );
}

template< class T, class Allocator >
Vector< T, Allocator >::Vector( const T* _array_, U4 _n_ )
    : _size( _n_ ), _end( _n_ )
{
    if( _n_ ) {
        Allocator::alloc( this->_data, _n_ );
        for( U4 i = 0; i < _n_; ++i ) {
            Allocator::copy( &this->_data[i], _array_[i] );
        }
    } else {
        this->_data = 0;
    }
}

template< class T, class Allocator >
void Vector< T, Allocator >::reserve( U4 _size_ )
{
    ASSERT( this->_size >= this->_end );
    if( _size_ > this->_size ) {
        U4 org_end = this->_end;
        resize( _size_ );
        this->_end = org_end;
    }
}

template< class T, class Allocator >
void Vector< T, Allocator >::resize( U4 _size_ )
{
    ASSERT( this->_size >= this->_end );
    if( _size_ != this->_size ) {
        if( _size_ ) {
            T* tmp;
            Allocator::alloc( tmp, _size_ );
            if( this->_end ) {
                Allocator::moveTo( tmp, this->_data, ( _size_ <= this->_end ? _size_ : this->_end ) );
            }
            Allocator::dealloc( this->_data, this->_size, this->_end );
            this->_data = tmp;
        } else {
            Allocator::dealloc( this->_data, this->_size, this->_end );
        }
        this->_size = _size_;
    }
    this->_end = _size_;
}

template< class T, class Allocator >
void Vector< T, Allocator >::setCount( U4 _cnt_ )
{
    if( _cnt_ <= this->_size ) {
        this->_end = _cnt_;
    }
}

template< class T, class Allocator >
const Vector< T, Allocator >& Vector< T, Allocator >::operator=( const Vector< T, Allocator >& _org_ )
{
    if( this->_size < _org_._end ) resize( _org_._end );
    for( U4 ii = 0; ii < _org_._end; ++ii ) {
        Allocator::copy( &this->_data[ii], _org_._data[ii] );
    }
    this->_end = _org_._end;

    return *this;
}

template< class T, class Allocator >
bool Vector< T, Allocator >::operator==( const Vector< T, Allocator >& _org_ ) const
{
    if( this->_end != _org_._end ) return false;
    for( U4 _voe_i = 0; _voe_i<this->_end; ++_voe_i ) {
        if( ! ( this->_data[_voe_i] == _org_._data[_voe_i] ) ) return false;
    }
    return true;
}

template< class T, class Allocator >
void Vector< T, Allocator >::sort( bool descending )
{
    if( descending ) {
        sort( greaterThan );
    } else {
        sort( lessThan );
    }
}

template< class T, class Allocator >
void Vector< T, Allocator >::sort( int (*comp)(const T *, const T *) )
{
    ASSERT( this->_size >= this->_end );
// See tr#650
#ifdef ITAC_WINDOWS
    qsort( this->_data, this->_end, sizeof( *this->_data ), (int (*)(const void *, const void *))comp );
#else
    this->qSort( 0, _end, comp );
#endif
}

/// implements qsort.
/// Spawns only one recursion per level (second is done 'inline/inloop').
/// Minimizes memory consumption
/// Stops if size of array <=3 and sorts it 'manually'.
template< class T, class Allocator >
inline void Vector< T, Allocator >::qSort( U4 _start_, U4 _end_, int (*comp)(const T *, const T *) )
{
    I4 n = (I4)_end_ - (I4)_start_; 
    if (n < 2) return; // avoid to dereference invalid pointers

    T p( _data[_start_] );
    U4 l( _start_ );
    U4 r( _end_ );

    while( n > 3 ) {
        while( (++l) < (_end_-1) && comp( &_data[l], &p ) <= 0 ) {};
        while( comp( &_data[--r], &p ) > 0 ) {};
        while( l < r ) {
            swap( _data[l], _data[r] );
            while( comp( &_data[--r], &p ) > 0 ) {};
            while( comp( &_data[++l], &p ) <= 0 ) {};
        }
        swap( _data[r], _data[_start_] );
        qSort( _start_, r, comp );
        // prepare for in-loop recursion of second half
        p = _data[r+1];
        l = _start_ = r + 1;
        r = _end_;
        n = (I4)_end_ - (I4)_start_; 
    }
    if( n > 1 && comp( &_data[_start_], &_data[_start_+1] ) > 0 ) {
        swap( _data[_start_], _data[_start_+1] );
    }
    if( n == 3 ) {
        if( comp( &_data[_start_], &_data[_start_+2] ) > 0 ) {
            swap( _data[_start_], _data[_start_+2] );
        }
        if( comp( &_data[_start_+1], &_data[_start_+2] ) > 0 ) {
            swap( _data[_start_+1], _data[_start_+2] );
        }
    }   
}

template< class T, class Allocator >
T* Vector< T, Allocator >::insertSorted( const T& _elem_, int (*compare)( const T&, const T& ) )
{ 
    int _i = count() - 1;
    while( _i >= 0 && compare( _data[_i], _elem_ ) > 0 ) --_i;
    return _i > 0 ? insertAt( &_elem_, _i + 1 ) : pushBack( _elem_ );
}

template< class T, class Allocator >
bool  Vector< T, Allocator >::binarySearch(const T* _elem_, unsigned *_pos_) const
{
    if ( 0 == count() ) return false;

    U4 dummy;
    if ( 0 == _pos_ ) _pos_ = &dummy;

    U4 lb  = 0;                    // lower bound
    U4 ub  = count() - 1;          // upper bound
    U4 mid = lb + ( ub - lb ) / 2; // don't use (ub+lb)/2 because it could overflow :-)
    do {
        if ( this->_data[mid] == *_elem_ ) { *_pos_ = mid; return true; }
        if ( this->_data[mid] < *_elem_ ) { 
            lb = mid + 1; 
        } else { 
            ub = mid - 1; 
        }
        mid = lb + ( ub - lb ) / 2;
    } while ( lb < ub && ub < count() );

    if ( lb > 0 && lb <= ub && this->_data[lb] == *_elem_ ) { *_pos_ = lb; return true; }
    if ( ub >= 0 && ub < count() && this->_data[ub] == *_elem_ ) { *_pos_ = ub; return true; } //FS FIXME: comparison of unsigned expression >= 0 is always true

    return false;
}

template< class T, class Allocator >
bool  Vector< T, Allocator >::contains(const T* _elem_, unsigned *_pos_) const
{
    if (0 == count()) return false;

    U4 dummy;
    if (0 == _pos_) _pos_ = &dummy;

    for(unsigned i=0;i<count();i++)
        if(this->_data[i] == *_elem_) {
            *_pos_=i;
            return true;
        }
    return false;
}

template< class T, class Allocator >
void Vector< T, Allocator >::release()
{
    // some allocators check for != 0 (NULL), this is a hack :(
    memset( this->_data, 0, this->_end * sizeof( T ) );
    Allocator::dealloc( this->_data, this->_size ? this->_size : this->_end, this->_end );
    this->_end = this->_size = 0;
}

template< class T, class Allocator >
const T* Vector< T, Allocator >::getMax()
{
    if( count() ) {
        ConstIterator _mv_i = begin();
        const T * _mv_mx = &(*_mv_i);
        ++_mv_i;
        while( _mv_i != end() ) {
            if( *_mv_mx < *_mv_i ) _mv_mx = &(*_mv_i);
            ++_mv_i;
        }
        return _mv_mx;
    }
    return NULL;
}

template< class T, class Allocator >
const T* Vector< T, Allocator >::getMin()
{
    if( count() ) {
        ConstIterator _mv_i = begin();
        const T * _mv_mn = &(*_mv_i);
        ++_mv_i;
        while( _mv_i != end() ) {
            if( *_mv_mn > *_mv_i ) _mv_mn = &(*_mv_i);
            ++_mv_i;
        }
        return _mv_mn;
    }
    return NULL;
}

template< class T, class Allocator >
ostream& operator<<( ostream& _os_, const Vector< T, Allocator >& _vector_ )
{
    Allocator::output( _os_, _vector_._data, _vector_._end );
    return _os_;
}

template< class T, class Allocator >
istream& operator>>( istream& _is_, Vector< T, Allocator >& _vector_ )
{
    Allocator::input( _is_, _vector_._data, _vector_._size, _vector_._end );
    return _is_;
}

// Implementation of BVector

template< class T, class Allocator, class Bufferizer >
U4 BVector< T, Allocator, Bufferizer >::bufferizedSize() const
{
    return align( align( sizeof( this->_end ) ) + Bufferizer::bufferizedSize( this->_data, this->_end ) );
}

template< class T, class Allocator, class Bufferizer >
U4 BVector< T, Allocator, Bufferizer >::bufferize( unsigned char * _buffer_, U4 _buffSize_ ) const
{
    if( _buffSize_ < this->bufferizedSize() ) return (U4)SMALL_BUFFER;
    *((U4*)_buffer_) = this->_end;
    return align( align( sizeof( U4 ) ) + Bufferizer::bufferize( this->_data, this->_end, _buffer_ + align( sizeof( U4 ) ), _buffSize_ - align( sizeof( U4 ) ) ) );
}

template< class T, class Allocator, class Bufferizer >
U4 BVector< T, Allocator, Bufferizer >::debufferize( const unsigned char * _buffer_, U4 _buffSize_ )
{
    U4 bvd_sz = align( sizeof( U4 ) );
    if( _buffSize_ < bvd_sz ) return (U4)SMALL_BUFFER;
    U4 sz = *((U4*)_buffer_);
    this->reserve( sz );
    this->_end = 0;
    U4 ret = Bufferizer::debufferize( this->_data, sz, _buffer_ + bvd_sz, _buffSize_ - bvd_sz );
    if( ret == (U4)SMALL_BUFFER ) return (U4)SMALL_BUFFER;
    this->_end = sz;
    return align( bvd_sz + ret );
}

template< class T, class Allocator, class Bufferizer >
U4 BVector< T, Allocator, Bufferizer >::attach( const unsigned char * _buffer_, U4 _buffSize_ )
{
    U4 bva_sz = align( sizeof( U4 ) );
    if( _buffSize_ < bva_sz ) return (U4)SMALL_BUFFER;
    U4 sz = *((U4*)_buffer_);
    if( this->_size || Bufferizer::freeBuffer() ) {
        Allocator::dealloc( this->_data, this->_size ? this->_size : this->_end, this->_end );
        this->_size = 0;
    }
    this->_end = 0;
    U4 ret = Bufferizer::attach( this->_data, sz, _buffer_ + bva_sz, _buffSize_ - bva_sz );
    if( ret == (U4)SMALL_BUFFER ) return (U4)SMALL_BUFFER;
    this->_end = sz;
    return align( bva_sz + ret );
}

template< class T, class Allocator, class Bufferizer >
void BVector< T, Allocator, Bufferizer >::detach()
{
    BVector< T, Allocator, Bufferizer > _dtv;
    _dtv._end = this->_end;
    _dtv._size = this->_size;
    _dtv._data = this->_data;
    this->_end = this->_size = 0;
    this->_data = NULL;
    *this = _dtv;
    if( ! Bufferizer::freeBuffer() ) {
        _dtv._data = NULL;
        _dtv._size = _dtv._end = 0;
    }
}

#endif
