/***
* ==++==
*
* Copyright (c) Microsoft Corporation.  All rights reserved.
* 
* ==--==
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* internal_concurrent_hash.h
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/
#pragma once

#include <utility>
#include "internal_split_ordered_list.h"
#include <concrt.h>

namespace Concurrency
{
namespace details
{
// Template class for hash compare
template<typename _Key_type, typename _Hasher, typename _Key_equality>
class _Hash_compare
{
public:
    typedef _Hasher hasher;

    _Hash_compare()
    {
    }

    _Hash_compare(hasher _Hasharg) : _M_hash_object(_Hasharg)
    {
    }

    _Hash_compare(hasher _Hasharg, _Key_equality _Keyeqarg) : _M_hash_object(_Hasharg), _M_key_compare_object(_Keyeqarg)
    {
    }

    size_t operator()(const _Key_type& _Keyval) const
    {
        return ((size_t)_M_hash_object(_Keyval));
    }

    bool operator()(const _Key_type& _Keyval1, const _Key_type& _Keyval2) const
    {
        return (!_M_key_compare_object(_Keyval1, _Keyval2));
    }

    hasher        _M_hash_object;        // The hash object
    _Key_equality _M_key_compare_object; // The equality comparator object
};

// An efficient implementation of the _Reverse function utilizes a 2^8 or 2^16 lookup table holding the
// bit-reversed values of [0..2^8 - 1] or [0..2^16 - 1] respectively. Those values can also be computed
// on the fly at a slightly higher cost.
const unsigned char _Byte_reverse_table[] =
{
    0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
    0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
    0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
    0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
    0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
    0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
    0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
    0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
    0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
    0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
    0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
    0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
    0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
    0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
    0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
    0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

// Given a byte, reverses it
inline unsigned char _Reverse_byte(unsigned char _Original_byte)
{
    // return ((_Original_byte * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32;
    return _Byte_reverse_table[_Original_byte];
}

// Finds the most significant bit in a size_t
inline unsigned char _Get_msb(size_t _Mask)
{
    unsigned long _Index = 0;

#if defined(_M_IX86)
    _BitScanReverse(&_Index, _Mask);
#else
    _BitScanReverse64(&_Index, _Mask);
#endif

    return (unsigned char) _Index;
}

#pragma warning(push)
#pragma warning(disable: 4127) // warning 4127 -- while (true) has a constant expression in it

template <typename _Traits>
class _Concurrent_hash : public _Traits
{
public:
    // Type definitions
    typedef _Concurrent_hash<_Traits> _Mytype;
    typedef typename _Traits::_Value_type _Value_type;
    typedef typename _Traits::value_type value_type;
    typedef typename _Traits::key_type key_type;
    typedef typename _Traits::key_compare key_compare;
    typedef typename _Traits::value_compare value_compare;

    typedef typename _Traits::allocator_type allocator_type;
    typedef typename allocator_type::pointer pointer;
    typedef typename allocator_type::const_pointer const_pointer;
    typedef typename allocator_type::reference reference;
    typedef typename allocator_type::const_reference const_reference;

    typedef typename allocator_type::size_type size_type;
    typedef typename allocator_type::difference_type difference_type;

    typedef typename _Split_ordered_list<typename _Traits::value_type, typename _Traits::allocator_type> _Mylist;
    typedef typename _Mylist::_Nodeptr _Nodeptr;

    typedef typename std::tr1::conditional<std::tr1::is_same<key_type, value_type>::value, typename _Mylist::const_iterator, typename _Mylist::iterator>::type iterator;
    typedef typename _Mylist::const_iterator const_iterator;
    typedef iterator local_iterator;
    typedef const_iterator const_local_iterator;
    typedef std::pair<iterator, bool> _Pairib;
    typedef std::pair<iterator, iterator> _Pairii;
    typedef std::pair<const_iterator, const_iterator> _Paircc;

    // Iterators that walk the entire split-order list, including dummy nodes
    typedef typename _Mylist::_Full_iterator _Full_iterator;
    typedef typename _Mylist::_Full_const_iterator _Full_const_iterator;

    static const size_type _Initial_bucket_number = 8;                               // Initial number of buckets
    static const size_type _Initial_bucket_load = 4;                                 // Initial maximum number of elements per bucket
    static size_type const _Pointers_per_table = sizeof(size_type) * 8;              // One bucket segment per bit

    // Constructors/Destructors
    _Concurrent_hash(size_type _Number_of_buckets = _Initial_bucket_number, const key_compare& _Parg = key_compare(), const allocator_type& _Allocator = allocator_type())
        : _Traits(_Parg), _M_number_of_buckets(_Number_of_buckets), _M_split_ordered_list(_Allocator), _M_allocator(_Allocator), _M_maximum_bucket_size((float) _Initial_bucket_load)
    {
        _Init();
    }

    _Concurrent_hash(const _Concurrent_hash& _Right, const allocator_type& _Allocator) : _Traits(_Right._M_comparator), _M_split_ordered_list(_Allocator), _M_allocator(_Allocator)
    {
        _Copy(_Right);
    }

    _Concurrent_hash(const _Concurrent_hash& _Right) : _Traits(_Right._M_comparator), _M_split_ordered_list(_Right.get_allocator()), _M_allocator(_Right.get_allocator())
    {
        _Init();
        _Copy(_Right);
    }

    _Concurrent_hash(_Concurrent_hash&& _Right) : _Traits(_Right._M_comparator), _M_split_ordered_list(_Right.get_allocator()), _M_allocator(_Right.get_allocator()),
        _M_number_of_buckets(_Initial_bucket_number), _M_maximum_bucket_size((float) _Initial_bucket_load)
    {
        _Init();
        swap(_Right);
    }

    _Concurrent_hash& operator=(const _Concurrent_hash& _Right)
    {
        if (this != &_Right)
        {
            _Copy(_Right);
        }

        return (*this);
    }

    _Concurrent_hash& operator=(_Concurrent_hash&& _Right)
    {
        if (this != &_Right)
        {
            clear();
            swap(_Right);
        }

        return (*this);
    }

    ~_Concurrent_hash()
    {
        // Delete all node segments
        for (size_type _Index = 0; _Index < _Pointers_per_table; _Index++)
        {
            if (_M_buckets[_Index] != NULL)
            {
                size_type _Seg_size = _Segment_size(_Index);
                for (size_type _Index2 = 0; _Index2 < _Seg_size; _Index2++)
                {
                    _M_allocator.destroy(&_M_buckets[_Index][_Index2]);
                }
                _M_allocator.deallocate(_M_buckets[_Index], _Seg_size);
            }
        }
    }

    static size_type __cdecl _Segment_index_of( size_type _Index )
    {
        return size_type( _Get_msb( _Index|1 ) );
    }

    static size_type _Segment_base( size_type _K )
    {
        return (size_type(1)<<_K & ~size_type(1));
    }

    static size_type _Segment_size( size_type _K )
    {
        return _K ? size_type(1)<<_K : 2;
    }

    /// <summary>
    ///     Returns the allocator used for this concurrent container. 
    /// </summary>
    /// <remarks>
    ///     This function is concurrency safe.
    /// </remarks>
    /// <returns>
    ///     The allocator used for this concurrent container.
    /// </returns>
    /**/
    allocator_type get_allocator() const
    {
        return _M_split_ordered_list.get_allocator();
    }

    // Size and capacity function
    /// <summary>
    ///     Checks whether the number of elements in this concurrent container is non-zero. 
    /// </summary>
    /// <remarks>
    ///     This function is concurrency safe.
    ///     <para>With concurrent inserts, whether or not the concurrent container is empty may change 
    ///     immediately after calling this function, before the return value is even read.</para>
    /// </remarks>
    /// <returns>
    ///     True, if this concurrent container object is empty, false, otherwise.
    /// </returns>
    /**/
    bool empty() const
    {
        return _M_split_ordered_list.empty();
    }

    /// <summary>
    ///     Returns the number of elements in this concurrent container. 
    /// </summary>
    /// <remarks>
    ///     This function is concurrency safe.
    ///     <para>With concurrent inserts, the number of elements in the concurrent container may change 
    ///     immediately after calling this function, before the return value is even read.</para>
    /// </remarks>
    /// <returns>
    ///     The number of items in the container.
    /// </returns>
    /**/
    size_type size() const
    {
        return _M_split_ordered_list.size();
    }

    /// <summary>
    ///     Returns the maximum size of the concurrent container, determined by the allocator.
    /// </summary>
    /// <remarks>
    ///     This function is concurrency safe.
    ///     <para>This upper bound value may actually be higher than what the container can actually hold.</para>
    /// </remarks>
    /// <returns>
    ///     The maximum number of elements that can be put into this concurrent container.
    /// </returns>
    /**/
    size_type max_size() const
    {
        return _M_split_ordered_list.max_size();
    }

    // Iterators 
    /// <summary>
    ///     Returns an iterator pointing to the first element in the concurrent container.
    /// </summary>
    /// <remarks>
    ///     This function is concurrency safe.
    /// </remarks>
    /// <returns>
    ///     An iterator to the first element in the concurrent container.
    /// </returns>
    /**/
    iterator begin()
    {
        return _M_split_ordered_list.begin();
    }

    /// <summary>
    ///     Returns a const_iterator pointing to the first element in the concurrent container.
    /// </summary>
    /// <remarks>
    ///     This function is concurrency safe.
    /// </remarks>
    /// <returns>
    ///     A const_iterator to the first element in the concurrent container.
    /// </returns>
    /**/
    const_iterator begin() const
    {
        return _M_split_ordered_list.begin();
    }

    /// <summary>
    ///     Returns an iterator pointing to the location succeeding the last element in the concurrent container.
    /// </summary>
    /// <remarks>
    ///     This function is concurrency safe.
    /// </remarks>
    /// <returns>
    ///     An iterator to the location succeeding the last element in the concurrent container.
    /// </returns>
    /**/
    iterator end()
    {
        return _M_split_ordered_list.end();
    }

    /// <summary>
    ///     Returns a const_iterator pointing to the location succeeding the last element in the concurrent container.
    /// </summary>
    /// <remarks>
    ///     This function is concurrency safe.
    /// </remarks>
    /// <returns>
    ///     A const_iterator to the location succeeding the last element in the concurrent container.
    /// </returns>
    /**/
    const_iterator end() const
    {
        return _M_split_ordered_list.end();
    }

    /// <summary>
    ///     Returns a const_iterator pointing to the first element in the concurrent container.
    /// </summary>
    /// <remarks>
    ///     This function is concurrency safe.
    /// </remarks>
    /// <returns>
    ///     A const_iterator to the first element in the concurrent container.
    /// </returns>
    /**/
    const_iterator cbegin() const
    {
        return _M_split_ordered_list.cbegin();
    }

    /// <summary>
    ///     Returns a const_iterator pointing to the location succeeding the last element in the concurrent container.
    /// </summary>
    /// <remarks>
    ///     This function is concurrency safe.
    /// </remarks>
    /// <returns>
    ///     A const_iterator to the location succeeding the last element in the concurrent container.
    /// </returns>
    /**/
    const_iterator cend() const
    {
        return _M_split_ordered_list.cend();
    }

    // Modifiers
    /// <summary>
    ///     Inserts a value into the concurrent container.
    /// </summary>
    /// <param name="_Value">
    ///     The value to insert.
    /// </param>
    /// <remarks>
    ///     This function is concurrency safe.
    /// </remarks>
    /// <returns>
    ///     A <see cref="pair Class">pair</see> where the first object is an iterator into the container at the insertion point and the 
    ///     second object is a bool indicating whether the value was inserted (true) or not (false).
    /// </returns>
    /**/
    _Pairib insert(const value_type& _Value)
    {
        return _Insert(_Value);
    }

    /// <summary>
    ///     Inserts a value into the concurrent container.
    /// </summary>
    /// <typeparam name="_Valty">
    ///     The type of the value inserted into the map.
    /// </typeparm>
    /// <param name="_Value">
    ///     The value to insert.
    /// </param>
    /// <remarks>
    ///     This function is concurrency safe.
    /// </remarks>
    /// <returns>
    ///     A <see cref="pair Class">pair</see> where the first object is an iterator into the container at the insertion point and the 
    ///     second object is a bool indicating whether the value was inserted (true) or not (false).
    /// </returns>
    /**/
    template<class _Valty>
    _Pairib insert(_Valty&& _Value)
    {
        return _Insert(std::forward<_Valty>(_Value));
    }

    /// <summary>
    ///     Inserts a value into the concurrent container.
    /// </summary>
    /// <param name="_Value">
    ///     The value to insert.
    /// </param>
    /// <remarks>
    ///     This function is concurrency safe.
    ///     <para>The current implementation ignores the first <c>const_iterator</c> argument. It exists for
    ///     similarity with the <see cref="unordered_map Class">unordered_map</see>, and hints to a location to start the search
    ///     for an insertion point.</para>
    /// </remarks>
    /// <returns>
    ///     An iterator pointing to the insertion point of the object.  If the key already exists in the container
    ///     and the container does not support multiple keys, an iterator pointing to the duplicate key location in 
    ///     the map is returned. 
    /// </returns>
    /**/
    iterator insert(const_iterator, const value_type& _Value)
    {
        // Ignore hint
        return insert(_Value).first;
    }

    /// <summary>
    ///     Inserts a value into the concurrent container.
    /// </summary>
    /// <typeparam name="_Valty">
    ///     The type of the value inserted into the map.
    /// </typeparm>
    /// <param name="_Value">
    ///     The value to insert.
    /// </param>
    /// <remarks>
    ///     This function is concurrency safe.
    ///     <para>The current implementation ignores the first <c>const_iterator</c> argument. It exists for
    ///     similarity with the <see cref="unordered_map Class">unordered_map</see>, and hints to a location to start the search
    ///     for an insertion point.</para>
    /// </remarks>
    /// <returns>
    ///     An iterator pointing to the insertion point of the object.  If the key already exists in the container
    ///     and the container does not support multiple keys, an iterator pointing to the duplicate key location in 
    ///     the map is returned.
    /// </returns>
    /**/
    template<class _Valty>
        typename std::tr1::enable_if<!std::tr1::is_same<const_iterator, 
            typename std::tr1::remove_reference<_Valty>::type>::value, iterator>::type
    insert(const_iterator, _Valty&& _Value)
    {
        // Ignore hint
        return insert(std::forward<_Valty>(_Value)).first;
    }

    /// <summary>
    ///     Inserts a set of values into the concurrent container from an iterator.
    /// </summary>
    /// <typeparam name="_Iterator">
    ///     The iterator type used for insertion.
    /// </typeparm>
    /// <param name="_First">
    ///     The input iterator pointing to the beginning location.
    /// </param>
    /// <param name="_Last">
    ///     The input iterator pointing to the end location.
    /// </param>
    /// <remarks>
    ///     This function is concurrency safe.
    /// </remarks>
    /**/
    template<class _Iterator>
    void insert(_Iterator _First, _Iterator _Last)
    {
        for (_Iterator _I = _First; _I != _Last; _I++)
        {
            insert(*_I);
        }
    }

    /// <summary>
    ///     Erases an element from the concurrent container.
    /// </summary>
    /// <param name="_Where">
    ///     A <c>const_iterator</c> pointing to the element to erase.
    /// </param>
    /// <remarks>
    ///     This function is not concurrency safe.
    /// </remarks>
    /// <returns>
    ///     An iterator pointing to the location immediately following the deleted object in the container.
    /// </returns>
    /**/
    iterator unsafe_erase(const_iterator _Where)
    {
        return _Erase(_Where);
    }

    /// <summary>
    ///     Erases multiple elements from the concurrent container.
    /// </summary>
    /// <param name="_First">
    ///     A <c>const_iterator</c> pointing to the first element to erase.
    /// </param>
    /// <param name="_Last">
    ///     A <c>const_iterator</c> pointing to the location after the last element to erase,.
    /// </param>
    /// <remarks>
    ///     This function is not concurrency safe.
    /// </remarks>
    /// <returns>
    ///     An iterator pointing to the location immediately following the deleted object(s) in the container.
    /// </returns>
    /**/
    iterator unsafe_erase(const_iterator _First, const_iterator _Last)
    {
        while (_First != _Last)
        {
            unsafe_erase(_First++);
        }

        return _M_split_ordered_list._Get_iterator(_First);
    }

    /// <summary>
    ///     Erases an element from the concurrent container.
    /// </summary>
    /// <param name="_Keyval">
    ///     The value to erase from the concurrent container.
    /// </param>
    /// <remarks>
    ///     This function is not concurrency safe.
    /// </remarks>
    /// <returns>
    ///     A count of the number of keys removed from the concurrent_unordered_map.  
    /// </returns>
    /**/
    size_type unsafe_erase(const key_type& _Keyval)
    {
        _Pairii _Where = equal_range(_Keyval);
        size_type _Count = _Distance(_Where.first, _Where.second);
        unsafe_erase(_Where.first, _Where.second);
        return _Count;
    }

    /// <summary>
    ///     Swaps the contents of two concurrent containers.
    /// </summary>
    /// <param name="_Right">
    ///     The container to swap elements from.
    /// </param>
    /// <remarks>
    ///     This function is not concurrency safe.
    /// </remarks>
    /// <returns>
    ///     Throws an <see cref="invalid_argument Class">invalid_argument</see> exception if the swap is being
    ///     performed on unequal allocators.
    /// </returns>
    /**/
    void swap(_Concurrent_hash& _Right)
    {
        if (this != &_Right)
        {
            std::_Swap_adl(_M_comparator, _Right._M_comparator);
            _M_split_ordered_list.swap(_Right._M_split_ordered_list);
            _Swap_buckets(_Right);
            std::swap(_M_number_of_buckets, _Right._M_number_of_buckets);
            std::swap(_M_maximum_bucket_size, _Right._M_maximum_bucket_size);
        }
    }

    // Observers
    /// <summary>
    ///     Clears all the objects in the concurrent container.
    /// </summary>
    /// <remarks>
    ///     This function is not concurrency safe.
    /// </remarks>
    /**/
    void clear()
    {
        // Clear list
        _M_split_ordered_list.clear();

        // Clear buckets
        for (size_type _Index = 0; _Index < _Pointers_per_table; _Index++)
        {
            if (_M_buckets[_Index] != NULL)
            {
                size_type _Seg_size = _Segment_size(_Index);
                for (size_type _Index2 = 0; _Index2 < _Seg_size; _Index2++)
                {
                    _M_allocator.destroy(&_M_buckets[_Index][_Index2]);
                }
                _M_allocator.deallocate(_M_buckets[_Index], _Seg_size);
            }
        }

        // memset all the buckets to zero and initialize the dummy node 0
        _Init();
    }

    // Lookup
    /// <summary>
    ///     Searches the concurrent container for a specific key.
    /// </summary>
    /// <param name="_Keyval">
    ///     The key to search for.
    /// </param>
    /// <remarks>
    ///     This function is concurrency safe.
    /// </remarks>
    /// <returns>
    ///     An iterator pointing to the location of the searched for key, or points to end() if not found.
    /// </returns>
    /**/
    iterator find(const key_type& _Keyval)
    {
        return _Find(_Keyval);
    }

    /// <summary>
    ///     Searches the concurrent container for a specific key.
    /// </summary>
    /// <param name="_Keyval">
    ///     The key to search for.
    /// </param>
    /// <remarks>
    ///     This function is concurrency safe.
    /// </remarks>
    /// <returns>
    ///     A const_iterator pointing to the location of the searched for key.
    /// </returns>
    /**/
    const_iterator find(const key_type& _Keyval) const
    {
        return _Find(_Keyval);
    }

    /// <summary>
    ///     Counts the number of times a specific key appears in the container.
    /// </summary>
    /// <param name="_Keyval">
    ///     The key to count.
    /// </param>
    /// <remarks>
    ///     This function is concurrency safe.
    /// </remarks>
    /// <returns>
    ///     The number of times the key appears in the container.
    /// </returns>
    /**/
    size_type count(const key_type& _Keyval) const
    {
        size_type _Count = 0;
        const_iterator _It = _Find(_Keyval);
        for (;_It != end() && !_M_comparator(_Key_function(*_It), _Keyval); _It++)
        {
            _Count++;
        }
        return _Count;
    }

    /// <summary>
    ///     Finds the iterators pointing to the being and end locations a specific key appears in the container.
    /// </summary>
    /// <param name="_Keyval">
    ///     The key to search for.
    /// </param>
    /// <remarks>
    ///     This function is concurrency safe. 
    ///     <para>It is possible that concurrent inserts may cause the additional keys to be inserted after the 
    ///     begin iterator and before the end iterator.</para>
    /// </remarks>
    /// <returns>
    ///     A <see cref="pair Class">pair</see> where the first element is an iterator to the beginning and the second element
    ///     is an iterator to the end of the range..
    /// </returns>
    /**/
    _Pairii equal_range(const key_type& _Keyval)
    {
        return _Equal_range(_Keyval);
    }

    /// <summary>
    ///     Finds the const_iterators pointing to the being and end locations a specific key appears in the container.
    /// </summary>
    /// <param name="_Keyval">
    ///     The key to search for.
    /// </param>
    /// <remarks>
    ///     This function is concurrency safe. 
    ///     <para>It is possible that concurrent inserts may cause the additional keys to be inserted after the 
    ///     begin iterator and before the end iterator.</para>
    /// </remarks>
    /// <returns>
    ///     A <see cref="pair Class">pair</see> where the first element is a const_iterator to the beginning and the second element
    ///     is a const_iterator to the end of the range.
    /// </returns>
    /**/
    _Paircc equal_range(const key_type& _Keyval) const
    {
        return _Equal_range(_Keyval);
    }

    // Bucket interface - for debugging 
    /// <summary>
    ///     Returns the current number of buckets in this container.
    /// </summary>
    /// <returns>
    ///     The current number of buckets in this container.
    /// </returns>
    /**/
    size_type unsafe_bucket_count() const
    {
        return _M_number_of_buckets;
    }

    /// <summary>
    ///     Returns the maximum number of buckets in this container.
    /// </summary>
    /// <returns>
    ///     The maximum number of buckets in this container.
    /// </returns>
    /**/
    size_type unsafe_max_bucket_count() const
    {
        return _Segment_size(_Pointers_per_table-1);
    }

    /// <summary>
    ///     Returns the number of items in a specific bucket of this container.
    /// </summary>
    /// <param name="_Bucket">
    ///     The bucket to search for.
    /// </param>
    /// <returns>
    ///     The current number of buckets in this container.
    /// </returns>
    /**/
    size_type unsafe_bucket_size(size_type _Bucket)
    {
        size_type _Count = 0;

        if (!_Is_initialized(_Bucket))
        {
            return _Count;
        }

        _Full_iterator _Iterator = _Get_bucket(_Bucket);
        _Iterator++;

        for (; _Iterator != _M_split_ordered_list._End() && !_Iterator._Mynode()->_Is_dummy(); _Iterator++)
        {
            _Count++;
        }

        return _Count;
    }

    /// <summary>
    ///     Returns the bucket index that a specific key maps to in this container.
    /// </summary>
    /// <param name="_Keyval">
    ///     The element key being searched for.
    /// </param>
    /// <returns>
    ///     The bucket index for the key in this container.
    /// </returns>
    /**/
    size_type unsafe_bucket(const key_type& _Keyval) const
    {
        _Split_order_key _Order_key = (_Split_order_key) _M_comparator(_Keyval);
        size_type _Bucket = _Order_key % _M_number_of_buckets;
        return _Bucket;
    }

    // If the bucket is initialized, return a first non-dummy element in it

    /// <summary>
    ///     Returns an iterator to the first element in this container for a specific bucket.
    /// </summary>
    /// <param name="_Bucket">
    ///     The bucket index.
    /// </param>
    /// <returns>
    ///     An iterator pointing to the beginning of the bucket.
    /// </returns>
    /**/
    local_iterator unsafe_begin(size_type _Bucket)
    {
        // It is possible the bucket being searched for has not yet been initialized
        if (!_Is_initialized(_Bucket))
        {
            _Initialize_bucket(_Bucket);
        }

        _Full_iterator _Iterator = _Get_bucket(_Bucket);
        return _M_split_ordered_list._Get_first_real_iterator(_Iterator);
    }

    // If the bucket is initialized, return a first non-dummy element in it

    /// <summary>
    ///     Returns a const_iterator to the first element in this container for a specific bucket.
    /// </summary>
    /// <param name="_Bucket">
    ///     The bucket index.
    /// </param>
    /// <returns>
    ///     A const_iterator pointing to the beginning of the bucket.
    /// </returns>
    /**/
    const_local_iterator unsafe_begin(size_type _Bucket) const
    {
        // It is possible the bucket being searched for has not yet been initialized
        if (!_Is_initialized(_Bucket))
        {
            _Initialize_bucket(_Bucket);
        }


        _Full_const_iterator _Iterator = _Get_bucket(_Bucket);
        return _M_split_ordered_list._Get_first_real_iterator(_Iterator);
    }

    // Returns the iterator after the last non-dummy element in the bucket

    /// <summary>
    ///     Returns an iterator to the last element in this container for a specific bucket.
    /// </summary>
    /// <param name="_Bucket">
    ///     The bucket index.
    /// </param>
    /// <returns>
    ///     An iterator pointing to the end of the bucket.
    /// </returns>
    /**/
    local_iterator unsafe_end(size_type _Bucket)
    {
        // If we've increased the number of buckets, there's a chance the intermediate dummy
        // node marking the end of this bucket has not yet been lazily initialized.
        // Inserting from from _M_number_of_buckets/2 to _M_number_of_buckets will recursively
        // initialize all the dummy nodes in the map.
        for(size_type _Bucket_index = _M_number_of_buckets >> 1; _Bucket_index < _M_number_of_buckets; _Bucket_index++)
        {
            if (!_Is_initialized(_Bucket_index))
            {
                _Initialize_bucket(_Bucket_index);
            }
        }

        _Full_iterator _Iterator = _Get_bucket(_Bucket);
    
        // Find the end of the bucket, denoted by the dummy element
        do
        {
            _Iterator++;
        }
        while(_Iterator != _M_split_ordered_list._End() && !_Iterator._Mynode()->_Is_dummy());

        // Return the first real element past the end of the bucket
        return _M_split_ordered_list._Get_first_real_iterator(_Iterator);
    }

    // Returns the iterator after the last non-dummy element in the bucket

    /// <summary>
    ///     Returns a const_iterator to the last element in this container for a specific bucket.
    /// </summary>
    /// <param name="_Bucket">
    ///     The bucket index.
    /// </param>
    /// <returns>
    ///     A const_iterator pointing to the end of the bucket.
    /// </returns>
    /**/
    const_local_iterator unsafe_end(size_type _Bucket) const
    {
        // If we've increased the number of buckets, there's a chance the intermediate dummy
        // node marking the end of this bucket has not yet been lazily initialized.
        // Inserting from from _M_number_of_buckets/2 to _M_number_of_buckets will recursively
        // initialize all the dummy nodes in the map.
        for(size_type _Bucket_index = _M_number_of_buckets >> 1; _Bucket_index < _M_number_of_buckets; _Bucket_index++)
        {
            if (!_Is_initialized(_Bucket_index))
            {
                _Initialize_bucket(_Bucket_index);
            }
        }

        _Full_const_iterator _Iterator = _Get_bucket(_Bucket);
    
        // Find the end of the bucket, denoted by the dummy element
        do
        {
            _Iterator++;
        }
        while(_Iterator != _M_split_ordered_list._End() && !_Iterator._Mynode()->_Is_dummy());

        // Return the first real element past the end of the bucket
        return _M_split_ordered_list._Get_first_real_iterator(_Iterator);
    }

    /// <summary>
    ///     Returns a const_iterator to the first element in this container for a specific bucket.
    /// </summary>
    /// <param name="_Bucket">
    ///     The bucket index.
    /// </param>
    /// <returns>
    ///     A const_iterator pointing to the beginning of the bucket.
    /// </returns>
    /**/
    const_local_iterator unsafe_cbegin(size_type _Bucket) const
    {
        return ((const _Mytype *) this)->begin();
    }

    /// <summary>
    ///     Returns a const_iterator to the last element in this container for a specific bucket.
    /// </summary>
    /// <param name="_Bucket">
    ///     The bucket index.
    /// </param>
    /// <returns>
    ///     A const_iterator pointing to the end of the bucket.
    /// </returns>
    /**/
    const_local_iterator unsafe_cend(size_type _Bucket) const
    {
        return ((const _Mytype *) this)->end();
    }

    // Hash policy
    /// <summary>
    ///     Computes and returns the current load factor of the container.  
    ///     The load factor is the number of elements in the container divided by the number of buckets.
    /// </summary>
    /// <returns>
    ///     The load factor for the container
    /// </returns>
    /**/
    float load_factor() const
    {
        return (float) size() / (float) unsafe_bucket_count();
    }

    /// <summary>
    ///     Returns the maximum load factor of the container.  The maximum load factor is the 
    ///     largest number of elements than can be in any bucket before the container grows its 
    ///     internal table.
    /// </summary>
    /// <returns>
    ///     The maximum load factor for the container
    /// </returns>
    /**/
    float max_load_factor() const
    {
        return _M_maximum_bucket_size;
    }

    /// <summary>
    ///     Sets the maximum load factor of the container to a specific value.
    /// </summary>
    /// <param name="_Newmax">
    ///     The desired load factor for this container.
    /// </param>
    /// <remarks>
    ///     Throws an <see cref="out_of_range Class">out_of_range</see> exception if the load factor is invalid (NaN or negative)
    /// </remarks>
    /**/
    void max_load_factor(float _Newmax)
    {
        // The _Newmax != _Newmax is a check for NaN, because NaN is != to itself
        if (_Newmax != _Newmax || _Newmax < 0)
        {
            throw std::out_of_range("invalid hash load factor");
        }

        _M_maximum_bucket_size = _Newmax;
    }

    // This function is a noop, because the underlying split-ordered list
    // is already sorted, so an increase in the bucket number will be
    // reflected next time this bucket is touched.

    /// <summary>
    ///     Sets the the number of buckets for the container to a specific value.
    /// </summary>
    /// <param name="_Buckets">
    ///     The desired number of buckets.
    /// </param>
    /// <remarks>
    ///     The number of buckets must be a power of 2. If not a power of 2, it will be rounded up to
    ///     the next largest power of 2.
    ///     <para>Throws an <see cref="out_of_range Class">out_of_range</see> exception if the number of buckets 
    ///     is invalid (0 or greater than the maximum number of buckets)</para>
    /// </remarks>
    /**/
    void rehash(size_type _Buckets)
    {
        size_type _Current_buckets = _M_number_of_buckets;

        if (_Current_buckets > _Buckets)
        {
            return;
        }
        else if (_Buckets <= 0 || _Buckets > unsafe_max_bucket_count())
        {
            throw std::out_of_range("invalid number of buckets");
        }
        // Round up the number of buckets to the next largest power of 2
        _M_number_of_buckets = ((size_type) 1) << _Get_msb(_Buckets*2-1);
    }

private:

    // Initialize the hash and keep the first bucket open
    void _Init()
    {
        // Allocate an array of segment pointers
        memset(_M_buckets, 0, _Pointers_per_table * sizeof(void *));

        // Insert the first element in the split-ordered list
        _Full_iterator _Dummy_node = _M_split_ordered_list._Begin();
        _Set_bucket(0, _Dummy_node);
    }

    void _Copy(const _Mytype& _Right)
    {
        clear();

        _M_maximum_bucket_size = _Right._M_maximum_bucket_size;
        _M_number_of_buckets = _Right._M_number_of_buckets;

        try
        {
            insert(_Right.begin(), _Right.end());
            _M_comparator = _Right._M_comparator;
        }
        catch(...)
        {
            _M_split_ordered_list.clear();
            throw;
        }
    }

    void _Swap_buckets(_Concurrent_hash& _Right)
    {
        if (_M_allocator == _Right._M_allocator)
        {
            // Swap all node segments
            for (size_type _Index = 0; _Index < _Pointers_per_table; _Index++)
            {
                _Full_iterator * _Iterator_pointer = _M_buckets[_Index];
                _M_buckets[_Index] = _Right._M_buckets[_Index];
                _Right._M_buckets[_Index] = _Iterator_pointer;
            }
        }
        else
        {
            throw std::invalid_argument("swap is invalid on non-equal allocators");
        }
    }

    // Hash APIs
    size_type _Distance(const_iterator _First, const_iterator _Last) const
    {
        size_type _Num = 0;

        for (const_iterator _Iterator = _First; _Iterator != _Last; _Iterator++)
        {
            _Num++;
        }

        return _Num;
    }

    // Insert an element in the hash given its value
    template<typename _ValTy>
    _Pairib _Insert(_ValTy&& _Value)
    {
        _Split_order_key _Order_key = (_Split_order_key) _M_comparator(_Key_function(_Value));
        size_type _Bucket = _Order_key % _M_number_of_buckets;

        // If bucket is empty, initialize it first
        if (!_Is_initialized(_Bucket))
        {
            _Initialize_bucket(_Bucket);
        }

        long _New_count;
        _Order_key = _Split_order_regular_key(_Order_key);
        _Full_iterator _Iterator = _Get_bucket(_Bucket);
        _Full_iterator _Last = _M_split_ordered_list._End();
        _Full_iterator _Where = _Iterator;
        _Nodeptr _New_node = _M_split_ordered_list._Buynode(_Order_key, std::forward<_ValTy>(_Value));

        _ASSERT_EXPR(_Where != _Last, L"Invalid head node");

        // First node is a dummy node
        _Where++;

        for (;;)
        {
            if (_Where == _Last || _Mylist::_Get_key(_Where) > _Order_key)
            {
                // Try to insert it in the right place
                _Pairib _Result = _M_split_ordered_list._Insert(_Iterator, _Where, _New_node, &_New_count);
                
                if (_Result.second)
                {
                    // Insertion succeeded, adjust the table size, if needed
                    _Adjust_table_size(_New_count, _M_number_of_buckets);
                    return _Result;
                }
                else
                {
                    // Insertion failed: either the same node was inserted by another thread, or
                    // another element was inserted at exactly the same place as this node.
                    // Proceed with the search from the previous location where order key was
                    // known to be larger (note: this is legal only because there is no safe
                    // concurrent erase operation supported).
                    _Where = _Iterator;
                    _Where++;
                    continue;
                }
            }
            else if (!_M_allow_multimapping && _Mylist::_Get_key(_Where) == _Order_key && 
                _M_comparator(_Key_function(*_Where), _Key_function(_New_node->_M_element)) == 0)
            {
                // If the insert failed (element already there), then delete the new one
                _M_split_ordered_list._Erase(_New_node);

                // Element already in the list, return it
                return _Pairib(_M_split_ordered_list._Get_iterator(_Where), false);
            }

            // Move the iterator forward
            _Iterator = _Where;
            _Where++;
        }
    }
    // Find the element in the split-ordered list
    iterator _Find(const key_type& _Keyval)
    {
        _Split_order_key _Order_key = (_Split_order_key) _M_comparator(_Keyval);
        size_type _Bucket = _Order_key % _M_number_of_buckets;

        // If _Bucket is empty, initialize it first
        if (!_Is_initialized(_Bucket))
        {
            _Initialize_bucket(_Bucket);
        }

        _Order_key = _Split_order_regular_key(_Order_key);
        _Full_iterator _Last = _M_split_ordered_list._End();

        for (_Full_iterator _Iterator = _Get_bucket(_Bucket); _Iterator != _Last; _Iterator++)
        {
            if (_Mylist::_Get_key(_Iterator) > _Order_key)
            {
                // If the order key is smaller than the current order key, the element
                // is not in the hash.
                return end();
            }
            else if (_Mylist::_Get_key(_Iterator) == _Order_key)
            {
                // The fact that order keys match does not mean that the element is found.
                // Key function comparison has to be performed to check whether this is the
                // right element. If not, keep searching while order key is the same.
                if (!_M_comparator(_Key_function(*_Iterator), _Keyval))
                {
                    return _M_split_ordered_list._Get_iterator(_Iterator);
                }
            }
        }

        return end();
    }

    // Find the element in the split-ordered list
    const_iterator _Find(const key_type& _Keyval) const
    {
        _Split_order_key _Order_key = (_Split_order_key) _M_comparator(_Keyval);
        size_type _Bucket = _Order_key % _M_number_of_buckets;

        // If _Bucket has not been initialized, keep searching up for a parent bucket
        // that has been initialized.  Worst case is the entire map will be read.
        while (!_Is_initialized(_Bucket))
        {
            _Bucket = _Get_parent(_Bucket);
        }

        _Order_key = _Split_order_regular_key(_Order_key);
        _Full_const_iterator _Last = _M_split_ordered_list._End();

        for (_Full_const_iterator _Iterator = _Get_bucket(_Bucket); _Iterator != _Last; _Iterator++)
        {
            if (_Mylist::_Get_key(_Iterator) > _Order_key)
            {
                // If the order key is smaller than the current order key, the element
                // is not in the hash.
                return end();
            }
            else if (_Mylist::_Get_key(_Iterator) == _Order_key)
            {
                // The fact that order keys match does not mean that the element is found.
                // Key function comparison has to be performed to check whether this is the
                // right element. If not, keep searching while order key is the same.
                if (!_M_comparator(_Key_function(*_Iterator), _Keyval))
                {
                    return _M_split_ordered_list._Get_iterator(_Iterator);
                }
            }
        }

        return end();
    }

    // Erase an element from the list. This is not a concurrency safe function.
    iterator _Erase(const_iterator _Iterator)
    {
        key_type _Keyval = _Key_function(*_Iterator);
        _Split_order_key _Order_key = (_Split_order_key) _M_comparator(_Keyval);
        size_type _Bucket = _Order_key % _M_number_of_buckets;

        // If bucket is empty, initialize it first
        if (!_Is_initialized(_Bucket))
        {
            _Initialize_bucket(_Bucket);
        }

        _Order_key = _Split_order_regular_key(_Order_key);

        _Full_iterator _Previous = _Get_bucket(_Bucket);
        _Full_iterator _Last = _M_split_ordered_list._End();
        _Full_iterator _Where = _Previous;

        _ASSERT_EXPR(_Where != _Last, L"Invalid head node");

        // First node is a dummy node
        _Where++;

        for (;;)
        {
            if (_Where == _Last)
            {
                return end();
            }
            else if (_M_split_ordered_list._Get_iterator(_Where) == _Iterator)
            {
                return _M_split_ordered_list._Erase(_Previous, _Iterator);
            }

            // Move the iterator forward
            _Previous = _Where;
            _Where++;
        }
    }

    // Return the [begin, end) pair of iterators with the same key values.
    // This operation makes sense only if mapping is many-to-one.
    _Pairii _Equal_range(const key_type& _Keyval)
    {
        _Split_order_key _Order_key = (_Split_order_key) _M_comparator(_Keyval);
        size_type _Bucket = _Order_key % _M_number_of_buckets;

        // If _Bucket is empty, initialize it first
        if (!_Is_initialized(_Bucket))
        {
            _Initialize_bucket(_Bucket);
        }

        _Order_key = _Split_order_regular_key(_Order_key);
        _Full_iterator _Last = _M_split_ordered_list._End();

        for (_Full_iterator _Iterator = _Get_bucket(_Bucket); _Iterator != _Last; _Iterator++)
        {
            if (_Mylist::_Get_key(_Iterator) > _Order_key)
            {
                // There is no element with the given key
                return _Pairii(end(), end());
            }
            else if (_Mylist::_Get_key(_Iterator) == _Order_key && !_M_comparator(_Key_function(*_Iterator), _Keyval))
            {
                iterator _Begin = _M_split_ordered_list._Get_iterator(_Iterator);
                iterator _End= _Begin;

                for (;_End != end() && !_M_comparator(_Key_function(*_End), _Keyval); _End++)
                {
                }

                return _Pairii(_Begin, _End);
            }
        }

        return _Pairii(end(), end());
    }

    // Return the [begin, end) pair of const iterators with the same key values.
    // This operation makes sense only if mapping is many-to-one.
    _Paircc _Equal_range(const key_type& _Keyval) const
    {
        _Split_order_key _Order_key = (_Split_order_key) _M_comparator(_Keyval);
        size_type _Bucket = _Order_key % _M_number_of_buckets;

        // If _Bucket has not been initialized, keep searching up for a parent bucket
        // that has been initialized.  Worst case is the entire map will be read.
        while (!_Is_initialized(_Bucket))
        {
            _Bucket = _Get_parent(_Bucket);
        }

        _Order_key = _Split_order_regular_key(_Order_key);
        _Full_const_iterator _Last = _M_split_ordered_list._End();

        for (_Full_const_iterator _Iterator = _Get_bucket(_Bucket); _Iterator != _Last; _Iterator++)
        {
            if (_Mylist::_Get_key(_Iterator) > _Order_key)
            {
                // There is no element with the given key
                return _Paircc(end(), end());
            }
            else if (_Mylist::_Get_key(_Iterator) == _Order_key && !_M_comparator(_Key_function(*_Iterator), _Keyval))
            {
                const_iterator _Begin = _M_split_ordered_list._Get_iterator(_Iterator);
                const_iterator _End = _Begin;

                for (; _End != end() && !_M_comparator(_Key_function(*_End), _Keyval); _End++)
                {
                }

                return _Paircc(_Begin, _End);
            }
        }

        return _Paircc(end(), end());
    }

    // Bucket APIs
    void _Initialize_bucket(size_type _Bucket)
    {
        // Bucket 0 has no parent. Initialize it and return.
        if (_Bucket == 0)
        {
            _Init();
            return;
        }

        size_type _Parent_bucket = _Get_parent(_Bucket);

        // All _Parent_bucket buckets have to be initialized before this bucket is
        if (!_Is_initialized(_Parent_bucket))
        {
            _Initialize_bucket(_Parent_bucket);
        }

        _Full_iterator _Parent = _Get_bucket(_Parent_bucket);

        // Create a dummy first node in this bucket
        _Full_iterator _Dummy_node = _M_split_ordered_list._Insert_dummy(_Parent, _Split_order_dummy_key(_Bucket));
        _Set_bucket(_Bucket, _Dummy_node);
    }

    void _Adjust_table_size(size_type _Total_elements, size_type _Current_size)
    {
        // Grow the table by a factor of 2 if possible and needed
        if (((float) _Total_elements / (float) _Current_size) > _M_maximum_bucket_size)
        {
             // Double the size of the hash only if size has not changed inbetween loads
            _InterlockedCompareExchangeSizeT(&_M_number_of_buckets, 2 * _Current_size, _Current_size);
        }
    }

    size_type _Get_parent(size_type _Bucket) const
    {
        // Unsets bucket's most significant turned-on bit
        unsigned char _Msb = _Get_msb(_Bucket);
        return _Bucket & ~(1 << _Msb);
    }


    // Dynamic sized array (segments)

    _Full_iterator _Get_bucket(size_type _Bucket) const
    {
        size_type _Segment = _Segment_index_of(_Bucket);
        _Bucket -= _Segment_base(_Segment);
        return _M_buckets[_Segment][_Bucket];
    }

    void _Set_bucket(size_type _Bucket, _Full_iterator _Dummy_head)
    {
        size_type _Segment = _Segment_index_of(_Bucket);
        _Bucket -= _Segment_base(_Segment);

        if (_M_buckets[_Segment] == NULL)
        {
            size_type _Seg_size = _Segment_size(_Segment);
            _Full_iterator * _New_segment = _M_allocator.allocate(_Seg_size);
            _Uninitialized_default_fill_n(_New_segment, _Seg_size, (_Full_iterator *) 0, _M_allocator);
            if (_InterlockedCompareExchangePointer((void * volatile *) &_M_buckets[_Segment], _New_segment, NULL) != NULL)
            {
                _M_allocator.deallocate(_New_segment, _Seg_size);
            }
        }
        _M_buckets[_Segment][_Bucket] = _Dummy_head;
    }

    bool _Is_initialized(size_type _Bucket) const
    {
        size_type _Segment = _Segment_index_of(_Bucket);
        _Bucket -= _Segment_base(_Segment);

        if (_M_buckets[_Segment] == NULL)
        {
            return false;
        }

        _Full_iterator _Iterator = _M_buckets[_Segment][_Bucket];
        return (_Iterator._Mynode() != NULL);
    }

    // Utilities for keys

    _Split_order_key _Reverse(_Map_key _Order_key) const
    {
        _Split_order_key _Reversed_order_key;

        unsigned char * _Original = (unsigned char *) &_Order_key;
        unsigned char * _Reversed = (unsigned char *) &_Reversed_order_key;

        int _Size = sizeof(_Map_key);
        for (int _Index = 0; _Index < _Size; _Index++)
        {
            _Reversed[_Size - _Index - 1] = _Reverse_byte(_Original[_Index]);
        }

        return _Reversed_order_key;
    }

    // A regular order key has its original hash value reversed and the last bit set
    _Split_order_key _Split_order_regular_key(_Map_key _Order_key) const
    {
        return _Reverse(_Order_key) | 0x1;
    }

    // A dummy order key has its original hash value reversed and the last bit unset
    _Split_order_key _Split_order_dummy_key(_Map_key _Order_key) const
    {
        return _Reverse(_Order_key) & ~(0x1);
    }

    // Shared variables
    _Full_iterator *                                                _M_buckets[_Pointers_per_table]; // The segment table
    _Mylist                                                         _M_split_ordered_list;           // List where all the elements are kept
    typename allocator_type::template rebind<_Full_iterator>::other _M_allocator;                    // Allocator object for segments
    size_type                                                       _M_number_of_buckets;            // Current table size
    float                                                           _M_maximum_bucket_size;          // Maximum size of the bucket
};

#pragma warning(pop) // warning 4127 -- while (true) has a constant expression in it

} // namespace details;
} // namespace Concurrency
