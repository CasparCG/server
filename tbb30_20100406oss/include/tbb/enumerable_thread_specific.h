/*
    Copyright 2005-2010 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks.

    Threading Building Blocks is free software; you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    version 2 as published by the Free Software Foundation.

    Threading Building Blocks is distributed in the hope that it will be
    useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Threading Building Blocks; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    As a special exception, you may use this file as part of a free software
    library without restriction.  Specifically, if other files instantiate
    templates or use macros or inline functions from this file, or you compile
    this file and link it with other files to produce an executable, this
    file does not by itself cause the resulting executable to be covered by
    the GNU General Public License.  This exception does not however
    invalidate any other reasons why the executable file might be covered by
    the GNU General Public License.
*/

#ifndef __TBB_enumerable_thread_specific_H
#define __TBB_enumerable_thread_specific_H

#include "concurrent_vector.h"
#include "tbb_thread.h"
#include "cache_aligned_allocator.h"
#if __SUNPRO_CC
#include <string.h>  // for memcpy
#endif

#if _WIN32||_WIN64
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace tbb {

//! enum for selecting between single key and key-per-instance versions
enum ets_key_usage_type { ets_key_per_instance, ets_no_key };

namespace interface5 {
 
    //! @cond
    namespace internal { 

        template<ets_key_usage_type ETS_key_type>
        class ets_base: tbb::internal::no_copy {
        protected:
#if _WIN32||_WIN64
            typedef DWORD key_type;
#else
            typedef pthread_t key_type;
#endif
#if __TBB_GCC_3_3_PROTECTED_BROKEN
        public:
#endif
            struct slot;

            struct array {
                array* next;
                size_t lg_size;
                slot& at( size_t k ) {
                    return ((slot*)(void*)(this+1))[k];
                }
                size_t size() const {return (size_t)1<<lg_size;}
                size_t mask() const {return size()-1;}
                size_t start( size_t h ) const {
                    return h>>(8*sizeof(size_t)-lg_size);
                }
            };
            struct slot {
                key_type key;
                void* ptr;
                bool empty() const {return !key;}
                bool match( key_type k ) const {return key==k;}
                bool claim( key_type k ) {
                    __TBB_ASSERT(sizeof(tbb::atomic<key_type>)==sizeof(key_type), NULL);
                    __TBB_ASSERT(sizeof(void*)==sizeof(tbb::atomic<key_type>*), NULL);
                    union { void* space; tbb::atomic<key_type>* key_atomic; } helper;
                    helper.space = &key;
                    return helper.key_atomic->compare_and_swap(k,0)==0;
                }
            };
#if __TBB_GCC_3_3_PROTECTED_BROKEN
        protected:
#endif
        
            static key_type key_of_current_thread() {
               tbb::tbb_thread::id id = tbb::this_tbb_thread::get_id();
               key_type k;
               memcpy( &k, &id, sizeof(k) );
               return k;
            }

            //! Root of linked list of arrays of decreasing size.
            /** NULL if and only if my_count==0.  
                Each array in the list is half the size of its predecessor. */
            atomic<array*> my_root;
            atomic<size_t> my_count;
            virtual void* create_local() = 0;
            virtual void* create_array(size_t _size) = 0;  // _size in bytes
            virtual void free_array(void* ptr, size_t _size) = 0; // _size in bytes
            array* allocate( size_t lg_size ) {
                size_t n = 1<<lg_size;  
                array* a = static_cast<array*>(create_array( sizeof(array)+n*sizeof(slot) ));
                a->lg_size = lg_size;
                std::memset( a+1, 0, n*sizeof(slot) );
                return a;
            }
            void free(array* a) {
                size_t n = 1<<(a->lg_size);  
                free_array( (void *)a, size_t(sizeof(array)+n*sizeof(slot)) );
            }
            static size_t hash( key_type k ) {
                // Multiplicative hashing.  Client should use *upper* bits.
                // casts required for Mac gcc4.* compiler
#if __TBB_WORDSIZE == 4
                return uintptr_t(k)*0x9E3779B9;
#else
                return uintptr_t(k)*0x9E3779B97F4A7C15;
#endif 
            } 
        
            ets_base() {my_root=NULL; my_count=0;}
            virtual ~ets_base();  // g++ complains if this is not virtual...
            void* table_lookup( bool& exists );
            void table_clear();
            slot& table_find( key_type k ) {
                size_t h = hash(k);
                array* r = my_root;
                size_t mask = r->mask();
                for(size_t i = r->start(h);;i=(i+1)&mask) {
                    slot& s = r->at(i);
                    if( s.empty() || s.match(k) )
                        return s;
                }
            }
            void table_reserve_for_copy( const ets_base& other ) {
                __TBB_ASSERT(!my_root,NULL);
                __TBB_ASSERT(!my_count,NULL);
                if( other.my_root ) {
                    array* a = allocate(other.my_root->lg_size);
                    a->next = NULL;
                    my_root = a;
                    my_count = other.my_count;
                }
            }
        };

        template<ets_key_usage_type ETS_key_type>
        ets_base<ETS_key_type>::~ets_base() {
            __TBB_ASSERT(!my_root, NULL);
        }

        template<ets_key_usage_type ETS_key_type>
        void ets_base<ETS_key_type>::table_clear() {
            while( array* r = my_root ) {
                my_root = r->next;
                free(r);
            }
            my_count = 0;
        }
                
        template<ets_key_usage_type ETS_key_type>
        void* ets_base<ETS_key_type>::table_lookup( bool& exists ) {
            const key_type k = key_of_current_thread(); 

            __TBB_ASSERT(k!=0,NULL);
            void* found;
            size_t h = hash(k);
            for( array* r=my_root; r; r=r->next ) {
                size_t mask=r->mask();
                for(size_t i = r->start(h); ;i=(i+1)&mask) {
                    slot& s = r->at(i);
                    if( s.empty() ) break;
                    if( s.match(k) ) {
                        if( r==my_root ) {
                            // Success at top level
                            exists = true;
                            return s.ptr;
                        } else {
                            // Success at some other level.  Need to insert at top level.
                            exists = true;
                            found = s.ptr;
                            goto insert;
                        }
                    }
                }
            }
            // Key does not yet exist
            exists = false;
            found = create_local();
            {
                size_t c = ++my_count;
                array* r = my_root;
                if( !r || c>r->size()/2 ) {
                    size_t s = r ? r->lg_size : 2;
                    while( c>size_t(1)<<(s-1) ) ++s;
                    array* a = allocate(s);
                    for(;;) {
                        a->next = my_root;
                        array* new_r = my_root.compare_and_swap(a,r);
                        if( new_r==r ) break;
                        if( new_r->lg_size>=s ) {
                            // Another thread inserted an equal or  bigger array, so our array is superfluous.
                            free(a);
                            break;
                        }
                        r = new_r;
                    }
                }
            }
        insert:
            // Guaranteed to be room for it, and it is not present, so search for empty slot and grab it.
            array* ir = my_root;
            size_t mask = ir->mask();
            for(size_t i = ir->start(h);;i=(i+1)&mask) {
                slot& s = ir->at(i);
                if( s.empty() ) {
                    if( s.claim(k) ) {
                        s.ptr = found;
                        return found;
                    }
                }
            }
        };

        //! Specialization that exploits native TLS 
        template <>
        class ets_base<ets_key_per_instance>: protected ets_base<ets_no_key> {
            typedef ets_base<ets_no_key> super;
#if _WIN32||_WIN64
            typedef DWORD tls_key_t;
            void create_key() { my_key = TlsAlloc(); }
            void destroy_key() { TlsFree(my_key); }
            void set_tls(void * value) { TlsSetValue(my_key, (LPVOID)value); }
            void* get_tls() { return (void *)TlsGetValue(my_key); }
#else
            typedef pthread_key_t tls_key_t;
            void create_key() { pthread_key_create(&my_key, NULL); }
            void destroy_key() { pthread_key_delete(my_key); }
            void set_tls( void * value ) const { pthread_setspecific(my_key, value); }
            void* get_tls() const { return pthread_getspecific(my_key); }
#endif
            tls_key_t my_key;
            virtual void* create_local() = 0;
            virtual void* create_array(size_t _size) = 0;  // _size in bytes
            virtual void free_array(void* ptr, size_t _size) = 0; // size in bytes
        public:
            ets_base() {create_key();}
            ~ets_base() {destroy_key();}
            void* table_lookup( bool& exists ) {
                void* found = get_tls();
                if( found ) {
                    exists=true;
                } else {
                    found = super::table_lookup(exists);
                    set_tls(found);
                }
                return found; 
            }
            void table_clear() {
                destroy_key();
                create_key(); 
                super::table_clear();
            }
        };

        //! Random access iterator for traversing the thread local copies.
        template< typename Container, typename Value >
        class enumerable_thread_specific_iterator 
#if defined(_WIN64) && defined(_MSC_VER) 
            // Ensure that Microsoft's internal template function _Val_type works correctly.
            : public std::iterator<std::random_access_iterator_tag,Value>
#endif /* defined(_WIN64) && defined(_MSC_VER) */
        {
            //! current position in the concurrent_vector 
        
            Container *my_container;
            typename Container::size_type my_index;
            mutable Value *my_value;
        
            template<typename C, typename T>
            friend enumerable_thread_specific_iterator<C,T> operator+( ptrdiff_t offset, 
                                                                       const enumerable_thread_specific_iterator<C,T>& v );
        
            template<typename C, typename T, typename U>
            friend bool operator==( const enumerable_thread_specific_iterator<C,T>& i, 
                                    const enumerable_thread_specific_iterator<C,U>& j );
        
            template<typename C, typename T, typename U>
            friend bool operator<( const enumerable_thread_specific_iterator<C,T>& i, 
                                   const enumerable_thread_specific_iterator<C,U>& j );
        
            template<typename C, typename T, typename U>
            friend ptrdiff_t operator-( const enumerable_thread_specific_iterator<C,T>& i, const enumerable_thread_specific_iterator<C,U>& j );
            
            template<typename C, typename U> 
            friend class enumerable_thread_specific_iterator;
        
            public:
        
            enumerable_thread_specific_iterator( const Container &container, typename Container::size_type index ) : 
                my_container(&const_cast<Container &>(container)), my_index(index), my_value(NULL) {}
        
            //! Default constructor
            enumerable_thread_specific_iterator() : my_container(NULL), my_index(0), my_value(NULL) {}
        
            template<typename U>
            enumerable_thread_specific_iterator( const enumerable_thread_specific_iterator<Container, U>& other ) :
                    my_container( other.my_container ), my_index( other.my_index), my_value( const_cast<Value *>(other.my_value) ) {}
        
            enumerable_thread_specific_iterator operator+( ptrdiff_t offset ) const {
                return enumerable_thread_specific_iterator(*my_container, my_index + offset);
            }
        
            enumerable_thread_specific_iterator &operator+=( ptrdiff_t offset ) {
                my_index += offset;
                my_value = NULL;
                return *this;
            }
        
            enumerable_thread_specific_iterator operator-( ptrdiff_t offset ) const {
                return enumerable_thread_specific_iterator( *my_container, my_index-offset );
            }
        
            enumerable_thread_specific_iterator &operator-=( ptrdiff_t offset ) {
                my_index -= offset;
                my_value = NULL;
                return *this;
            }
        
            Value& operator*() const {
                Value* value = my_value;
                if( !value ) {
                    value = my_value = reinterpret_cast<Value *>(&(*my_container)[my_index].value);
                }
                __TBB_ASSERT( value==reinterpret_cast<Value *>(&(*my_container)[my_index].value), "corrupt cache" );
                return *value;
            }
        
            Value& operator[]( ptrdiff_t k ) const {
               return (*my_container)[my_index + k].value;
            }
        
            Value* operator->() const {return &operator*();}
        
            enumerable_thread_specific_iterator& operator++() {
                ++my_index;
                my_value = NULL;
                return *this;
            }
        
            enumerable_thread_specific_iterator& operator--() {
                --my_index;
                my_value = NULL;
                return *this;
            }
        
            //! Post increment
            enumerable_thread_specific_iterator operator++(int) {
                enumerable_thread_specific_iterator result = *this;
                ++my_index;
                my_value = NULL;
                return result;
            }
        
            //! Post decrement
            enumerable_thread_specific_iterator operator--(int) {
                enumerable_thread_specific_iterator result = *this;
                --my_index;
                my_value = NULL;
                return result;
            }
        
            // STL support
            typedef ptrdiff_t difference_type;
            typedef Value value_type;
            typedef Value* pointer;
            typedef Value& reference;
            typedef std::random_access_iterator_tag iterator_category;
        };
        
        template<typename Container, typename T>
        enumerable_thread_specific_iterator<Container,T> operator+( ptrdiff_t offset, 
                                                                    const enumerable_thread_specific_iterator<Container,T>& v ) {
            return enumerable_thread_specific_iterator<Container,T>( v.my_container, v.my_index + offset );
        }
        
        template<typename Container, typename T, typename U>
        bool operator==( const enumerable_thread_specific_iterator<Container,T>& i, 
                         const enumerable_thread_specific_iterator<Container,U>& j ) {
            return i.my_index==j.my_index && i.my_container == j.my_container;
        }
        
        template<typename Container, typename T, typename U>
        bool operator!=( const enumerable_thread_specific_iterator<Container,T>& i, 
                         const enumerable_thread_specific_iterator<Container,U>& j ) {
            return !(i==j);
        }
        
        template<typename Container, typename T, typename U>
        bool operator<( const enumerable_thread_specific_iterator<Container,T>& i, 
                        const enumerable_thread_specific_iterator<Container,U>& j ) {
            return i.my_index<j.my_index;
        }
        
        template<typename Container, typename T, typename U>
        bool operator>( const enumerable_thread_specific_iterator<Container,T>& i, 
                        const enumerable_thread_specific_iterator<Container,U>& j ) {
            return j<i;
        }
        
        template<typename Container, typename T, typename U>
        bool operator>=( const enumerable_thread_specific_iterator<Container,T>& i, 
                         const enumerable_thread_specific_iterator<Container,U>& j ) {
            return !(i<j);
        }
        
        template<typename Container, typename T, typename U>
        bool operator<=( const enumerable_thread_specific_iterator<Container,T>& i, 
                         const enumerable_thread_specific_iterator<Container,U>& j ) {
            return !(j<i);
        }
        
        template<typename Container, typename T, typename U>
        ptrdiff_t operator-( const enumerable_thread_specific_iterator<Container,T>& i, 
                             const enumerable_thread_specific_iterator<Container,U>& j ) {
            return i.my_index-j.my_index;
        }

    template<typename SegmentedContainer, typename Value >
        class segmented_iterator
#if defined(_WIN64) && defined(_MSC_VER)
        : public std::iterator<std::input_iterator_tag, Value>
#endif
        {
            template<typename C, typename T, typename U>
            friend bool operator==(const segmented_iterator<C,T>& i, const segmented_iterator<C,U>& j);

            template<typename C, typename T, typename U>
            friend bool operator!=(const segmented_iterator<C,T>& i, const segmented_iterator<C,U>& j);
            
            template<typename C, typename U> 
            friend class segmented_iterator;

            public:

                segmented_iterator() {my_segcont = NULL;}

                segmented_iterator( const SegmentedContainer& _segmented_container ) : 
                    my_segcont(const_cast<SegmentedContainer*>(&_segmented_container)),
                    outer_iter(my_segcont->end()) { }

                ~segmented_iterator() {}

                typedef typename SegmentedContainer::iterator outer_iterator;
                typedef typename SegmentedContainer::value_type InnerContainer;
                typedef typename InnerContainer::iterator inner_iterator;

                // STL support
                typedef ptrdiff_t difference_type;
                typedef Value value_type;
                typedef typename SegmentedContainer::size_type size_type;
                typedef Value* pointer;
                typedef Value& reference;
                typedef std::input_iterator_tag iterator_category;

                // Copy Constructor
                template<typename U>
                segmented_iterator(const segmented_iterator<SegmentedContainer, U>& other) :
                    my_segcont(other.my_segcont),
                    outer_iter(other.outer_iter),
                    // can we assign a default-constructed iterator to inner if we're at the end?
                    inner_iter(other.inner_iter)
                {}

                // assignment
                template<typename U>
                segmented_iterator& operator=( const segmented_iterator<SegmentedContainer, U>& other) {
                    if(this != &other) {
                        my_segcont = other.my_segcont;
                        outer_iter = other.outer_iter;
                        if(outer_iter != my_segcont->end()) inner_iter = other.inner_iter;
                    }
                    return *this;
                }

                // allow assignment of outer iterator to segmented iterator.  Once it is
                // assigned, move forward until a non-empty inner container is found or
                // the end of the outer container is reached.
                segmented_iterator& operator=(const outer_iterator& new_outer_iter) {
                    __TBB_ASSERT(my_segcont != NULL, NULL);
                    // check that this iterator points to something inside the segmented container
                    for(outer_iter = new_outer_iter ;outer_iter!=my_segcont->end(); ++outer_iter) {
                        if( !outer_iter->empty() ) {
                            inner_iter = outer_iter->begin();
                            break;
                        }
                    }
                    return *this;
                }

                // pre-increment
                segmented_iterator& operator++() {
                    advance_me();
                    return *this;
                }

                // post-increment
                segmented_iterator operator++(int) {
                    segmented_iterator tmp = *this;
                    operator++();
                    return tmp;
                }

                bool operator==(const outer_iterator& other_outer) const {
                    __TBB_ASSERT(my_segcont != NULL, NULL);
                    return (outer_iter == other_outer &&
                            (outer_iter == my_segcont->end() || inner_iter == outer_iter->begin()));
                }

                bool operator!=(const outer_iterator& other_outer) const {
                    return !operator==(other_outer);

                }

                // (i)* RHS
                reference operator*() const {
                    __TBB_ASSERT(my_segcont != NULL, NULL);
                    __TBB_ASSERT(outer_iter != my_segcont->end(), "Dereferencing a pointer at end of container");
                    __TBB_ASSERT(inner_iter != outer_iter->end(), NULL); // should never happen
                    return *inner_iter;
                }

                // i->
                pointer operator->() const { return &operator*();}

            private:
                SegmentedContainer*             my_segcont;
                outer_iterator outer_iter;
                inner_iterator inner_iter;

                void advance_me() {
                    __TBB_ASSERT(my_segcont != NULL, NULL);
                    __TBB_ASSERT(outer_iter != my_segcont->end(), NULL); // not true if there are no inner containers
                    __TBB_ASSERT(inner_iter != outer_iter->end(), NULL); // not true if the inner containers are all empty.
                    ++inner_iter;
                    while(inner_iter == outer_iter->end() && ++outer_iter != my_segcont->end()) {
                        inner_iter = outer_iter->begin();
                    }
                }
        };    // segmented_iterator

        template<typename SegmentedContainer, typename T, typename U>
        bool operator==( const segmented_iterator<SegmentedContainer,T>& i, 
                         const segmented_iterator<SegmentedContainer,U>& j ) {
            if(i.my_segcont != j.my_segcont) return false;
            if(i.my_segcont == NULL) return true;
            if(i.outer_iter != j.outer_iter) return false;
            if(i.outer_iter == i.my_segcont->end()) return true;
            return i.inner_iter == j.inner_iter;
        }

        // !=
        template<typename SegmentedContainer, typename T, typename U>
        bool operator!=( const segmented_iterator<SegmentedContainer,T>& i, 
                         const segmented_iterator<SegmentedContainer,U>& j ) {
            return !(i==j);
        }

        // storage for initialization function pointer
        template<typename T>
        struct callback_base {
            virtual T apply( ) = 0;
            virtual void destroy( ) = 0;
            // need to be able to create copies of callback_base for copy constructor
            virtual callback_base* make_copy() = 0;
            // need virtual destructor to satisfy GCC compiler warning
            virtual ~callback_base() { }
        };

        template <typename T, typename Functor>
        struct callback_leaf : public callback_base<T>, public tbb::internal::no_copy {
            typedef Functor my_callback_type;
            typedef callback_leaf<T,Functor> my_type;
            typedef my_type* callback_pointer;
            typedef typename tbb::tbb_allocator<my_type> my_allocator_type;
            Functor f;
            callback_leaf( const Functor& f_) : f(f_) {
            }

            static callback_pointer new_callback(const Functor& f_ ) {
                void* new_void = my_allocator_type().allocate(1);
                callback_pointer new_cb = new (new_void) callback_leaf<T,Functor>(f_); // placement new
                return new_cb;
            }

            /* override */ callback_pointer make_copy() {
                return new_callback( f );
            }

             /* override */ void destroy( ) {
                 callback_pointer my_ptr = this;
                 my_allocator_type().destroy(my_ptr);
                 my_allocator_type().deallocate(my_ptr,1);
             }
            /* override */ T apply() { return f(); }  // does copy construction of returned value.
        };


        //! Template for adding padding in order to avoid false sharing
        /** ModularSize should be sizeof(U) modulo the cache line size.
            All maintenance of the space will be done explicitly on push_back,
            and all thread local copies must be destroyed before the concurrent
            vector is deleted.
        */
        template<typename U, size_t ModularSize>
        struct ets_element {
            char value[sizeof(U) + tbb::internal::NFS_MaxLineSize-ModularSize];
            void unconstruct() {
                // "reinterpret_cast<U*>(&value)->~U();" causes type-punning warning with gcc 4.4,
                // "U* u = reinterpret_cast<U*>(&value); u->~U();" causes unused variable warning with VS2010.
                // Thus another "casting via union" hack.
                __TBB_ASSERT(sizeof(void*)==sizeof(U*),NULL);
                union { void* space; U* val; } helper;
                helper.space = &value;
                helper.val->~U();
            }
        };

        //! Partial specialization for case where no padding is needed.
        template<typename U>
        struct ets_element<U,0> {
            char value[sizeof(U)];
            void unconstruct() { // Same implementation as in general case
                __TBB_ASSERT(sizeof(void*)==sizeof(U*),NULL);
                union { void* space; U* val; } helper;
                helper.space = &value;
                helper.val->~U();
            }
        };

    } // namespace internal
    //! @endcond

    //! The enumerable_thread_specific container
    /** enumerable_thread_specific has the following properties:
        - thread-local copies are lazily created, with default, exemplar or function initialization.
        - thread-local copies do not move (during lifetime, and excepting clear()) so the address of a copy is invariant.
        - the contained objects need not have operator=() defined if combine is not used.
        - enumerable_thread_specific containers may be copy-constructed or assigned.
        - thread-local copies can be managed by hash-table, or can be accessed via TLS storage for speed.
        - outside of parallel contexts, the contents of all thread-local copies are accessible by iterator or using combine or combine_each methods
        
    @par Segmented iterator
        When the thread-local objects are containers with input_iterators defined, a segmented iterator may
        be used to iterate over all the elements of all thread-local copies.

    @par combine and combine_each
        - Both methods are defined for enumerable_thread_specific. 
        - combine() requires the the type T have operator=() defined.  
        - neither method modifies the contents of the object (though there is no guarantee that the applied methods do not modify the object.)  
        - Both are evaluated in serial context (the methods are assumed to be non-benign.)
        
    @ingroup containers */
    template <typename T, 
              typename Allocator=cache_aligned_allocator<T>, 
              ets_key_usage_type ETS_key_type=ets_no_key > 
    class enumerable_thread_specific: internal::ets_base<ETS_key_type> { 

        template<typename U, typename A, ets_key_usage_type C> friend class enumerable_thread_specific;
    
        typedef internal::ets_element<T,sizeof(T)%tbb::internal::NFS_MaxLineSize> padded_element;

        //! A generic range, used to create range objects from the iterators
        template<typename I>
        class generic_range_type: public blocked_range<I> {
        public:
            typedef T value_type;
            typedef T& reference;
            typedef const T& const_reference;
            typedef I iterator;
            typedef ptrdiff_t difference_type;
            generic_range_type( I begin_, I end_, size_t grainsize_ = 1) : blocked_range<I>(begin_,end_,grainsize_) {} 
            template<typename U>
            generic_range_type( const generic_range_type<U>& r) : blocked_range<I>(r.begin(),r.end(),r.grainsize()) {} 
            generic_range_type( generic_range_type& r, split ) : blocked_range<I>(r,split()) {}
        };
    
        typedef typename Allocator::template rebind< padded_element >::other padded_allocator_type;
        typedef tbb::concurrent_vector< padded_element, padded_allocator_type > internal_collection_type;
        
        internal::callback_base<T> *my_finit_callback;

        // need to use a pointed-to exemplar because T may not be assignable.
        // using tbb_allocator instead of padded_element_allocator because we may be
        // copying an exemplar from one instantiation of ETS to another with a different
        // allocator.
        typedef typename tbb::tbb_allocator<padded_element > exemplar_allocator_type;
        static padded_element * create_exemplar(const T& my_value) {
            padded_element *new_exemplar = reinterpret_cast<padded_element *>(exemplar_allocator_type().allocate(1));
            new(new_exemplar->value) T(my_value);
            return new_exemplar;
        }

        static padded_element *create_exemplar( ) {
            padded_element *new_exemplar = reinterpret_cast<padded_element *>(exemplar_allocator_type().allocate(1));
            new(new_exemplar->value) T( );
            return new_exemplar;
        }

        static void free_exemplar(padded_element *my_ptr) {
            my_ptr->unconstruct();
            exemplar_allocator_type().destroy(my_ptr);
            exemplar_allocator_type().deallocate(my_ptr,1);
        }

        padded_element* my_exemplar_ptr;

        internal_collection_type my_locals;
   
        /*override*/ void* create_local() {
#if TBB_DEPRECATED
            void* lref = &my_locals[my_locals.push_back(padded_element())];
#else
            void* lref = &*my_locals.push_back(padded_element());
#endif
            if(my_finit_callback) {
                new(lref) T(my_finit_callback->apply());
            } else if(my_exemplar_ptr) {
                pointer t_exemp = reinterpret_cast<T *>(&(my_exemplar_ptr->value));
                new(lref) T(*t_exemp);
            } else {
                new(lref) T();
            }
            return lref;
        } 

        void unconstruct_locals() {
            for(typename internal_collection_type::iterator cvi = my_locals.begin(); cvi != my_locals.end(); ++cvi) {
                cvi->unconstruct();
            }
        }

        typedef typename Allocator::template rebind< uintptr_t >::other array_allocator_type;

        // _size is in bytes
        /*override*/ void* create_array(size_t _size) {
            size_t nelements = (_size + sizeof(uintptr_t) -1) / sizeof(uintptr_t);
            return array_allocator_type().allocate(nelements);
        }

        /*override*/ void free_array( void* _ptr, size_t _size) {
            size_t nelements = (_size + sizeof(uintptr_t) -1) / sizeof(uintptr_t);
            array_allocator_type().deallocate( reinterpret_cast<uintptr_t *>(_ptr),nelements);
        }
   
    public:
    
        //! Basic types
        typedef Allocator allocator_type;
        typedef T value_type;
        typedef T& reference;
        typedef const T& const_reference;
        typedef T* pointer;
        typedef const T* const_pointer;
        typedef typename internal_collection_type::size_type size_type;
        typedef typename internal_collection_type::difference_type difference_type;
    
        // Iterator types
        typedef typename internal::enumerable_thread_specific_iterator< internal_collection_type, value_type > iterator;
        typedef typename internal::enumerable_thread_specific_iterator< internal_collection_type, const value_type > const_iterator;

        // Parallel range types
        typedef generic_range_type< iterator > range_type;
        typedef generic_range_type< const_iterator > const_range_type;
    
        //! Default constructor, which leads to default construction of local copies
        enumerable_thread_specific() : my_finit_callback(0) { 
            my_exemplar_ptr = 0;
        }

        //! construction with initializer method
        // Finit should be a function taking 0 parameters and returning a T
        template <typename Finit>
        enumerable_thread_specific( Finit _finit )
        {
            my_finit_callback = internal::callback_leaf<T,Finit>::new_callback( _finit );
            my_exemplar_ptr = 0; // don't need exemplar if function is provided
        }
    
        //! Constuction with exemplar, which leads to copy construction of local copies
        enumerable_thread_specific(const T &_exemplar) : my_finit_callback(0) {
            my_exemplar_ptr = create_exemplar(_exemplar);
        }
    
        //! Destructor
        ~enumerable_thread_specific() { 
            if(my_finit_callback) {
                my_finit_callback->destroy();
            }
            if(my_exemplar_ptr) {
                free_exemplar(my_exemplar_ptr);
            }
            this->clear();  // deallocation before the derived class is finished destructing
            // So free(array *) is still accessible
        }
      
        //! returns reference to local, discarding exists
        reference local() {
            bool exists;
            return local(exists);
        }

        //! Returns reference to calling thread's local copy, creating one if necessary
        reference local(bool& exists)  {
            __TBB_ASSERT(ETS_key_type==ets_no_key,"ets_key_per_instance not yet implemented"); 
            void* ptr = this->table_lookup(exists);
            return *(T*)ptr;
        }

        //! Get the number of local copies
        size_type size() const { return my_locals.size(); }
    
        //! true if there have been no local copies created
        bool empty() const { return my_locals.empty(); }
    
        //! begin iterator
        iterator begin() { return iterator( my_locals, 0 ); }
        //! end iterator
        iterator end() { return iterator(my_locals, my_locals.size() ); }
    
        //! begin const iterator
        const_iterator begin() const { return const_iterator(my_locals, 0); }
    
        //! end const iterator
        const_iterator end() const { return const_iterator(my_locals, my_locals.size()); }

        //! Get range for parallel algorithms
        range_type range( size_t grainsize=1 ) { return range_type( begin(), end(), grainsize ); } 
        
        //! Get const range for parallel algorithms
        const_range_type range( size_t grainsize=1 ) const { return const_range_type( begin(), end(), grainsize ); }

        //! Destroys local copies
        void clear() {
            unconstruct_locals();
            my_locals.clear();
            this->table_clear();
            // callback is not destroyed
            // exemplar is not destroyed
        }

    private:

        template<typename U, typename A2, ets_key_usage_type C2>
        void internal_copy( const enumerable_thread_specific<U, A2, C2>& other);

    public:

        template<typename U, typename Alloc, ets_key_usage_type Cachetype>
        enumerable_thread_specific( const enumerable_thread_specific<U, Alloc, Cachetype>& other ) : internal::ets_base<ETS_key_type> ()
        {
            internal_copy(other);
        }

        enumerable_thread_specific( const enumerable_thread_specific& other ) : internal::ets_base<ETS_key_type> ()
        {
            internal_copy(other);
        }

    private:

        template<typename U, typename A2, ets_key_usage_type C2>
        enumerable_thread_specific &
        internal_assign(const enumerable_thread_specific<U, A2, C2>& other) {
            if(static_cast<void *>( this ) != static_cast<const void *>( &other )) {
                this->clear(); 
                if(my_finit_callback) {
                    my_finit_callback->destroy();
                    my_finit_callback = 0;
                }
                if(my_exemplar_ptr) {
                    free_exemplar(my_exemplar_ptr);
                    my_exemplar_ptr = 0;
                }
                internal_copy( other );
            }
            return *this;
        }

    public:

        // assignment
        enumerable_thread_specific& operator=(const enumerable_thread_specific& other) {
            return internal_assign(other);
        }

        template<typename U, typename Alloc, ets_key_usage_type Cachetype>
        enumerable_thread_specific& operator=(const enumerable_thread_specific<U, Alloc, Cachetype>& other)
        {
            return internal_assign(other);
        }

        // combine_func_t has signature T(T,T) or T(const T&, const T&)
        template <typename combine_func_t>
        T combine(combine_func_t f_combine) {
            if(begin() == end()) {
                if(my_finit_callback) {
                    return my_finit_callback->apply();
                }
                pointer local_ref = reinterpret_cast<T*>((my_exemplar_ptr->value));
                return T(*local_ref);
            }
            const_iterator ci = begin();
            T my_result = *ci;
            while(++ci != end()) 
                my_result = f_combine( my_result, *ci );
            return my_result;
        }

        // combine_func_t has signature void(T) or void(const T&)
        template <typename combine_func_t>
        void combine_each(combine_func_t f_combine) {
            for(const_iterator ci = begin(); ci != end(); ++ci) {
                f_combine( *ci );
            }
        }

    }; // enumerable_thread_specific

    
    template <typename T, typename Allocator, ets_key_usage_type ETS_key_type> 
    template<typename U, typename A2, ets_key_usage_type C2>
    void enumerable_thread_specific<T,Allocator,ETS_key_type>::internal_copy( const enumerable_thread_specific<U, A2, C2>& other) {
        typedef internal::ets_base<ets_no_key> base;
        __TBB_ASSERT(my_locals.size()==0,NULL);
        this->table_reserve_for_copy( other );
        for( base::array* r=other.my_root; r; r=r->next ) {
            for( size_t i=0; i<r->size(); ++i ) {
                base::slot& s1 = r->at(i);
                if( !s1.empty() ) {
                    base::slot& s2 = this->table_find(s1.key);
                    if( s2.empty() ) { 
#if TBB_DEPRECATED
                        void* lref = &my_locals[my_locals.push_back(padded_element())];
#else
                        void* lref = &*my_locals.push_back(padded_element());
#endif
                        s2.ptr = new(lref) T(*(U*)s1.ptr);
                        s2.key = s1.key;
                    } else {
                        // Skip the duplicate
                    } 
                }
            }
        }
        if(other.my_finit_callback) {
            my_finit_callback = other.my_finit_callback->make_copy();
        } else {
            my_finit_callback = 0;
        }
        if(other.my_exemplar_ptr) {
            pointer local_ref = reinterpret_cast<U*>(other.my_exemplar_ptr->value);
            my_exemplar_ptr = create_exemplar(*local_ref);
        } else {
            my_exemplar_ptr = 0;
        }
    }

    template< typename Container >
    class flattened2d {

        // This intermediate typedef is to address issues with VC7.1 compilers
        typedef typename Container::value_type conval_type;

    public:

        //! Basic types
        typedef typename conval_type::size_type size_type;
        typedef typename conval_type::difference_type difference_type;
        typedef typename conval_type::allocator_type allocator_type;
        typedef typename conval_type::value_type value_type;
        typedef typename conval_type::reference reference;
        typedef typename conval_type::const_reference const_reference;
        typedef typename conval_type::pointer pointer;
        typedef typename conval_type::const_pointer const_pointer;

        typedef typename internal::segmented_iterator<Container, value_type> iterator;
        typedef typename internal::segmented_iterator<Container, const value_type> const_iterator;

        flattened2d( const Container &c, typename Container::const_iterator b, typename Container::const_iterator e ) : 
            my_container(const_cast<Container*>(&c)), my_begin(b), my_end(e) { }

        flattened2d( const Container &c ) : 
            my_container(const_cast<Container*>(&c)), my_begin(c.begin()), my_end(c.end()) { }

        iterator begin() { return iterator(*my_container) = my_begin; }
        iterator end() { return iterator(*my_container) = my_end; }
        const_iterator begin() const { return const_iterator(*my_container) = my_begin; }
        const_iterator end() const { return const_iterator(*my_container) = my_end; }

        size_type size() const {
            size_type tot_size = 0;
            for(typename Container::const_iterator i = my_begin; i != my_end; ++i) {
                tot_size += i->size();
            }
            return tot_size;
        }

    private:

        Container *my_container;
        typename Container::const_iterator my_begin;
        typename Container::const_iterator my_end;

    };

    template <typename Container>
    flattened2d<Container> flatten2d(const Container &c, const typename Container::const_iterator b, const typename Container::const_iterator e) {
        return flattened2d<Container>(c, b, e);
    }

    template <typename Container>
    flattened2d<Container> flatten2d(const Container &c) {
        return flattened2d<Container>(c);
    }

} // interface5

namespace internal {
using interface5::internal::segmented_iterator;
}

using interface5::enumerable_thread_specific;
using interface5::flattened2d;
using interface5::flatten2d;

} // namespace tbb

#endif
