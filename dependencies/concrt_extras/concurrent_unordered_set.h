/***
* ==++==
*
* Copyright (c) Microsoft Corporation.  All rights reserved.
* 
* ==--==
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* concurrent_unordered_set.h
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/
#pragma once

#include <utility>
#include "internal_concurrent_hash.h"

#if !(defined(_M_AMD64) || defined(_M_IX86))
    #error ERROR: Concurrency Runtime is supported only on X64 and X86 architectures.
#endif

#if defined(_M_CEE)
    #error ERROR: Concurrency Runtime is not supported when compiling /clr.
#endif

#pragma pack(push,_CRT_PACKING)

namespace Concurrency
{
namespace samples
{
namespace details
{
// Template class for hash set traits
template<typename _Key_type, typename _Key_comparator, typename _Allocator_type, bool _Allow_multimapping>
class _Concurrent_unordered_set_traits : public std::_Container_base
{
public:
    typedef _Key_type _Value_type;
    typedef _Key_type value_type;
    typedef _Key_type key_type;
    typedef _Key_comparator key_compare;

    typedef typename _Allocator_type::template rebind<value_type>::other allocator_type;

    enum
    {
        _M_allow_multimapping = _Allow_multimapping
    };

    _Concurrent_unordered_set_traits() : _M_comparator()
    {
    }

    _Concurrent_unordered_set_traits(const _Key_comparator& _Traits) : _M_comparator(_Traits)
    {
    }

    typedef key_compare value_compare;

    static const _Key_type& _Key_function(const value_type& _Value)
    {
        return _Value;
    }

    _Key_comparator _M_comparator; // the comparator predicate for keys
};
} // namespace details;

/// <summary>
///     The <c>concurrent_unordered_set</c> class is an concurrency-safe container that controls a varying-length sequence of 
///     elements of type _Key_type The sequence is represented in a way that enables concurrency-safe append, element access, 
///     iterator access and iterator traversal operations.
/// </summary>
/// <typeparam name="_Key_type">
///     The key type.
/// </typeparam>
/// <typeparam name="_Hasher">
///     The hash function object type. This argument is optional and the default value is
///     tr1::hash&lt;</c><typeparamref name="_Key_type"/><c>&gt;</c>.
/// </typeparam>
/// <typeparam name="_Key_equality">
///     The equality comparison function object type. This argument is optional and the default value is
///     <c>equal_to&lt;</c><typeparamref name="_Key_type"/><c>&gt;</c>.
/// </typeparam>
/// <typeparam name="_Allocator_type">
///     The type that represents the stored allocator object that encapsulates details about the allocation and
///     deallocation of memory for the concurrent vector. This argument is optional and the default value is
///     <c>allocator&lt;</c><typeparamref name="_Key_type"/>, <typeparamref name="_Element_type"/><c>&gt;</c>.
/// </typeparam>
/// <remarks>
///     For detailed information on the <c>concurrent_unordered_set</c> class, see <see cref="Parallel Containers and Objects"/>.
/// </remarks>
/// <seealso cref="Parallel Containers and Objects"/>
/**/
template <typename _Key_type, typename _Hasher = std::tr1::hash<_Key_type>, typename _Key_equality = std::equal_to<_Key_type>, typename _Allocator_type = std::allocator<_Key_type> >
class concurrent_unordered_set : public details::_Concurrent_hash< details::_Concurrent_unordered_set_traits<_Key_type, details::_Hash_compare<_Key_type, _Hasher, _Key_equality>, _Allocator_type, false> >
{
public:
    // Base type definitions
    typedef concurrent_unordered_set<_Key_type, _Hasher, _Key_equality, _Allocator_type> _Mytype;
    typedef details::_Hash_compare<_Key_type, _Hasher, _Key_equality> _Mytraits;
    typedef details::_Concurrent_hash< details::_Concurrent_unordered_set_traits<_Key_type, _Mytraits, _Allocator_type, false> > _Mybase;

    // Type definitions
    typedef _Key_type key_type;
    typedef typename _Mybase::value_type value_type;
    typedef _Key_type mapped_type;
    typedef _Hasher hasher;
    typedef _Key_equality key_equal;
    typedef _Mytraits key_compare;

    typedef typename _Mybase::allocator_type allocator_type;
    typedef typename _Mybase::pointer pointer;
    typedef typename _Mybase::const_pointer const_pointer;
    typedef typename _Mybase::reference reference;
    typedef typename _Mybase::const_reference const_reference;

    typedef typename _Mybase::size_type size_type;
    typedef typename _Mybase::difference_type difference_type;

    typedef typename _Mybase::iterator iterator;
    typedef typename _Mybase::const_iterator const_iterator;
    typedef typename _Mybase::iterator local_iterator;
    typedef typename _Mybase::const_iterator const_local_iterator;

    /// <summary>
    ///     Constructs a concurrent unordered set.
    /// </summary>
    /// <param name="_Number_of_buckets">
    ///     The initial number of buckets for this unordered set.
    /// </param>
    /// <param name="_Hasher">
    ///     The hash function for this unordered set.
    /// </param>
    /// <param name="_Key_equality">
    ///     The equality comparison function for this unordered set.
    /// </param>
    /// <param name="_Allocator">
    ///     The allocator for this unordered set.
    /// </param>
    /// <remarks>
    ///     All constructors store an allocator object <paramref name="_Allocator"/> and initialize the unordered set.
    ///     <para>The first constructor specifies an empty initial set and explicitly specifies the number of buckets,
    ///     hash function, equality function and allocator type to be used.</para>
    ///     <para>The second constructor specifies an allocator for the unordered set.<para>
    ///     <para>The third constructor specifies values supplied by the iterator range [<paramref name="_Begin"/>, <paramref name="_End"/>).</para>
    ///     <para>The fourth and fifth constructors specify a copy of the concurrent unordered set <paramref name="_Uset"/>.</para>
    ///     <para>The last constructor specifies a move of the concurrent unordered set <paramref name="_Uset"/>.</para>
    /// </remarks>
    /**/
    explicit concurrent_unordered_set(size_type _Number_of_buckets = 8, const hasher& _Hasher = hasher(), const key_equal& _Key_equality = key_equal(),
        const allocator_type& _Allocator = allocator_type())
        : _Mybase(_Number_of_buckets, key_compare(_Hasher, _Key_equality), _Allocator)
    {
        this->rehash(_Number_of_buckets);
    }

    /// <summary>
    ///     Constructs a concurrent unordered set.
    /// </summary>
    /// <param name="_Allocator">
    ///     The allocator for this unordered set.
    /// </param>
    /// <remarks>
    ///     All constructors store an allocator object <paramref name="_Allocator"/> and initialize the unordered set.
    ///     <para>The first constructor specifies an empty initial set and explicitly specifies the number of buckets,
    ///     hash function, equality function and allocator type to be used.</para>
    ///     <para>The second constructor specifies an allocator for the unordered set.<para>
    ///     <para>The third constructor specifies values supplied by the iterator range [<paramref name="_Begin"/>, <paramref name="_End"/>).</para>
    ///     <para>The fourth and fifth constructors specify a copy of the concurrent unordered set <paramref name="_Uset"/>.</para>
    ///     <para>The last constructor specifies a move of the concurrent unordered set <paramref name="_Uset"/>.</para>
    /// </remarks>
    /**/
    concurrent_unordered_set(const allocator_type& _Allocator) : _Mybase(8, key_compare(), _Allocator)
    {
    }

    /// <summary>
    ///     Constructs a concurrent unordered set.
    /// </summary>
    /// <typeparam name="_Iterator">
    ///     The type of the input iterator.
    /// </typeparam>
    /// <param name="_Begin">
    ///     Position of the first element in the range of elements to be copied.
    /// </param>
    /// <param name="_End">
    ///     Position of the first element beyond the range of elements to be copied.
    /// </param>
    /// <param name="_Number_of_buckets">
    ///     The initial number of buckets for this unordered set.
    /// </param>
    /// <param name="_Hasher">
    ///     The hash function for this unordered set.
    /// </param>
    /// <param name="_Key_equality">
    ///     The equality comparison function for this unordered set.
    /// </param>
    /// <param name="_Allocator">
    ///     The allocator for this unordered set.
    /// </param>
    /// <remarks>
    ///     All constructors store an allocator object <paramref name="_Allocator"/> and initialize the unordered set.
    ///     <para>The first constructor specifies an empty initial set and explicitly specifies the number of buckets,
    ///     hash function, equality function and allocator type to be used.</para>
    ///     <para>The second constructor specifies an allocator for the unordered set.<para>
    ///     <para>The third constructor specifies values supplied by the iterator range [<paramref name="_Begin"/>, <paramref name="_End"/>).</para>
    ///     <para>The fourth and fifth constructors specify a copy of the concurrent unordered set <paramref name="_Uset"/>.</para>
    ///     <para>The last constructor specifies a move of the concurrent unordered set <paramref name="_Uset"/>.</para>
    /// </remarks>
    /**/
    template <typename _Iterator>
    concurrent_unordered_set(_Iterator _First, _Iterator _Last, size_type _Number_of_buckets = 8, const hasher& _Hasher = hasher(),
        const key_equal& _Key_equality = key_equal(), const allocator_type& _Allocator = allocator_type())
        : _Mybase(_Number_of_buckets, key_compare(), allocator_type())
    {
        this->rehash(_Number_of_buckets);
        for (; _First != _Last; ++_First)
        {
            _Mybase::insert(*_First);
        }
    }

    /// <summary>
    ///     Constructs a concurrent unordered set.
    /// </summary>
    /// <param name="_Uset">
    ///     The source <c>concurrent_unordered_set</c> object to copy or move elements from.
    /// </param>
    /// <remarks>
    ///     All constructors store an allocator object <paramref name="_Allocator"/> and initialize the unordered set.
    ///     <para>The first constructor specifies an empty initial set and explicitly specifies the number of buckets,
    ///     hash function, equality function and allocator type to be used.</para>
    ///     <para>The second constructor specifies an allocator for the unordered set.<para>
    ///     <para>The third constructor specifies values supplied by the iterator range [<paramref name="_Begin"/>, <paramref name="_End"/>).</para>
    ///     <para>The fourth and fifth constructors specify a copy of the concurrent unordered set <paramref name="_Uset"/>.</para>
    ///     <para>The last constructor specifies a move of the concurrent unordered set <paramref name="_Uset"/>.</para>
    /// </remarks>
    /**/
    concurrent_unordered_set(const concurrent_unordered_set& _Uset) : _Mybase(_Uset)
    {
    }

    /// <summary>
    ///     Constructs a concurrent unordered set.
    /// </summary>
    /// <param name="_Uset">
    ///     The source <c>concurrent_unordered_map</c> object to copy or move elements from.
    /// </param>
    /// <param name="_Allocator">
    ///     The allocator for this unordered set.
    /// </param>
    /// <remarks>
    ///     All constructors store an allocator object <paramref name="_Allocator"/> and initialize the unordered set.
    ///     <para>The first constructor specifies an empty initial set and explicitly specifies the number of buckets,
    ///     hash function, equality function and allocator type to be used.</para>
    ///     <para>The second constructor specifies an allocator for the unordered set.<para>
    ///     <para>The third constructor specifies values supplied by the iterator range [<paramref name="_Begin"/>, <paramref name="_End"/>).</para>
    ///     <para>The fourth and fifth constructors specify a copy of the concurrent unordered set <paramref name="_Uset"/>.</para>
    ///     <para>The last constructor specifies a move of the concurrent unordered set <paramref name="_Uset"/>.</para>
    /// </remarks>
    /**/
    concurrent_unordered_set(const concurrent_unordered_set& _Uset, const allocator_type& _Allocator) : _Mybase(_Uset, _Allocator)
    {
    }

    /// <summary>
    ///     Constructs a concurrent unordered set.
    /// </summary>
    /// <param name="_Uset">
    ///     The source <c>concurrent_unordered_set</c> object to copy or move elements from.
    /// </param>
    /// <remarks>
    ///     All constructors store an allocator object <paramref name="_Allocator"/> and initialize the unordered set.
    ///     <para>The first constructor specifies an empty initial set and explicitly specifies the number of buckets,
    ///     hash function, equality function and allocator type to be used.</para>
    ///     <para>The second constructor specifies an allocator for the unordered set.<para>
    ///     <para>The third constructor specifies values supplied by the iterator range [<paramref name="_Begin"/>, <paramref name="_End"/>).</para>
    ///     <para>The fourth and fifth constructors specify a copy of the concurrent unordered set <paramref name="_Uset"/>.</para>
    ///     <para>The last constructor specifies a move of the concurrent unordered set <paramref name="_Uset"/>.</para>
    /// </remarks>
    /**/
    concurrent_unordered_set(concurrent_unordered_set&& _Uset) : _Mybase(std::move(_Uset))
    {
    }

    /// <summary>
    ///     Assigns the contents of another <c>concurrent_unordered_set</c> object to this one. This method is not concurrency-safe.
    /// </summary>
    /// <param name="_Uset">
    ///     The source <c>concurrent_unordered_set</c> object.
    /// </param>
    /// <returns>
    ///     A reference to this <c>concurrent_unordered_set</c> object.
    /// </returns>
    /// <remarks>
    ///     After erasing any existing elements in a concurrent unordered set, <c>operator=</c> either copies or moves the contents of
    ///     <paramref name="_Uset"/> into the concurrent unordered set.
    /// </remarks>
    /**/
    concurrent_unordered_set& operator=(const concurrent_unordered_set& _Uset)
    {
        _Mybase::operator=(_Uset);
        return (*this);
    }

    /// <summary>
    ///     Assigns the contents of another <c>concurrent_unordered_set</c> object to this one. This method is not concurrency-safe.
    /// </summary>
    /// <param name="_Uset">
    ///     The source <c>concurrent_unordered_set</c> object.
    /// </param>
    /// <returns>
    ///     A reference to this <c>concurrent_unordered_set</c> object.
    /// </returns>
    /// <remarks>
    ///     After erasing any existing elements in a concurrent unordered set, <c>operator=</c> either copies or moves the contents of
    ///     <paramref name="_Uset"/> into the concurrent unordered set.
    /// </remarks>
    /**/
    concurrent_unordered_set& operator=(concurrent_unordered_set&& _Uset)
    {
        _Mybase::operator=(std::move(_Uset));
        return (*this);
    }

    /// <summary>
    ///     Erases elements from the <c>concurrent_unordered_set</c>. This method is not concurrency-safe.
    /// </summary>
    /// <param name="_Where">
    ///     The iterator position to erase from.
    /// </param>
    /// <remarks>
    ///     The first function erases an element from the set given an iterator position.
    ///     <para>The second function erases an element matching a key</para>
    ///     <para>The third function erases elements given an iterator begin and end position</para>
    /// </remarks>
    /// <returns>
    ///     The iterator for this <c>concurrent_unordered_set</c> object.
    /// </returns>
    /**/
    iterator unsafe_erase(const_iterator _Where)
    {
        return _Mybase::unsafe_erase(_Where);
    }

    /// <summary>
    ///     Erases elements from the <c>concurrent_unordered_set</c>. This method is not concurrency-safe.
    /// </summary>
    /// <param name="_Keyval">
    ///     The key to erase.
    /// </param>
    /// <remarks>
    ///     The first function erases an element from the set given an iterator position.
    ///     <para>The second function erases an element matching a key</para>
    ///     <para>The third function erases elements given an iterator begin and end position</para>
    /// </remarks>
    /// <returns>
    ///     The count of elements erased from this <c>concurrent_unordered_set</c> object.
    /// </returns>
    /**/
    size_type unsafe_erase(const key_type& _Keyval)
    {
        return _Mybase::unsafe_erase(_Keyval);
    }

    /// <summary>
    ///     Erases elements from the <c>concurrent_unordered_set</c>. This method is not concurrency-safe.
    /// </summary>
    /// <param name="_Begin">
    ///     Position of the first element in the range of elements to be erased.
    /// </param>
    /// <param name="_End">
    ///     Position of the first element beyond the range of elements to be erased.
    /// </param>
    /// <remarks>
    ///     The first function erases an element from the set given an iterator position.
    ///     <para>The second function erases an element matching a key</para>
    ///     <para>The third function erases elements given an iterator begin and end position</para>
    /// </remarks>
    /// <returns>
    ///     The iterator for this <c>concurrent_unordered_set</c> object.
    /// </returns>
    /**/
    iterator unsafe_erase(const_iterator _First, const_iterator _Last)
    {
        return _Mybase::unsafe_erase(_First, _Last);
    }

    /// <summary>
    ///     Swaps the contents of two <c>concurrent_unordered_set</c> objects. 
    ///     This method is not concurrency-safe.
    /// </summary>
    /// <param name="_Uset">
    ///     The <c>concurrent_unordered_set</c> object to swap with.
    /// </param>
    /**/
    void swap(concurrent_unordered_set& _Uset)
    {
        _Mybase::swap(_Uset);
    }

    // Observers
    /// <summary>
    ///     The hash function object.
    /// </summary>
    /**/
    hasher hash_function() const
    {
        return _M_comparator._M_hash_object;
    }

    /// <summary>
    ///     The equality comparison function object.
    /// </summary>
    /**/
    key_equal key_eq() const
    {
        return _M_comparator._M_key_compare_object;
    }
};

/// <summary>
///     The <c>concurrent_unordered_multiset</c> class is an concurrency-safe container that controls a varying-length sequence of 
///     elements of type _Key_type The sequence is represented in a way that enables concurrency-safe append, element access, 
///     iterator access and iterator traversal operations.
/// </summary>
/// <typeparam name="_Key_type">
///     The key type.
/// </typeparam>
/// <typeparam name="_Hasher">
///     The hash function object type. This argument is optional and the default value is
///     tr1::hash&lt;</c><typeparamref name="_Key_type"/><c>&gt;</c>.
/// </typeparam>
/// <typeparam name="_Key_equality">
///     The equality comparison function object type. This argument is optional and the default value is
///     <c>equal_to&lt;</c><typeparamref name="_Key_type"/><c>&gt;</c>.
/// </typeparam>
/// <typeparam name="_Allocator_type">
///     The type that represents the stored allocator object that encapsulates details about the allocation and
///     deallocation of memory for the concurrent vector. This argument is optional and the default value is
///     <c>allocator&lt;</c><typeparamref name="_Key_type"/>, <typeparamref name="_Element_type"/><c>&gt;</c>.
/// </typeparam>
/// <remarks>
///     For detailed information on the <c>concurrent_unordered_multiset</c> class, see <see cref="Parallel Containers and Objects"/>.
/// </remarks>
/// <seealso cref="Parallel Containers and Objects"/>
/**/
template <typename _Key_type, typename _Hasher = std::tr1::hash<_Key_type>, typename _Key_equality = std::equal_to<_Key_type>, typename _Allocator_type = std::allocator<_Key_type> >
class concurrent_unordered_multiset : public details::_Concurrent_hash< details::_Concurrent_unordered_set_traits<_Key_type, details::_Hash_compare<_Key_type, _Hasher, _Key_equality>, _Allocator_type, true> >
{
public:
    // Base type definitions
    typedef concurrent_unordered_multiset<_Key_type, _Hasher, _Key_equality, _Allocator_type> _Mytype;
    typedef details::_Hash_compare<_Key_type, _Hasher, _Key_equality> _Mytraits;
    typedef details::_Concurrent_hash< details::_Concurrent_unordered_set_traits<_Key_type, _Mytraits, _Allocator_type, true> > _Mybase;

    // Type definitions
    typedef _Key_type key_type;
    typedef typename _Mybase::value_type value_type;
    typedef _Key_type mapped_type;
    typedef _Hasher hasher;
    typedef _Key_equality key_equal;
    typedef _Mytraits key_compare;

    typedef typename _Mybase::allocator_type allocator_type;
    typedef typename _Mybase::pointer pointer;
    typedef typename _Mybase::const_pointer const_pointer;
    typedef typename _Mybase::reference reference;
    typedef typename _Mybase::const_reference const_reference;

    typedef typename _Mybase::size_type size_type;
    typedef typename _Mybase::difference_type difference_type;

    typedef typename _Mybase::iterator iterator;
    typedef typename _Mybase::const_iterator const_iterator;
    typedef typename _Mybase::iterator local_iterator;
    typedef typename _Mybase::const_iterator const_local_iterator;

    /// <summary>
    ///     Constructs a concurrent unordered multiset.
    /// </summary>
    /// <param name="_Number_of_buckets">
    ///     The initial number of buckets for this unordered multiset.
    /// </param>
    /// <param name="_Hasher">
    ///     The hash function for this unordered multiset.
    /// </param>
    /// <param name="_Key_equality">
    ///     The equality comparison function for this unordered multiset.
    /// </param>
    /// <param name="_Allocator">
    ///     The allocator for this unordered multiset.
    /// </param>
    /// <remarks>
    ///     All constructors store an allocator object <paramref name="_Allocator"/> and initialize the unordered multiset.
    ///     <para>The first constructor specifies an empty initial multiset and explicitly specifies the number of buckets,
    ///     hash function, equality function and allocator type to be used.</para>
    ///     <para>The second constructor specifies an allocator for the unordered multiset.<para>
    ///     <para>The third constructor specifies values supplied by the iterator range [<paramref name="_Begin"/>, <paramref name="_End"/>).</para>
    ///     <para>The fourth and fifth constructors specify a copy of the concurrent unordered multiset <paramref name="_Uset"/>.</para>
    ///     <para>The last constructor specifies a move of the concurrent unordered multiset <paramref name="_Uset"/>.</para>
    /// </remarks>
    /**/
    explicit concurrent_unordered_multiset(size_type _Number_of_buckets = 8, const hasher& _Hasher = hasher(), const key_equal& _Key_equality = key_equal(),
        const allocator_type& _Allocator = allocator_type())
        : _Mybase(_Number_of_buckets, key_compare(_Hasher, _Key_equality), _Allocator)
    {
        this->rehash(_Number_of_buckets);
    }

    /// <summary>
    ///     Constructs a concurrent unordered multiset.
    /// </summary>
    /// <param name="_Allocator">
    ///     The allocator for this unordered multiset.
    /// </param>
    /// <remarks>
    ///     All constructors store an allocator object <paramref name="_Allocator"/> and initialize the unordered multiset.
    ///     <para>The first constructor specifies an empty initial multiset and explicitly specifies the number of buckets,
    ///     hash function, equality function and allocator type to be used.</para>
    ///     <para>The second constructor specifies an allocator for the unordered multiset.<para>
    ///     <para>The third constructor specifies values supplied by the iterator range [<paramref name="_Begin"/>, <paramref name="_End"/>).</para>
    ///     <para>The fourth and fifth constructors specify a copy of the concurrent unordered multiset <paramref name="_Uset"/>.</para>
    ///     <para>The last constructor specifies a move of the concurrent unordered multiset <paramref name="_Uset"/>.</para>
    /// </remarks>
    /**/
    concurrent_unordered_multiset(const allocator_type& _Allocator) : _Mybase(8, key_compare(), _Allocator)
    {
    }

    /// <summary>
    ///     Constructs a concurrent unordered multiset.
    /// </summary>
    /// <typeparam name="_Iterator">
    ///     The type of the input iterator.
    /// </typeparam>
    /// <param name="_Begin">
    ///     Position of the first element in the range of elements to be copied.
    /// </param>
    /// <param name="_End">
    ///     Position of the first element beyond the range of elements to be copied.
    /// </param>
    /// <param name="_Number_of_buckets">
    ///     The initial number of buckets for this unordered multiset.
    /// </param>
    /// <param name="_Hasher">
    ///     The hash function for this unordered multiset.
    /// </param>
    /// <param name="_Key_equality">
    ///     The equality comparison function for this unordered multiset.
    /// </param>
    /// <param name="_Allocator">
    ///     The allocator for this unordered multiset.
    /// </param>
    /// <remarks>
    ///     All constructors store an allocator object <paramref name="_Allocator"/> and initialize the unordered multiset.
    ///     <para>The first constructor specifies an empty initial multiset and explicitly specifies the number of buckets,
    ///     hash function, equality function and allocator type to be used.</para>
    ///     <para>The second constructor specifies an allocator for the unordered multiset.<para>
    ///     <para>The third constructor specifies values supplied by the iterator range [<paramref name="_Begin"/>, <paramref name="_End"/>).</para>
    ///     <para>The fourth and fifth constructors specify a copy of the concurrent unordered multiset <paramref name="_Uset"/>.</para>
    ///     <para>The last constructor specifies a move of the concurrent unordered multiset <paramref name="_Uset"/>.</para>
    /// </remarks>
    /**/
    template <typename _Iterator>
    concurrent_unordered_multiset(_Iterator _First, _Iterator _Last, size_type _Number_of_buckets = 8, const hasher& _Hasher = hasher(),
        const key_equal& _Key_equality = key_equal(), const allocator_type& _Allocator = allocator_type())
        : _Mybase(_Number_of_buckets, key_compare(), allocator_type())
    {
        this->rehash(_Number_of_buckets);
        for (; _First != _Last; ++_First)
        {
            _Mybase::insert(*_First);
        }
    }

    /// <summary>
    ///     Constructs a concurrent unordered multiset.
    /// </summary>
    /// <param name="_Uset">
    ///     The source <c>concurrent_unordered_multiset</c> object to copy elements from.
    /// </param>
    /// <remarks>
    ///     All constructors store an allocator object <paramref name="_Allocator"/> and initialize the unordered multiset.
    ///     <para>The first constructor specifies an empty initial multiset and explicitly specifies the number of buckets,
    ///     hash function, equality function and allocator type to be used.</para>
    ///     <para>The second constructor specifies an allocator for the unordered multiset.<para>
    ///     <para>The third constructor specifies values supplied by the iterator range [<paramref name="_Begin"/>, <paramref name="_End"/>).</para>
    ///     <para>The fourth and fifth constructors specify a copy of the concurrent unordered multiset <paramref name="_Uset"/>.</para>
    ///     <para>The last constructor specifies a move of the concurrent unordered multiset <paramref name="_Uset"/>.</para>
    /// </remarks>
    /**/
    concurrent_unordered_multiset(const concurrent_unordered_multiset& _Uset) : _Mybase(_Uset)
    {
    }

    /// <summary>
    ///     Constructs a concurrent unordered multiset.
    /// </summary>
    /// <param name="_Uset">
    ///     The source <c>concurrent_unordered_multiset</c> object to copy elements from.
    /// </param>
    /// <param name="_Allocator">
    ///     The allocator for this unordered multiset.
    /// </param>
    /// <remarks>
    ///     All constructors store an allocator object <paramref name="_Allocator"/> and initialize the unordered multiset.
    ///     <para>The first constructor specifies an empty initial multiset and explicitly specifies the number of buckets,
    ///     hash function, equality function and allocator type to be used.</para>
    ///     <para>The second constructor specifies an allocator for the unordered multiset.<para>
    ///     <para>The third constructor specifies values supplied by the iterator range [<paramref name="_Begin"/>, <paramref name="_End"/>).</para>
    ///     <para>The fourth and fifth constructors specify a copy of the concurrent unordered multiset <paramref name="_Uset"/>.</para>
    ///     <para>The last constructor specifies a move of the concurrent unordered multiset <paramref name="_Uset"/>.</para>
    /// </remarks>
    /**/
    concurrent_unordered_multiset(const concurrent_unordered_multiset& _Uset, const allocator_type& _Allocator) : _Mybase(_Uset, _Allocator)
    {
    }

    /// <summary>
    ///     Constructs a concurrent unordered multiset.
    /// </summary>
    /// <param name="_Uset">
    ///     The source <c>concurrent_unordered_multiset</c> object to move elements from.
    /// </param>
    /// <remarks>
    ///     All constructors store an allocator object <paramref name="_Allocator"/> and initialize the unordered multiset.
    ///     <para>The first constructor specifies an empty initial multiset and explicitly specifies the number of buckets,
    ///     hash function, equality function and allocator type to be used.</para>
    ///     <para>The second constructor specifies an allocator for the unordered multiset.<para>
    ///     <para>The third constructor specifies values supplied by the iterator range [<paramref name="_Begin"/>, <paramref name="_End"/>).</para>
    ///     <para>The fourth and fifth constructors specify a copy of the concurrent unordered multiset <paramref name="_Uset"/>.</para>
    ///     <para>The last constructor specifies a move of the concurrent unordered multiset <paramref name="_Uset"/>.</para>
    /// </remarks>
    /**/
    concurrent_unordered_multiset(concurrent_unordered_multiset&& _Uset) : _Mybase(std::move(_Uset))
    {
    }

    /// <summary>
    ///     Assigns the contents of another <c>concurrent_unordered_multiset</c> object to this one. This method is not concurrency-safe.
    /// </summary>
    /// <param name="_Uset">
    ///     The source <c>concurrent_unordered_multiset</c> object.
    /// </param>
    /// <returns>
    ///     A reference to this <c>concurrent_unordered_multiset</c> object.
    /// </returns>
    /// <remarks>
    ///     After erasing any existing elements in a concurrent unordered multiset, <c>operator=</c> either copies or moves the contents of 
    ///     <paramref name="_Uset"/> into the concurrent unordered multiset.
    /// </remarks>
    /**/
    concurrent_unordered_multiset& operator=(const concurrent_unordered_multiset& _Uset)
    {
        _Mybase::operator=(_Uset);
        return (*this);
    }

    /// <summary>
    ///     Assigns the contents of another <c>concurrent_unordered_multiset</c> object to this one. This method is not concurrency-safe.
    /// </summary>
    /// <param name="_Uset">
    ///     The source <c>concurrent_unordered_multiset</c> object.
    /// </param>
    /// <returns>
    ///     A reference to this <c>concurrent_unordered_multiset</c> object.
    /// </returns>
    /// <remarks>
    ///     After erasing any existing elements in a concurrent unordered multiset, <c>operator=</c> either copies or moves the contents of 
    ///     <paramref name="_Uset"/> into the concurrent unordered multiset.
    /// </remarks>
    /**/
    concurrent_unordered_multiset& operator=(concurrent_unordered_multiset&& _Uset)
    {
        _Mybase::operator=(std::move(_Uset));
        return (*this);
    }

    // Modifiers
    /// <summary>
    ///     Inserts a value into the <c>concurrent_unordered_multiset</c> object. 
    /// </summary>
    /// <param name="_Value">
    ///     The value to be inserted into the <c>concurrent_unordered_multiset</c> object.
    /// </param>
    /// <remarks>
    ///     The first function determines whether an element X exists in the sequence whose key has equivalent ordering to 
    ///     that of _Value. If not, it creates such an element X and initializes it with _Value. The function then determines the 
    ///     iterator where that designates X. If an insertion occurred, the function returns std::pair(where, true). Otherwise, 
    ///     it returns std::pair(where, false).
    ///     <para>The second function uses the <c>const_iterator _Where</c> as a starting location to search for an insertion point</para>
    ///     <para>The third function inserts the sequence of element values, from the range [_First, _Last).</para>
    ///     <para>The last two functions behave the same as the first two, except that <paramref name="_Value"/> is used to construct the inserted value.
    /// </remarks>
    /// <returns>
    ///     An iterator pointing to the insertion location.
    /// </returns>
    /**/
    iterator insert(const value_type& _Value)
    {
        return (_Mybase::insert(_Value)).first;
    }

    /// <summary>
    ///     Inserts a value into the <c>concurrent_unordered_multiset</c> object. 
    /// </summary>
    /// <param name="_Where">
    ///     The starting location to search for an insertion point into the <c>concurrent_unordered_multiset</c> object.
    /// </param>
    /// <param name="_Value">
    ///     The value to be inserted into the <c>concurrent_unordered_multiset</c> object.
    /// </param>
    /// <remarks>
    ///     The first function determines whether an element X exists in the sequence whose key has equivalent ordering to 
    ///     that of _Value. If not, it creates such an element X and initializes it with _Value. The function then determines the 
    ///     iterator where that designates X. If an insertion occurred, the function returns std::pair(where, true). Otherwise, 
    ///     it returns std::pair(where, false).
    ///     <para>The second function uses the <c>const_iterator _Where</c> as a starting location to search for an insertion point</para>
    ///     <para>The third function inserts the sequence of element values, from the range [_First, _Last).</para>
    ///     <para>The last two functions behave the same as the first two, except that <paramref name="_Value"/> is used to construct the inserted value.
    /// </remarks>
    /// <returns>
    ///     The iterator for the <c>concurrent_unordered_multiset</c> object.
    /// </returns>
    iterator insert(const_iterator _Where, const value_type& _Value)
    {
        return _Mybase::insert(_Where, _Value);
    }

    /// <summary>
    ///     Inserts values into the <c>concurrent_unordered_multiset</c> object. 
    /// </summary>
    /// <typeparam name="_Iterator">
    ///     The iterator type used for insertion.
    /// </typeparm>
    /// <param name="_First">
    ///     The starting location in an itertor of elements to insert into the <c>concurrent_unordered_multiset</c> object.
    /// </param>
    /// <param name="_Last">
    ///     The ending location in an itertor of elements to insert into the <c>concurrent_unordered_multiset</c> object.
    /// </param>
    /// <remarks>
    ///     The first function determines whether an element X exists in the sequence whose key has equivalent ordering to 
    ///     that of _Value. If not, it creates such an element X and initializes it with _Value. The function then determines the 
    ///     iterator where that designates X. If an insertion occurred, the function returns std::pair(where, true). Otherwise, 
    ///     it returns std::pair(where, false).
    ///     <para>The second function uses the <c>const_iterator _Where</c> as a starting location to search for an insertion point</para>
    ///     <para>The third function inserts the sequence of element values, from the range [_First, _Last).</para>
    ///     <para>The last two functions behave the same as the first two, except that <paramref name="_Value"/> is used to construct the inserted value.
    /// </remarks>
    template<class _Iterator>
    void insert(_Iterator _First, _Iterator _Last)
    {
        _Mybase::insert(_First, _Last);
    }

    /// <summary>
    ///     Inserts a value into the <c>concurrent_unordered_multiset</c> object. 
    /// </summary>
    /// <typeparam name="_Valty">
    ///     The type of the value inserted into the map.
    /// </typeparm>
    /// <param name="_Value">
    ///     The value to be inserted into the <c>concurrent_unordered_multiset</c> object.
    /// </param>
    /// <remarks>
    ///     The first function determines whether an element X exists in the sequence whose key has equivalent ordering to 
    ///     that of _Value. If not, it creates such an element X and initializes it with _Value. The function then determines the 
    ///     iterator where that designates X. If an insertion occurred, the function returns std::pair(where, true). Otherwise, 
    ///     it returns std::pair(where, false).
    ///     <para>The second function uses the <c>const_iterator _Where</c> as a starting location to search for an insertion point</para>
    ///     <para>The third function inserts the sequence of element values, from the range [_First, _Last).</para>
    ///     <para>The last two functions behave the same as the first two, except that <paramref name="_Value"/> is used to construct the inserted value.
    /// </remarks>
    /// <returns>
    ///     An iterator pointing to the insertion location.
    /// </returns>
    /**/
    template<class _Valty>
    iterator insert(_Valty&& _Value)
    {
        return (_Mybase::insert(std::forward<_Valty>(_Value))).first;
    }

    /// <summary>
    ///     Inserts a value into the <c>concurrent_unordered_multiset</c> object. 
    /// </summary>
    /// <typeparam name="_Valty">
    ///     The type of the value inserted into the map.
    /// </typeparm>
    /// <param name="_Where">
    ///     The starting location to search for an insertion point into the <c>concurrent_unordered_multiset</c> object.
    /// </param>
    /// <param name="_Value">
    ///     The value to be inserted into the <c>concurrent_unordered_multiset</c> object.
    /// </param>
    /// <remarks>
    ///     The first function determines whether an element X exists in the sequence whose key has equivalent ordering to 
    ///     that of _Value. If not, it creates such an element X and initializes it with _Value. The function then determines the 
    ///     iterator where that designates X. If an insertion occurred, the function returns std::pair(where, true). Otherwise, 
    ///     it returns std::pair(where, false).
    ///     <para>The second function uses the <c>const_iterator _Where</c> as a starting location to search for an insertion point</para>
    ///     <para>The third function inserts the sequence of element values, from the range [_First, _Last).</para>
    ///     <para>The last two functions behave the same as the first two, except that <paramref name="_Value"/> is used to construct the inserted value.
    /// </remarks>
    /// <returns>
    ///     The iterator for the <c>concurrent_unordered_multiset</c> object.
    /// </returns>
    template<class _Valty>
        typename std::tr1::enable_if<!std::tr1::is_same<const_iterator, 
            typename std::tr1::remove_reference<_Valty>::type>::value, iterator>::type
    insert(const_iterator _Where, _Valty&& _Value)
    {
        return _Mybase::insert(_Where, std::forward<_Valty>(_Value));
    }

    /// <summary>
    ///     Erases elements from the <c>concurrent_unordered_multiset</c>. This method is not concurrency-safe.
    /// </summary>
    /// <param name="_Where">
    ///     The iterator position to erase from.
    /// </param>
    /// <remarks>
    ///     The first function erases an element from the multiset given an iterator position.
    ///     <para>The second function erases an element matching a key</para>
    ///     <para>The third function erases elements given an iterator begin and end position</para>
    /// </remarks>
    /// <returns>
    ///     The iterator for this <c>concurrent_unordered_multiset</c> object.
    /// </returns>
    /**/
    iterator unsafe_erase(const_iterator _Where)
    {
        return _Mybase::unsafe_erase(_Where);
    }

    /// <summary>
    ///     Erases elements from the <c>concurrent_unordered_multiset</c>. This method is not concurrency-safe.
    /// </summary>
    /// <param name="_Keyval">
    ///     The key to erase.
    /// </param>
    /// <remarks>
    ///     The first function erases an element from the multiset given an iterator position.
    ///     <para>The second function erases an element matching a key</para>
    ///     <para>The third function erases elements given an iterator begin and end position</para>
    /// </remarks>
    /// <returns>
    ///     The count of elements erased from this <c>concurrent_unordered_multiset</c> object.
    /// </returns>
    /**/
    size_type unsafe_erase(const key_type& _Keyval)
    {
        return _Mybase::unsafe_erase(_Keyval);
    }

    /// <summary>
    ///     Erases elements from the <c>concurrent_unordered_multiset</c>. This method is not concurrency-safe.
    /// </summary>
    /// <param name="_Begin">
    ///     Position of the first element in the range of elements to be erased.
    /// </param>
    /// <param name="_End">
    ///     Position of the first element beyond the range of elements to be erased.
    /// </param>
    /// <remarks>
    ///     The first function erases an element from the multiset given an iterator position.
    ///     <para>The second function erases an element matching a key</para>
    ///     <para>The third function erases elements given an iterator begin and end position</para>
    /// </remarks>
    /// <returns>
    ///     The iterator for this <c>concurrent_unordered_multiset</c> object.
    /// </returns>
    /**/
    iterator unsafe_erase(const_iterator _First, const_iterator _Last)
    {
        return _Mybase::unsafe_erase(_First, _Last);
    }

    /// <summary>
    ///     Swaps the contents of two <c>concurrent_unordered_multiset</c> objects. 
    ///     This method is not concurrency-safe.
    /// </summary>
    /// <param name="_Uset">
    ///     The <c>concurrent_unordered_multiset</c> object to swap with.
    /// </param>
    /**/
    void swap(concurrent_unordered_multiset& _Uset)
    {
        _Mybase::swap(_Uset);
    }

    // Observers
    /// <summary>
    ///     The hash function object.
    /// </summary>
    /**/
    hasher hash_function() const
    {
        return _M_comparator._M_hash_object;
    }

    /// <summary>
    ///     The equality comparison function object.
    /// </summary>
    /**/
    key_equal key_eq() const
    {
        return _M_comparator._M_key_compare_object;
    }
};
} // namespace samples
} // namespace Concurrency

#pragma pack(pop)
