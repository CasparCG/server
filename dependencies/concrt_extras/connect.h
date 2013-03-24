//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: connect.h
//
//  connect / disconnect helper functions
//
//--------------------------------------------------------------------------
#pragma once
#include <type_traits>
#include <agents.h>

namespace Concurrency
{
        namespace details
        {   
            //details for the connect type traits

            //creates a has_member_foo class which can be used to check
            //for a named member variable
            #define DEFINE_HAS_MEMBER(NAME)                                             \
            template <typename T> class has_member_ ## NAME {                           \
            private:                                                                    \
                template <typename U> static std::true_type helper(decltype(&U::NAME)); \
                template <typename U> static std::false_type helper(...);               \
                typedef decltype(helper<T>(nullptr)) helper_t;                          \
            public:                                                                     \
                static const bool value = helper_t::value;                              \
            };

            //define input_buffer & output_buffer classes
            DEFINE_HAS_MEMBER(input_buffer)
            DEFINE_HAS_MEMBER(output_buffer)

                //there must be an existing type trait for this
            template<const bool value_type>
            struct _is_true : std::false_type
            {
            };

            template<>
            struct _is_true<true> : std::true_type
            {
            };

            //_as_ref normalizes an instance of a class to a reference parameter.
            //this works with pointers, references and values, but probably not with
            //pointer to pointer and definitely not with shared pointers
            template<typename T>
            class _as_ref
            {
                _as_ref & operator=(const _as_ref & );

            public:
                typedef T type;
                //_as_ref(T& t): ref(t){}
                _as_ref(T& t): ref(t){}
                _as_ref(T* t): ref(*t){}
                typename std::add_const<typename std::add_reference<T>::type>::type& ref;
            };

            //_as_ref normalizes an instance of a class to a reference parameter.
            template <typename T>
            class as_ref : public _as_ref<typename std::remove_pointer<T>::type>
            {
                as_ref & operator=( const as_ref & );
            public:
                as_ref(T& t):_as_ref(t){}
            };

        }// end details namespace

        using namespace details;
        //type traits, determines if a class acts as a compound message target
        template<typename target_type>
        struct is_compound_target
            : _is_true<has_member_input_buffer<typename std::remove_cv<target_type>::type>::value>
        {
        };

        //type traits, determines if a class acts as a compound message source
        template<typename source_type>
        struct is_compound_source
            : _is_true<has_member_output_buffer<typename std::remove_cv<source_type>::type>::value>
        {
        };

        //connects two message blocks or compound message blocks forward declaration
        template<typename source_type, typename target_type>
        inline void connect(source_type& source, target_type& target);

        //disconnects two message blocks or compound message blocks forward declaration
        template<typename source_type, typename target_type>
        inline void disconnect(source_type& source, target_type& target);

        namespace details
        {
            //connects an ISource to an ITarget
            template <typename source_block, typename target_block>
            inline void connect_to_target_impl(source_block& source, target_block& target)
            {
                source.link_target(&target);
            }
            //connect a simple source to a simple target
            template<typename source_type, typename target_type>
            inline void connect_impl(source_type& source, std::false_type /*not_compound_source*/, 
                target_type& target, std::false_type /*not_compound_target*/)
            {
                connect_to_target_impl(source,target);
            }

            //connect a simple source to a compound target
            template<typename source_type, typename target_type>
            inline void connect_impl(source_type& source, std::false_type /*not_compound_source*/, 
                target_type& target, std::true_type /*is_compound_target*/)
            {
                samples::connect(source, target.input_buffer);
            }

            //connect a compound source to a simple target
            template<typename source_type, typename target_type>
            inline void connect_impl(source_type& source, std::true_type  /*not_compound_source*/, 
                target_type& target, std::false_type /*is_compound_target*/)
            {
                samples::connect(source.output_buffer, target);
            }

            //connect a compound source to a compound target
            template<typename source_type, typename target_type>
            inline void connect_impl(source_type& source, std::true_type /*not_compound_source*/, 
                target_type& target, std::true_type /*is_compound_target*/)
            {
                samples::connect(source.output_buffer, target.input_buffer);
            }

            //connect_impl function that works with 'as_ref' types, this function
            //relies on overloading and is_compound_source/target type traits to resolve
            //to simple message blocks
            template<typename source_type, typename target_type>
            inline void connect_impl(const source_type& source, const target_type& target)
            {
                connect_impl(source.ref, is_compound_source<typename source_type::type>(),
                    target.ref, is_compound_target<typename target_type::type>());
            }

            template <typename source_block, typename target_block>
            inline void disconnect_from_target_impl(source_block& source, target_block& target)
            {
                source.unlink_target(&target);
            }
            //disconnect a source from a target, neither of which is a compound source or target
            template<typename source_type, typename target_type>
            inline void disconnect_impl(source_type& source, std::false_type /*not_compound_source*/, 
                target_type& target, std::false_type /*not_compound_target*/)
            {
                disconnect_from_target_impl(source,target);
            }
            //disconnect a simple source to a compound target
            template<typename source_type, typename target_type>
            inline void disconnect_impl(source_type& source, std::false_type /*not_compound_source*/, 
                target_type& target, std::true_type  /*is_compound_target*/)
            {
                samples::disconnect(source, target.input_buffer);
            }
            //disconnect a compound source from a simple target
            template<typename source_type, typename target_type>
            inline void disconnect_impl(source_type& source, std::true_type  /*not_compound_source*/, 
                target_type& target, std::false_type /*is_compound_target*/)
            {
                samples::disconnect(source.output_buffer, target);
            }

            //disconnect a compound source from a compound target
            template<typename source_type, typename target_type>
            inline void disconnect_impl(source_type& source, std::true_type /*not_compound_source*/, 
                target_type& target, std::true_type /*is_compound_target*/)
            {
                samples::disconnect(source.output_buffer, target.input_buffer);
            }

            //disconnect impl has pointers removed already these are as_ref types
            template<typename source_type, typename target_type>
            inline void disconnect_impl(const source_type& source, const target_type& target)
            {
                //first pass remove pointers
                disconnect_impl(source.ref, is_compound_source<typename source_type::type>(),
                    target.ref, is_compound_target<typename target_type::type>());
            }
            template<typename source_type>
            inline void disconnect_all_impl(source_type& source, std::false_type/*not_compound_source*/)
            {
                source.unlink_targets();
            }
            template<typename source_type>
            inline void disconnect_all_impl(source_type& source, std::true_type /*is_compound_source*/)
            {
                samples::disconnect(source.output_buffer);
            }
            template<typename source_type>
            inline void disconnect_all_impl(source_type& source)
            {
                details::disconnect_all_impl(source.ref, is_compound_source<typename source_type::type>());
            }

        }// end details namespace

        //connects two message blocks or compound message blocks
        template<typename source_type, typename target_type>
        inline void connect(source_type& source, target_type& target)
        {
            details::connect_impl(as_ref<source_type>(source), as_ref<target_type>(target));
        }

        //disconnects two message blocks or compound message blocks
        template<typename source_type, typename target_type>
        inline void disconnect(source_type& source, target_type& target)
        {
            details::disconnect_impl(as_ref<source_type>(source), as_ref<target_type>(target));
        }

        //connects two message blocks or compound message blocks, source is shared_ptr
        template<typename source_type, typename target_type>
        inline void connect(std::shared_ptr<source_type>& source_ptr, target_type& target)
        {
            details::connect_impl(as_ref<source_type>(*source_ptr.get()), as_ref<target_type>(target));
        }

        //connects two message blocks or compound message blocks both shared ptrs
        template<typename source_type, typename target_type>
        inline void connect(std::shared_ptr<source_type>& source_ptr, std::shared_ptr<target_type>& target_ptr)
        {
            details::connect_impl(as_ref<source_type>(*source_ptr.get()), as_ref<target_type>(*target_ptr.get()));
        }

        //connects two message blocks or compound message blocks target is shared_ptr
        template<typename source_type, typename target_type>
        inline void connect(source_type& source, std::shared_ptr<target_type>& target_ptr)
        {
            details::connect_impl(as_ref<source_type>(source), as_ref<target_type>(*target_ptr.get()));
        }

        //connects two message blocks or compound message blocks, source is shared_ptr
        template<typename source_type, typename target_type>
        inline void disconnect(std::shared_ptr<source_type>& source_ptr, target_type& target)
        {
            details::disconnect_impl(as_ref<source_type>(*source_ptr.get()), as_ref<target_type>(target));
        }

        //connects two message blocks or compound message blocks both shared ptrs
        template<typename source_type, typename target_type>
        inline void disconnect(std::shared_ptr<source_type>& source_ptr, std::shared_ptr<target_type>& target_ptr)
        {
            details::disconnect_impl(as_ref<source_type>(*source_ptr.get()), as_ref<target_type>(*target_ptr.get()));
        }

        //connects two message blocks or compound message blocks target is shared_ptr
        template<typename source_type, typename target_type>
        inline void disconnect(source_type& source, std::shared_ptr<target_type>& target_ptr)
        {
            details::disconnect_impl(as_ref<source_type>(source), as_ref<target_type>(*target_ptr.get()));
        }

        //disconnects all connected targets
        template<typename source_type>
        inline void disconnect(source_type& source)
        {
            details::disconnect_all_impl(as_ref<source_type>(source));
        }

        //disconnects a message block that is a shared_ptr
        template<typename source_type>
        inline void disconnect(std::shared_ptr<source_type>& source_ptr)
        {
            details::disconnect_all_impl(as_ref<source_type>(*source_ptr.get()));
        }
}