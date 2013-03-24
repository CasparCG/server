/*
    Copyright 2005-2011 Intel Corporation.  All Rights Reserved.

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

#ifndef __TBB_graph_H
#define __TBB_graph_H

#if !TBB_PREVIEW_GRAPH
#error Set TBB_PREVIEW_GRAPH to include graph.h
#endif

#include "tbb_stddef.h"
#include "atomic.h"
#include "spin_mutex.h"
#include "null_mutex.h"
#include "spin_rw_mutex.h"
#include "null_rw_mutex.h"
#include "task.h"
#include "concurrent_vector.h"
#include "_aggregator_internal.h"

// use the VC10 or gcc version of tuple if it is available.
#if TBB_IMPLEMENT_CPP0X && (!defined(_MSC_VER) || _MSC_VER < 1600)
#define TBB_PREVIEW_TUPLE 1
#include "compat/tuple"
#else
#include <tuple>
#endif

#include<list>
#include<queue>

/** @file
  \brief The graph related classes and functions

  There are some applications that best express dependencies as messages
  passed between nodes in a graph.  These messages may contain data or
  simply act as signals that a predecessors has completed. The graph
  class and its associated node classes can be used to express such
  applcations.
*/

namespace tbb {

    //! The base of all graph nodes.  Allows them to be stored in a collection for deletion.
    class graph_node {
    public:
        virtual ~graph_node() {} 
    }; 

    //! An empty class used for messages that mean "I'm done" 
    class continue_msg {};

    template< typename T > class sender;
    template< typename T > class receiver;
    class continue_receiver;

    //! Pure virtual template class that defines a sender of messages of type T
    template< typename T >
    class sender {
    public:
        //! The output type of this sender
        typedef T output_type;

        //! The successor type for this node
        typedef receiver<T> successor_type;

        virtual ~sender() {}

        //! Add a new successor to this node
        virtual bool register_successor( successor_type &r ) = 0;

        //! Removes a successor from this node
        virtual bool remove_successor( successor_type &r ) = 0;

        //! Request an item from the sender
        virtual bool try_get( T & ) { return false; }

        //! Reserves an item in the sender 
        virtual bool try_reserve( T & ) { return false; }

        //! Releases the reserved item
        virtual bool try_release( ) { return false; }

        //! Consumes the reserved item
        virtual bool try_consume( ) { return false; }

    };


    //! Pure virtual template class that defines a receiver of messages of type T
    template< typename T >
    class receiver {
    public:

        //! The input type of this receiver
        typedef T input_type;

        //! The predecessor type for this node
        typedef sender<T> predecessor_type;

        //! Destructor
        virtual ~receiver() {}

        //! Put an item to the receiver
        virtual bool try_put( T t ) = 0;

        //! Add a predecessor to the node
        virtual bool register_predecessor( predecessor_type & ) { return false; }

        //! Remove a predecessor from the node
        virtual bool remove_predecessor( predecessor_type & ) { return false; }

    };

    //! Base class for receivers of completion messages
    /** These receivers automatically reset, but cannot be explicitly waited on */
    class continue_receiver : public receiver< continue_msg > {
    public:

        //! The input type
        typedef continue_msg input_type;

        //! The predecessor type for this node
        typedef sender< continue_msg > predecessor_type;

        //! Constructor
        continue_receiver( int number_of_predecessors = 0 ) { 
            my_predecessor_count = number_of_predecessors;
            my_current_count = 0;
        }

        //! Destructor
        virtual ~continue_receiver() { }

        //! Increments the trigger threshold
        /* override */ bool register_predecessor( predecessor_type & ) {
            spin_mutex::scoped_lock l(my_mutex);
            ++my_predecessor_count;
            return true;
        }

        //! Decrements the trigger threshold
        /** Does not check to see if the removal of the predecessor now makes the current count
            exceed the new threshold.  So removing a predecessor while the graph is active can cause
            unexpected results. */
        /* override */ bool remove_predecessor( predecessor_type & ) {
            spin_mutex::scoped_lock l(my_mutex);
            --my_predecessor_count;
            return true;
        }

        //! Puts a continue_msg to the receiver
        /** If the message causes the message count to reach the predecessor count, execute() is called and
            the message count is reset to 0.  Otherwise the message count is incremented. */
        /* override */ bool try_put( input_type ) {
            {
                spin_mutex::scoped_lock l(my_mutex);
                if ( ++my_current_count < my_predecessor_count ) 
                    return true;
                else
                    my_current_count = 0;
            }
            execute();
            return true;
        }

    protected:

        spin_mutex my_mutex;
        int my_predecessor_count;
        int my_current_count;

        //! Does whatever should happen when the threshold is reached
        /** This should be very fast or else spawn a task.  This is
            called while the sender is blocked in the try_put(). */
        virtual void execute() = 0;

    };

    //! @cond INTERNAL
    namespace internal {

        //! The state of an executable node
        enum node_state { node_state_idle=0, node_state_nonidle=1, node_state_inactive=2 };


        //! A functor that takes no input and generates a value of type Output
        template< typename Output >
        class source_body : no_assign   {
        public:
            virtual ~source_body() {}
            virtual bool operator()(Output &output) = 0;
        };

        //! The leaf for source_body
        template< typename Output, typename Body>
        class source_body_leaf : public source_body<Output> {
        public:
            source_body_leaf( Body _body ) : body(_body) { }
            /*override */ bool operator()(Output &output) { return body( output ); }
        private:
            Body body;
        };

        //! A functor that takes an Input and generates an Output
        template< typename Input, typename Output >
            class function_body : no_assign {
        public:
            virtual ~function_body() {}
            virtual Output operator()(Input input) = 0;
        };

        //! the leaf for function_body
        template <typename Input, typename Output, typename B>
        class function_body_leaf : public function_body< Input, Output > {
        public:
            function_body_leaf( B _body ) : body(_body) { }
            Output operator()(Input i) { return body(i); }

        private:
            B body;
        };

        //! the leaf for function_body specialized for Input and output of continue_msg
        template <typename B>
        class function_body_leaf< continue_msg, continue_msg, B> : public function_body< continue_msg, continue_msg > {
        public:
            function_body_leaf( B _body ) : body(_body) { }
            continue_msg operator()( continue_msg i ) { 
                body(i); 
                return i; 
            }

        private:
            B body;
        };

        //! the leaf for function_body specialized for Output of continue_msg
        template <typename Input, typename B>
        class function_body_leaf< Input, continue_msg, B> : public function_body< Input, continue_msg > {
        public:
            function_body_leaf( B _body ) : body(_body) { }
            continue_msg operator()(Input i) { 
                body(i); 
                return continue_msg();
            }

        private:
            B body;
        };

        //! the leaf for function_body specialized for Input of continue_msg
        template <typename Output, typename B>
        class function_body_leaf< continue_msg, Output, B > : public function_body< continue_msg, Output > {
        public:
            function_body_leaf( B _body ) : body(_body) { }
            Output operator()(continue_msg i) { 
                return body(i); 
            }

        private:
            B body;
        };

        //! A task that calls a node's forward function
        template< typename NodeType >
        class forward_task : public task {

            NodeType &my_node;

        public:

            forward_task( NodeType &n ) : my_node(n) {}

            task *execute() {
                my_node.forward();
                return NULL;
            }
        };

        //! A task that calls a node's apply_body function, passing in an input of type Input
        template< typename NodeType, typename Input >
        class apply_body_task : public task {

            NodeType &my_node;
            Input my_input;

        public:

            apply_body_task( NodeType &n, Input i ) : my_node(n), my_input(i) {}

            task *execute() {
                my_node.apply_body( my_input );
                return NULL;
            }
        };

        //! A task that calls a node's apply_body function with no input
        template< typename NodeType >
        class source_task : public task {

            NodeType &my_node;

        public:

            source_task( NodeType &n ) : my_node(n) {}

            task *execute() {
                my_node.apply_body( );
                return NULL;
            }
        };

        //! An empty functor that takes an Input and returns a default constructed Output
        template< typename Input, typename Output >
        struct empty_body {
           Output operator()( const Input & ) const { return Output(); } 
        };

        //! A node_cache maintains a std::queue of elements of type T.  Each operation is protected by a lock. 
        template< typename T, typename M=spin_mutex >
        class node_cache {
            public:

            typedef size_t size_type;

            bool empty() {
                typename my_mutex_type::scoped_lock lock( my_mutex );
                return internal_empty();
            }

            void add( T &n ) {
                typename my_mutex_type::scoped_lock lock( my_mutex );
                internal_push(n);
            }

            void remove( T &n ) {
                typename my_mutex_type::scoped_lock lock( my_mutex );
                for ( size_t i = internal_size(); i != 0; --i ) {
                    T &s = internal_pop();
                    if ( &s != &n ) {
                        internal_push(s);
                    }
                }
            }

        protected:

            typedef M my_mutex_type;
            my_mutex_type my_mutex;
            std::queue< T * > my_q;

            // Assumes lock is held
            inline bool internal_empty( )  {
                return my_q.empty();
            }

            // Assumes lock is held
            inline size_type internal_size( )  {
                return my_q.size(); 
            }

            // Assumes lock is held
            inline void internal_push( T &n )  {
                my_q.push(&n);
            }

            // Assumes lock is held
            inline T &internal_pop() {
                T *v = my_q.front();
                my_q.pop();
                return *v;
            }

        };

        //! A cache of predecessors that only supports try_get
        template< typename T, typename M=spin_mutex >
        class predecessor_cache : public node_cache< sender<T>, M > {
            public:
            typedef M my_mutex_type;
            typedef T output_type; 
            typedef sender<output_type> predecessor_type;
            typedef receiver<output_type> successor_type;

            predecessor_cache( ) : my_owner( NULL ) { }

            void set_owner( successor_type *owner ) { my_owner = owner; }

            bool get_item( output_type &v ) {

                bool msg = false;

                do {
                    predecessor_type *src;
                    {
                        typename my_mutex_type::scoped_lock lock(this->my_mutex);
                        if ( this->internal_empty() ) {
                            break;
                        }
                        src = &this->internal_pop();
                    }

                    // Try to get from this sender
                    msg = src->try_get( v );

                    if (msg == false) {
                        // Relinquish ownership of the edge
                        if ( my_owner) 
                            src->register_successor( *my_owner );
                    } else {
                        // Retain ownership of the edge
                        this->add(*src);
                    }
                } while ( msg == false );
                return msg;
            }

        protected:
            successor_type *my_owner;
        };

        //! An cache of predecessors that supports requests and reservations
        template< typename T, typename M=spin_mutex >
        class reservable_predecessor_cache : public predecessor_cache< T, M > {
        public:
            typedef M my_mutex_type;
            typedef T output_type; 
            typedef sender<T> predecessor_type;
            typedef receiver<T> successor_type;

            reservable_predecessor_cache( ) : reserved_src(NULL) { }

            bool 
            try_reserve( output_type &v ) {
                bool msg = false;

                do {
                    {
                        typename my_mutex_type::scoped_lock lock(this->my_mutex);
                        if ( reserved_src || this->internal_empty() ) 
                            return false;

                        reserved_src = &this->internal_pop();
                    }

                    // Try to get from this sender
                    msg = reserved_src->try_reserve( v );

                    if (msg == false) {
                        typename my_mutex_type::scoped_lock lock(this->my_mutex);
                        // Relinquish ownership of the edge
                        reserved_src->register_successor( *this->my_owner );
                        reserved_src = NULL;
                    } else {
                        // Retain ownership of the edge
                        this->add( *reserved_src );
                    }
                } while ( msg == false );

                return msg;
            }

            bool 
            try_release( ) {
                reserved_src->try_release( );
                reserved_src = NULL;
                return true;
            }

            bool 
            try_consume( ) {
                reserved_src->try_consume( );
                reserved_src = NULL;
                return true;
            }

        private:
            predecessor_type *reserved_src;
        };


        //! An abstract cache of succesors
        template<typename T, typename M=spin_rw_mutex >
        class successor_cache : no_copy {
        protected:

            typedef M my_mutex_type;
            my_mutex_type my_mutex;

            typedef std::list< receiver<T> * > my_successors_type;
            my_successors_type my_successors;

            sender<T> *my_owner;

        public:

            successor_cache( ) : my_owner(NULL) {}

            void set_owner( sender<T> *owner ) { my_owner = owner; }

            virtual ~successor_cache() {}

            void register_successor( receiver<T> &r ) {
                typename my_mutex_type::scoped_lock l(my_mutex, true);
                my_successors.push_back( &r ); 
            }

            void remove_successor( receiver<T> &r ) {
                typename my_mutex_type::scoped_lock l(my_mutex, true);
                for ( typename my_successors_type::iterator i = my_successors.begin();
                      i != my_successors.end(); ++i ) { 
                    if ( *i == & r ) { 
                        my_successors.erase(i);
                        break;
                    }
                }
            }

            bool empty() { 
                typename my_mutex_type::scoped_lock l(my_mutex, false);
                return my_successors.empty(); 
            }

            virtual bool try_put( T t ) = 0; 
         };

        //! An abstract cache of succesors, specialized to continue_msg
        template<>
        class successor_cache< continue_msg > : no_copy {
        protected:

            typedef spin_rw_mutex my_mutex_type;
            my_mutex_type my_mutex;

            typedef std::list< receiver<continue_msg> * > my_successors_type;
            my_successors_type my_successors;

            sender<continue_msg> *my_owner;

        public:

            successor_cache( ) : my_owner(NULL) {}

            void set_owner( sender<continue_msg> *owner ) { my_owner = owner; }

            virtual ~successor_cache() {}

            void register_successor( receiver<continue_msg> &r ) {
                my_mutex_type::scoped_lock l(my_mutex, true);
                my_successors.push_back( &r ); 
                if ( my_owner )
                    r.register_predecessor( *my_owner );
            }

            void remove_successor( receiver<continue_msg> &r ) {
                my_mutex_type::scoped_lock l(my_mutex, true);
                for ( my_successors_type::iterator i = my_successors.begin();
                      i != my_successors.end(); ++i ) { 
                    if ( *i == & r ) { 
                        if ( my_owner )
                            r.remove_predecessor( *my_owner );
                        my_successors.erase(i);
                        break;
                    }
                }
            }

            bool empty() { 
                my_mutex_type::scoped_lock l(my_mutex, false);
                return my_successors.empty(); 
            }

            virtual bool try_put( continue_msg t ) = 0; 

         };

        //! A cache of successors that are broadcast to
        template<typename T, typename M=spin_rw_mutex>
        class broadcast_cache : public successor_cache<T, M> {
            typedef M my_mutex_type;
            typedef std::list< receiver<T> * > my_successors_type;

        public:

            broadcast_cache( ) {}

            bool try_put( T t ) {
                bool msg = false;
                bool upgraded = false;
                typename my_mutex_type::scoped_lock l(this->my_mutex, false);
                typename my_successors_type::iterator i = this->my_successors.begin();
                while ( i != this->my_successors.end() ) {
                   if ( (*i)->try_put( t ) == true ) {
                       ++i;
                       msg = true;
                   } else {
                      if ( (*i)->register_predecessor(*this->my_owner) ) {
                          if (!upgraded) {
                              l.upgrade_to_writer();
                              upgraded = true;
                          }
                          i = this->my_successors.erase(i);
                      }
                      else {
                          ++i;
                      }
                   }
                }
                return msg;
            }
        };

        //! A cache of successors that are put in a round-robin fashion
        template<typename T, typename M=spin_rw_mutex >
        class round_robin_cache : public successor_cache<T, M> {
            typedef size_t size_type;
            typedef M my_mutex_type;
            typedef std::list< receiver<T> * > my_successors_type;

        public:

            round_robin_cache( ) {}

            size_type size() {
                typename my_mutex_type::scoped_lock l(this->my_mutex, false);
                return this->my_successors.size();
            }

            bool try_put( T t ) {
                bool upgraded = false;
                typename my_mutex_type::scoped_lock l(this->my_mutex, false);
                typename my_successors_type::iterator i = this->my_successors.begin();
                while ( i != this->my_successors.end() ) {
                   if ( (*i)->try_put( t ) ) {
                       return true;
                   } else {
                      if ( (*i)->register_predecessor(*this->my_owner) ) {
                          if (!upgraded) {
                              l.upgrade_to_writer();
                              upgraded = true;
                          }
                          i = this->my_successors.erase(i);
                      }
                      else {
                          ++i;
                      }
                   }
                }
                return false;
            }
        };

        template<typename T>
        class decrementer : public continue_receiver, internal::no_copy {

            T *my_node;

            void execute() {
                my_node->decrement_counter();
            }

        public:
           
            typedef continue_msg input_type;
            typedef continue_msg output_type;
            decrementer( int number_of_predecessors = 0 ) : continue_receiver( number_of_predecessors ) { }
            void set_owner( T *node ) { my_node = node; }
        };

    }
    //! @endcond INTERNAL


    //! The graph class
    /** This class serves as a handle to the graph */
    class graph : internal::no_copy {

        template< typename Body >
        class run_task : public task {
        public: 
            run_task( Body& body ) : my_body(body) {}
            task *execute() {
                my_body();
                return NULL;
            }
        private:
            Body my_body;
        };

        template< typename Receiver, typename Body >
        class run_and_put_task : public task {
        public: 
            run_and_put_task( Receiver &r, Body& body ) : my_receiver(r), my_body(body) {}
            task *execute() {
                my_receiver.try_put( my_body() );
                return NULL;
            }
        private:
            Receiver &my_receiver;
            Body my_body;
        };

    public:

        //! An enumeration the provides the two most common concurrency levels: unlimited and serial
        enum concurrency { unlimited = 0, serial = 1 };

        //! Constructs a graph withy no nodes.
        graph() : my_root_task( new ( task::allocate_root( ) ) empty_task ) {
            my_root_task->set_ref_count(1);
        }

        //! Destroys the graph.
        /** Calls wait_for_all on the graph, deletes all of the nodes appended by calls to add, and then 
            destroys the root task of the graph. */ 
        ~graph() {
            wait_for_all();
            my_root_task->set_ref_count(0);
            task::destroy( *my_root_task );
        }


        //! Used to register that an external entity may still interact with the graph.
        /** The graph will not return from wait_for_all until a matching number of decrement_wait_count calls
            is made. */
        void increment_wait_count() { 
            if (my_root_task)
                my_root_task->increment_ref_count();
        }

        //! Deregisters an external entity that may have interacted with the graph.
        /** The graph will not return from wait_for_all until all the number of decrement_wait_count calls
            matches the number of increment_wait_count calls. */
        void decrement_wait_count() { 
            if (my_root_task)
                my_root_task->decrement_ref_count(); 
        }

        //! Spawns a task that runs a body and puts its output to a specific receiver
        /** The task is spawned as a child of the graph. This is useful for running tasks 
            that need to block a wait_for_all() on the graph.  For example a one-off source. */
        template< typename Receiver, typename Body >
            void run( Receiver &r, Body body ) {
           task::enqueue( * new ( task::allocate_additional_child_of( *my_root_task ) ) 
               run_and_put_task< Receiver, Body >( r, body ) );
        }

        //! Spawns a task that runs a function object 
        /** The task is spawned as a child of the graph. This is useful for running tasks 
            that need to block a wait_for_all() on the graph. For example a one-off source. */
        template< typename Body >
        void run( Body body ) {
           task::enqueue( * new ( task::allocate_additional_child_of( *my_root_task ) ) 
               run_task< Body >( body ) );
        }

        //! Waits until the graph is idle and the number of decrement_wait_count calls equals the number of increment_wait_count calls.
        /** The waiting thread will go off and steal work while it is block in the wait_for_all. */
        void wait_for_all() {
            if (my_root_task)
                my_root_task->wait_for_all();
            my_root_task->set_ref_count(1);
        }

        //! Returns the root task of the graph
        task * root_task() {
            return my_root_task;
        }

    private:

        task *my_root_task;

    };


    //! @cond INTERNAL
    namespace internal {

        //! Implements methods for a function node that takes a type T as input
        template< typename Input, typename Output >
        class function_input : public receiver<Input>, no_assign {
            typedef sender<Input> predecessor_type;
            enum op_stat {WAIT=0, SUCCEEDED, FAILED};
            enum op_type {reg_pred, rem_pred, app_body, tryput, try_fwd};
            typedef function_input<Input, Output> my_class;

        public:
            //! The input type of this receiver
            typedef Input input_type;
            //! The output type of this receiver
            typedef Output output_type;

            //! Constructor for function_input
            template< typename Body >
            function_input( graph &g, size_t max_concurrency, Body& body )
                : my_root_task(g.root_task()), my_max_concurrency(max_concurrency), my_concurrency(internal::node_state_idle),
                  my_body( new internal::function_body_leaf< input_type, output_type, Body>(body) ),
                forwarder_busy(false) {
                my_predecessors.set_owner(this);
                my_aggregator.initialize_handler(my_handler(this));
            }

            //! Destructor
            virtual ~function_input() { delete my_body; }

            //! Put to the node
            virtual bool try_put( input_type t ) {
               if ( my_max_concurrency == 0 ) {
                   spawn_body_task( t );
                   return true;
               } else {
                   my_operation op_data(t, tryput);
                   my_aggregator.execute(&op_data);
                   return op_data.status == SUCCEEDED;
               }
            }

            //! Adds src to the list of cached predecessors.
            /* override */ bool register_predecessor( predecessor_type &src ) {
                my_operation op_data(reg_pred);
                op_data.r = &src;
                my_aggregator.execute(&op_data);
                return true;
            }

            //! Removes src from the list of cached predecessors.
            /* override */ bool remove_predecessor( predecessor_type &src ) {
                my_operation op_data(rem_pred);
                op_data.r = &src;
                my_aggregator.execute(&op_data);
                return true;
            }

        protected:
            task *my_root_task;
            const size_t my_max_concurrency;
            size_t my_concurrency;
            function_body<input_type, output_type> *my_body;
            predecessor_cache<input_type, null_mutex > my_predecessors;

            virtual broadcast_cache<output_type > &successors() = 0;

        private:
            friend class apply_body_task< function_input< input_type, output_type >, input_type >;
            friend class forward_task< function_input< input_type, output_type > >;

            class my_operation : public aggregated_operation< my_operation > {
            public:
                char type;
                union {
                    input_type *elem;
                    predecessor_type *r;
                };
                my_operation(const input_type& e, op_type t) :
                    type(char(t)), elem(const_cast<input_type*>(&e)) {}
                my_operation(op_type t) : type(char(t)), r(NULL) {}
            };

            bool forwarder_busy;
            typedef internal::aggregating_functor<my_class, my_operation> my_handler;
            friend class internal::aggregating_functor<my_class, my_operation>;
            aggregator< my_handler, my_operation > my_aggregator;

            void handle_operations(my_operation *op_list) {
                my_operation *tmp;
                while (op_list) {
                    tmp = op_list;
                    op_list = op_list->next;
                    switch (tmp->type) {
                    case reg_pred:
                        my_predecessors.add(*(tmp->r));
                        __TBB_store_with_release(tmp->status, SUCCEEDED);
                        if (!forwarder_busy) {
                            forwarder_busy = true;
                            spawn_forward_task();
                        }
                        break;
                    case rem_pred:
                        my_predecessors.remove(*(tmp->r));
                        __TBB_store_with_release(tmp->status, SUCCEEDED);
                        break;
                    case app_body:
                        __TBB_ASSERT(my_max_concurrency != 0, NULL);
                        --my_concurrency;
                        __TBB_store_with_release(tmp->status, SUCCEEDED);
                        if (my_concurrency<my_max_concurrency) {
                            input_type i;
                            if (my_predecessors.get_item(i)) {
                                ++my_concurrency;
                                spawn_body_task(i);
                            }
                        }
                        break;
                    case tryput: internal_try_put(tmp);  break;
                    case try_fwd: internal_forward(tmp);  break;
                    }
                }
            }

            //! Put to the node
            void internal_try_put(my_operation *op) {
                __TBB_ASSERT(my_max_concurrency != 0, NULL);
                if (my_concurrency < my_max_concurrency) {
                   ++my_concurrency;
                   spawn_body_task(*(op->elem));
                   __TBB_store_with_release(op->status, SUCCEEDED);
               } else {
                   __TBB_store_with_release(op->status, FAILED);
               }
            }

            //! Tries to spawn bodies if available and if concurrency allows
            void internal_forward(my_operation *op) {
                if (my_concurrency<my_max_concurrency || !my_max_concurrency) {
                    input_type i;
                    if (my_predecessors.get_item(i)) {
                        ++my_concurrency;
                        __TBB_store_with_release(op->status, SUCCEEDED);
                        spawn_body_task(i);
                        return;
                    }
                }
                __TBB_store_with_release(op->status, FAILED);
                forwarder_busy = false;
            }

            //! Applies the body to the provided input
            void apply_body( input_type &i ) {
                successors().try_put( (*my_body)(i) );
                if ( my_max_concurrency != 0 ) {
                    my_operation op_data(app_body);
                    my_aggregator.execute(&op_data);
                }
            }

           //! Spawns a task that calls apply_body( input )
           inline void spawn_body_task( input_type &input ) {
               task::enqueue(*new(task::allocate_additional_child_of(*my_root_task)) apply_body_task<function_input<input_type, output_type>, input_type >(*this, input));
           }

           //! This is executed by an enqueued task, the "forwarder"
           void forward() {
               my_operation op_data(try_fwd);
               do {
                   op_data.status = WAIT;
                   my_aggregator.execute(&op_data);
               } while (op_data.status == SUCCEEDED);
           }

           //! Spawns a task that calls forward()
           inline void spawn_forward_task() {
               task::enqueue(*new(task::allocate_additional_child_of(*my_root_task)) forward_task<function_input<input_type, output_type> >(*this));
           }
        };

        //! Implements methods for an executable node that takes continue_msg as input
        template< typename Output >
        class continue_input : public continue_receiver {
        public:

            //! The input type of this receiver
            typedef continue_msg input_type;
    
            //! The output type of this receiver
            typedef Output output_type;

            template< typename Body >
            continue_input( graph &g, Body& body )
                : my_root_task(g.root_task()), 
                 my_body( new internal::function_body_leaf< input_type, output_type, Body>(body) ) { }

            template< typename Body >
            continue_input( graph &g, int number_of_predecessors, Body& body )
                : continue_receiver( number_of_predecessors ), my_root_task(g.root_task()), 
                 my_body( new internal::function_body_leaf< input_type, output_type, Body>(body) ) { }

        protected:

            task *my_root_task;
            function_body<input_type, output_type> *my_body;

            virtual broadcast_cache<output_type > &successors() = 0; 

            friend class apply_body_task< continue_input< Output >, continue_msg >;

            //! Applies the body to the provided input
            /* override */ void apply_body( input_type ) {
                successors().try_put( (*my_body)( continue_msg() ) );
            }

            //! Spawns a task that applies the body
            /* override */ void execute( ) {
                task::enqueue( * new ( task::allocate_additional_child_of( *my_root_task ) ) 
                   apply_body_task< continue_input< Output >, continue_msg >( *this, continue_msg() ) ); 
            }
        };

        //! Implements methods for both executable and function nodes that puts Output to its successors
        template< typename Output >
        class function_output : public sender<Output> {
        public:

            typedef Output output_type;

            function_output() { }

            //! Adds a new successor to this node
            /* override */ bool register_successor( receiver<output_type> &r ) {
                successors().register_successor( r );
                return true;
            }

            //! Removes a successor from this node
            /* override */ bool remove_successor( receiver<output_type> &r ) {
                successors().remove_successor( r );
                return true;
            }
  
        protected:

            virtual broadcast_cache<output_type > &successors() = 0; 

        };

    }
    //! @endcond INTERNAL

    //! An executable node that acts as a source, i.e. it has no predecessors
    template < typename Output >
    class source_node : public graph_node, public sender< Output > {
    public:

        //! The type of the output message, which is complete
        typedef Output output_type;           

        //! The type of successors of this node
        typedef receiver< Output > successor_type;

        //! Constructor for a node with a successor
        template< typename Body >
        source_node( graph &g, Body body, bool is_active = true )
             : my_root_task(g.root_task()), my_state( is_active ? internal::node_state_idle : internal::node_state_inactive ),
              my_body( new internal::source_body_leaf< output_type, Body>(body) ),
              my_reserved(false), my_has_cached_item(false) { 
            my_successors.set_owner(this);
        }

        //! The destructor
        ~source_node() { delete my_body; }

        //! Add a new successor to this node
        /* override */ bool register_successor( receiver<output_type> &r ) {
            spin_mutex::scoped_lock lock(my_mutex);
            my_successors.register_successor(r);
            if ( my_state != internal::node_state_inactive )
                spawn_put();
            return true;
        }

        //! Removes a successor from this node
        /* override */ bool remove_successor( receiver<output_type> &r ) {
            spin_mutex::scoped_lock lock(my_mutex);
            my_successors.remove_successor(r);
            return true;
        }

        //! Request an item from the node
        /*override */ bool try_get( output_type &v ) {
            spin_mutex::scoped_lock lock(my_mutex);
            if ( my_reserved )  
                return false;

            if ( my_has_cached_item ) {
                v = my_cached_item;
                my_has_cached_item = false;
            } else if ( (*my_body)(v) == false ) {
                return false;
            }
            return true;
        }

        //! Reserves an item.
        /* override */ bool try_reserve( output_type &v ) {
            spin_mutex::scoped_lock lock(my_mutex);
            if ( my_reserved ) {
                return false;
            }

            if ( !my_has_cached_item && (*my_body)(my_cached_item) )  
                my_has_cached_item = true;

            if ( my_has_cached_item ) {
                v = my_cached_item;
                my_reserved = true;
                return true;
            } else {
                return false;
            }
        }

        //! Release a reserved item.  
        /**  true = item has been released and so remains in sender, dest must request or reserve future items */
        /* override */ bool try_release( ) {
            spin_mutex::scoped_lock lock(my_mutex);
            __TBB_ASSERT( my_reserved && my_has_cached_item, "releasing non-existent reservation" );
            my_reserved = false;
            spawn_put();
            return true;
        }

        //! Consumes a reserved item
        /* override */ bool try_consume( ) {
            spin_mutex::scoped_lock lock(my_mutex);
            __TBB_ASSERT( my_reserved && my_has_cached_item, "consuming non-existent reservation" );
            my_reserved = false;
            my_has_cached_item = false;
            if ( !my_successors.empty() ) {
                spawn_put();
            }
            return true;
        }

        //! Activates a node that was created in the inactive state
        void activate() {
            spin_mutex::scoped_lock lock(my_mutex);
            my_state = internal::node_state_idle;
            if ( !my_successors.empty() )
                spawn_put();
        }

    private:

        task *my_root_task;
        spin_mutex my_mutex;
        internal::node_state my_state;
        internal::source_body<output_type> *my_body;
        internal::broadcast_cache< output_type > my_successors;
        bool my_reserved;
        bool my_has_cached_item;
        output_type my_cached_item;

        friend class internal::source_task< source_node< output_type > >;

        //! Applies the body
        /* override */ void apply_body( ) {
            output_type v;
            if ( try_reserve(v) == false )
                return;

            if ( my_successors.try_put( v ) ) 
                try_consume();
            else
                try_release();
        }

        //! Spawns a task that applies the body
        /* override */ void spawn_put( ) {
            task::enqueue( * new ( task::allocate_additional_child_of( *my_root_task ) ) 
               internal::source_task< source_node< output_type > >( *this ) ); 
        }

    };

    //! Implements a function node that supports Input -> Output
    template <typename Input, typename Output = continue_msg >
    class function_node : public graph_node, public internal::function_input<Input,Output>, public internal::function_output<Output> {
    public:

        typedef Input input_type;
        typedef Output output_type;
        typedef sender< input_type > predecessor_type;
        typedef receiver< output_type > successor_type;

        //! Constructor
        template< typename Body >
        function_node( graph &g, size_t concurrency, Body body )
        : internal::function_input<input_type,output_type>( g, concurrency, body ) {
            my_successors.set_owner(this);
        }

    protected:

        internal::broadcast_cache<output_type> my_successors; 
        /* override */ internal::broadcast_cache<output_type> &successors () { return my_successors; }

    };

    //! Implements an executable node that supports continue_msg -> Output
    template <typename Output>
    class executable_node : public graph_node, public internal::continue_input<Output>, public internal::function_output<Output> {
    public:

        typedef continue_msg input_type;
        typedef Output output_type;
        typedef sender< input_type > predecessor_type;
        typedef receiver< output_type > successor_type;

         //! Constructor for executable node with continue_msg -> Output
         template <typename Body >
         executable_node( graph &g, Body body )
                 : internal::continue_input<output_type>( g, body ) {
             my_successors.set_owner(this);
         }

         //! Constructor for executable node with continue_msg -> Output
         template <typename Body >
         executable_node( graph &g, int number_of_predecessors, Body body )
                 : internal::continue_input<output_type>( g, number_of_predecessors, body ) {
             my_successors.set_owner(this);
         }

    protected:

        internal::broadcast_cache<output_type> my_successors; 
        /* override */ internal::broadcast_cache<output_type> &successors () { return my_successors; }

    };



    template< typename T >
    class overwrite_node : public graph_node, public receiver<T>, public sender<T>, internal::no_copy {
    public:

        typedef T input_type;
        typedef T output_type;
        typedef sender< input_type > predecessor_type;
        typedef receiver< output_type > successor_type;

        overwrite_node() : my_buffer_is_valid(false) {
            my_successors.set_owner( this );
        }

        ~overwrite_node() {}

        /* override */ bool register_successor( successor_type &s ) {
            spin_mutex::scoped_lock l( my_mutex );
            if ( my_buffer_is_valid ) {
                // We have a valid value that must be forwarded immediately.
                if ( s.try_put( my_buffer ) || !s.register_predecessor( *this  ) ) {
                    // We add the successor: it accepted our put or it rejected it but won't let use become a predecessor
                    my_successors.register_successor( s );
                    return true;
                } else {
                    // We don't add the successor: it rejected our put and we became its predecessor instead
                    return false;
                }
            } else {
                // No valid value yet, just add as successor
                my_successors.register_successor( s );
                return true;
            }
        }

        /* override */ bool remove_successor( successor_type &s ) {
            spin_mutex::scoped_lock l( my_mutex );
            my_successors.remove_successor(s);
            return true;
        }

        /* override */ bool try_put( T v ) {
            spin_mutex::scoped_lock l( my_mutex );
            my_buffer = v;
            my_buffer_is_valid = true;
            my_successors.try_put(v);
            return true;
        }

        /* override */ bool try_get( T &v ) {
            spin_mutex::scoped_lock l( my_mutex );
            if ( my_buffer_is_valid ) {
                v = my_buffer;
                return true;
            } else {
                return false;
            }
        }

        bool is_valid() {
           spin_mutex::scoped_lock l( my_mutex );
           return my_buffer_is_valid;
        }

        void clear() {
           spin_mutex::scoped_lock l( my_mutex );
           my_buffer_is_valid = false;
        }

    protected:

        spin_mutex my_mutex;
        internal::broadcast_cache< T, null_rw_mutex > my_successors;
        T my_buffer;
        bool my_buffer_is_valid;

    };

    template< typename T >
    class write_once_node : public overwrite_node<T> {
    public:

        typedef T input_type;
        typedef T output_type;
        typedef sender< input_type > predecessor_type;
        typedef receiver< output_type > successor_type;

        /* override */ bool try_put( T v ) {
            spin_mutex::scoped_lock l( this->my_mutex );
            if ( this->my_buffer_is_valid ) {
                return false;
            } else {
                this->my_buffer = v;
                this->my_buffer_is_valid = true;
                this->my_successors.try_put(v);
                return true;
            }
        }
    };

    //! Broadcasts completion message when it receives completion messages from all predecessors. Then resets.
    /** Is equivalent to an executable_node< continue_msg > with an empty_body */
    class continue_node : public executable_node< continue_msg > { 
    public:

        typedef continue_msg input_type;
        typedef continue_msg output_type;
        typedef sender< input_type > predecessor_type;
        typedef receiver< output_type > successor_type;

        continue_node( graph &g ) : executable_node<continue_msg>( g, internal::empty_body< continue_msg, continue_msg>() ) {}
    };

    //! Forwards messages of type T to all successors
    template <typename T>
    class broadcast_node : public graph_node, public receiver<T>, public sender<T>, internal::no_copy {

        internal::broadcast_cache<T> my_successors;

    public:

        typedef T input_type;
        typedef T output_type;
        typedef sender< input_type > predecessor_type;
        typedef receiver< output_type > successor_type;

        broadcast_node( ) {
           my_successors.set_owner( this ); 
        }

        //! Adds a successor
        virtual bool register_successor( receiver<T> &r ) {
            my_successors.register_successor( r );
            return true;
        }

        //! Removes s as a successor
        virtual bool remove_successor( receiver<T> &r ) {
            my_successors.remove_successor( r );
            return true;
        }

        /* override */ bool try_put( T t ) {
            my_successors.try_put(t);
            return true;
        }

    };

#include "_item_buffer.h"

    //! Forwards messages in arbitrary order
    template <typename T, typename A=cache_aligned_allocator<T> >
        class buffer_node : public graph_node, public reservable_item_buffer<T, A>, public receiver<T>, public sender<T>, internal::no_copy {
    public:
        typedef T input_type;
        typedef T output_type;
        typedef sender< input_type > predecessor_type;
        typedef receiver< output_type > successor_type;
        typedef buffer_node<T, A> my_class;
    protected:
        typedef size_t size_type;
        internal::round_robin_cache< T, null_rw_mutex > my_successors;

        task *my_parent;

        friend class internal::forward_task< buffer_node< T, A > >;

        enum op_type {reg_succ, rem_succ, req_item, res_item, rel_res, con_res, put_item, try_fwd};
        enum op_stat {WAIT=0, SUCCEEDED, FAILED};

        // implements the aggregator_operation concept
        class buffer_operation : public internal::aggregated_operation< buffer_operation > {
        public:
            char type;
            T *elem;
            successor_type *r;
            buffer_operation(const T& e, op_type t) :
                type(char(t)), elem(const_cast<T*>(&e)), r(NULL) {}
            buffer_operation(op_type t) : type(char(t)), r(NULL) {}
        };

        bool forwarder_busy;
        typedef internal::aggregating_functor<my_class, buffer_operation> my_handler;
        friend class internal::aggregating_functor<my_class, buffer_operation>;
        internal::aggregator< my_handler, buffer_operation> my_aggregator;

        virtual void handle_operations(buffer_operation *op_list) {
            buffer_operation *tmp;
            bool try_forwarding=false;
            while (op_list) {
                tmp = op_list;
                op_list = op_list->next;
                switch (tmp->type) {
                case reg_succ: internal_reg_succ(tmp);  try_forwarding = true; break;
                case rem_succ: internal_rem_succ(tmp); break;
                case req_item: internal_pop(tmp); break;
                case res_item: internal_reserve(tmp); break;
                case rel_res:  internal_release(tmp);  try_forwarding = true; break;
                case con_res:  internal_consume(tmp);  try_forwarding = true; break;
                case put_item: internal_push(tmp);  try_forwarding = true; break;
                case try_fwd:  internal_forward(tmp); break;
                }
            }
            if (try_forwarding && !forwarder_busy) {
                forwarder_busy = true;
                task::enqueue(*new(task::allocate_additional_child_of(*my_parent)) internal::forward_task< buffer_node<input_type, A> >(*this));
            }
        }

        //! This is executed by an enqueued task, the "forwarder"
        virtual void forward() {
            buffer_operation op_data(try_fwd);
            do {
                op_data.status = WAIT;
                my_aggregator.execute(&op_data);
            } while (op_data.status == SUCCEEDED);
        }

        //! Register successor
        virtual void internal_reg_succ(buffer_operation *op) {
            my_successors.register_successor(*(op->r));
            __TBB_store_with_release(op->status, SUCCEEDED);
        }

        //! Remove successor
        virtual void internal_rem_succ(buffer_operation *op) {
            my_successors.remove_successor(*(op->r));
            __TBB_store_with_release(op->status, SUCCEEDED);
        }

        //! Tries to forward valid items to successors
        virtual void internal_forward(buffer_operation *op) {
            T i_copy;
            bool success = false; // flagged when a successor accepts
            size_type counter = my_successors.size();
            // Try forwarding, giving each successor a chance
            while (counter>0 && !this->buffer_empty() && this->item_valid(this->my_tail-1)) {
                this->fetch_back(i_copy);
                if( my_successors.try_put(i_copy) ) {
                    this->invalidate_back();
                    --(this->my_tail);
                    success = true; // found an accepting successor
                }
                --counter;
            }
            if (success && !counter)
                __TBB_store_with_release(op->status, SUCCEEDED);
            else {
                __TBB_store_with_release(op->status, FAILED);
                forwarder_busy = false;
            }
        }

        virtual void internal_push(buffer_operation *op) {
            this->push_back(*(op->elem));
            __TBB_store_with_release(op->status, SUCCEEDED);
        }

        virtual void internal_pop(buffer_operation *op) {
            if(this->pop_back(*(op->elem))) {
                __TBB_store_with_release(op->status, SUCCEEDED);
            }
            else {
                __TBB_store_with_release(op->status, FAILED);
            }
        }

        virtual void internal_reserve(buffer_operation *op) {
            if(this->reserve_front(*(op->elem))) {
                __TBB_store_with_release(op->status, SUCCEEDED);
            }
            else {
                __TBB_store_with_release(op->status, FAILED);
            }
        }

        virtual void internal_consume(buffer_operation *op) {
            this->consume_front();
            __TBB_store_with_release(op->status, SUCCEEDED);
        }

        virtual void internal_release(buffer_operation *op) {
            this->release_front();
            __TBB_store_with_release(op->status, SUCCEEDED);
        }

    public:
        //! Constructor
        buffer_node( graph &g ) : reservable_item_buffer<T>(),
            my_parent( g.root_task() ), forwarder_busy(false) {
            my_successors.set_owner(this);
            my_aggregator.initialize_handler(my_handler(this));
        }

        virtual ~buffer_node() {}

        //
        // message sender implementation
        //

        //! Adds a new successor.
        /** Adds successor r to the list of successors; may forward tasks.  */
        /* override */ bool register_successor( receiver<output_type> &r ) {
            buffer_operation op_data(reg_succ);
            op_data.r = &r;
            my_aggregator.execute(&op_data);
            return true;
        }

        //! Removes a successor.
        /** Removes successor r from the list of successors.
            It also calls r.remove_predecessor(*this) to remove this node as a predecessor. */
        /* override */ bool remove_successor( receiver<output_type> &r ) {
            r.remove_predecessor(*this);
            buffer_operation op_data(rem_succ);
            op_data.r = &r;
            my_aggregator.execute(&op_data);
            return true;
        }

        //! Request an item from the buffer_node
        /**  true = v contains the returned item<BR>
             false = no item has been returned */
        /* override */ bool try_get( T &v ) {
            buffer_operation op_data(req_item);
            op_data.elem = &v;
            my_aggregator.execute(&op_data);
            return (op_data.status==SUCCEEDED);
        }

        //! Reserves an item.
        /**  false = no item can be reserved<BR>
             true = an item is reserved */
        /* override */ bool try_reserve( T &v ) {
            buffer_operation op_data(res_item);
            op_data.elem = &v;
            my_aggregator.execute(&op_data);
            return (op_data.status==SUCCEEDED);
        }

        //! Release a reserved item.
        /**  true = item has been released and so remains in sender */
        /* override */ bool try_release() {
            buffer_operation op_data(rel_res);
            my_aggregator.execute(&op_data);
            return true;
        }

        //! Consumes a reserved item.
        /** true = item is removed from sender and reservation removed */
        /* override */ bool try_consume() {
            buffer_operation op_data(con_res);
            my_aggregator.execute(&op_data);
            return true;
        }

        //! Receive an item
        /** true is always returned */
        /* override */ bool try_put(T t) {
            buffer_operation op_data(t, put_item);
            my_aggregator.execute(&op_data);
            return true;
        }
    };


    //! Forwards messages in FIFO order
    template <typename T, typename A=cache_aligned_allocator<T> >
    class queue_node : public buffer_node<T, A> {
    protected:
    typedef typename buffer_node<T, A>::size_type size_type;
    typedef typename buffer_node<T, A>::buffer_operation queue_operation;

        enum op_stat {WAIT=0, SUCCEEDED, FAILED};

        //! Tries to forward valid items to successors
        /* override */ void internal_forward(queue_operation *op) {
            T i_copy;
            bool success = false; // flagged when a successor accepts
            size_type counter = this->my_successors.size();
            if (this->my_reserved || !this->item_valid(this->my_head)){
                __TBB_store_with_release(op->status, FAILED);
                this->forwarder_busy = false;
                return;
            }
            // Keep trying to send items while there is at least one accepting successor
            while (counter>0 && this->item_valid(this->my_head)) {
                this->fetch_front(i_copy);
                if(this->my_successors.try_put(i_copy)) {
                     this->invalidate_front();
                     ++(this->my_head);
                    success = true; // found an accepting successor
                }
                --counter;
            }
            if (success && !counter)
                __TBB_store_with_release(op->status, SUCCEEDED);
            else {
                __TBB_store_with_release(op->status, FAILED);
                this->forwarder_busy = false;
            }
        }

        /* override */ void internal_pop(queue_operation *op) {
            if ( this->my_reserved || !this->item_valid(this->my_head)){
                __TBB_store_with_release(op->status, FAILED);
            }
            else {
                this->pop_front(*(op->elem));
                __TBB_store_with_release(op->status, SUCCEEDED);
            }
        }
        /* override */ void internal_reserve(queue_operation *op) {
            if (this->my_reserved || !this->item_valid(this->my_head)) {
                __TBB_store_with_release(op->status, FAILED);
            }
            else {
                this->my_reserved = true;
                this->fetch_front(*(op->elem));
                this->invalidate_front();
                __TBB_store_with_release(op->status, SUCCEEDED);
            }
        }
        /* override */ void internal_consume(queue_operation *op) {
            this->consume_front();
            __TBB_store_with_release(op->status, SUCCEEDED);
        }

    public:

        typedef T input_type;
        typedef T output_type;
        typedef sender< input_type > predecessor_type;
        typedef receiver< output_type > successor_type;

        //! Constructor
    queue_node( graph &g ) : buffer_node<T, A>(g) {}
    };

    //! Forwards messages in sequence order
    template< typename T, typename A=cache_aligned_allocator<T> >
    class sequencer_node : public queue_node<T, A> {
        internal::function_body< T, size_t > *my_sequencer;
    public:

        typedef T input_type;
        typedef T output_type;
        typedef sender< input_type > predecessor_type;
        typedef receiver< output_type > successor_type;

        //! Constructor
        template< typename Sequencer >
        sequencer_node( graph &g, const Sequencer& s ) : queue_node<T, A>(g),
            my_sequencer(new internal::function_body_leaf< T, size_t, Sequencer>(s) ) {}

        //! Destructor
        ~sequencer_node() { delete my_sequencer; }
    protected:
        typedef typename buffer_node<T, A>::size_type size_type;
        typedef typename buffer_node<T, A>::buffer_operation sequencer_operation;

        enum op_stat {WAIT=0, SUCCEEDED, FAILED};

    private:
        /* override */ void internal_push(sequencer_operation *op) {
            size_type tag = (*my_sequencer)(*(op->elem));

            this->my_tail = (tag+1 > this->my_tail) ? tag+1 : this->my_tail;

            if(this->size() > this->capacity())
                this->grow_my_array(this->size());  // tail already has 1 added to it
            this->item(tag) = std::make_pair( *(op->elem), true );
            __TBB_store_with_release(op->status, SUCCEEDED);
        }
    };

    //! Forwards messages in priority order
    template< typename T, typename Compare = std::less<T>, typename A=cache_aligned_allocator<T> >
    class priority_queue_node : public buffer_node<T, A> {
    public:
        typedef T input_type;
        typedef T output_type;
        typedef sender< input_type > predecessor_type;
        typedef receiver< output_type > successor_type;

        //! Constructor
    priority_queue_node( graph &g ) : buffer_node<T, A>(g), mark(0) {}

    protected:
        typedef typename buffer_node<T, A>::size_type size_type;
        typedef typename buffer_node<T, A>::item_type item_type;
        typedef typename buffer_node<T, A>::buffer_operation prio_operation;

        enum op_stat {WAIT=0, SUCCEEDED, FAILED};

        /* override */ void handle_operations(prio_operation *op_list) {
            prio_operation *tmp /*, *pop_list*/ ;
            bool try_forwarding=false;
            while (op_list) {
                tmp = op_list;
                op_list = op_list->next;
                switch (tmp->type) {
                case buffer_node<T, A>::reg_succ: this->internal_reg_succ(tmp); try_forwarding = true; break;
                case buffer_node<T, A>::rem_succ: this->internal_rem_succ(tmp); break;
                case buffer_node<T, A>::put_item: internal_push(tmp); try_forwarding = true; break;
                case buffer_node<T, A>::try_fwd: internal_forward(tmp); break;
                case buffer_node<T, A>::rel_res: internal_release(tmp); try_forwarding = true; break;
                case buffer_node<T, A>::con_res: internal_consume(tmp); try_forwarding = true; break;
                case buffer_node<T, A>::req_item: internal_pop(tmp); break;
                case buffer_node<T, A>::res_item: internal_reserve(tmp); break;
                }
            }
            // process pops!  for now, no special pop processing
            if (mark<this->my_tail) heapify();
            if (try_forwarding && !this->forwarder_busy) {
                this->forwarder_busy = true;
                task::enqueue(*new(task::allocate_additional_child_of(*(this->my_parent))) internal::forward_task< buffer_node<input_type, A> >(*this));
            }
        }

        //! Tries to forward valid items to successors
        /* override */ void internal_forward(prio_operation *op) {
            T i_copy;
            bool success = false; // flagged when a successor accepts
            size_type counter = this->my_successors.size();

            if (this->my_reserved || this->my_tail == 0) {
                __TBB_store_with_release(op->status, FAILED);
                this->forwarder_busy = false;
                return;
            }
            // Keep trying to send while there exists an accepting successor
            while (counter>0 && this->my_tail > 0) {
                i_copy = this->my_array[0].first;
                bool msg = this->my_successors.try_put(i_copy);
                if ( msg == true ) {
                     if (mark == this->my_tail) --mark;
                    --(this->my_tail);
                    this->my_array[0].first=this->my_array[this->my_tail].first;
                    if (this->my_tail > 1) // don't reheap for heap of size 1
                        reheap();
                    success = true; // found an accepting successor
                }
                --counter;
            }
            if (success && !counter)
                __TBB_store_with_release(op->status, SUCCEEDED);
            else {
                __TBB_store_with_release(op->status, FAILED);
                this->forwarder_busy = false;
            }
        }

        /* override */ void internal_push(prio_operation *op) {
            if ( this->my_tail >= this->my_array_size )
                this->grow_my_array( this->my_tail + 1 );
            this->my_array[this->my_tail] = std::make_pair( *(op->elem), true );
            ++(this->my_tail);
            __TBB_store_with_release(op->status, SUCCEEDED);
        }
        /* override */ void internal_pop(prio_operation *op) {
            if ( this->my_reserved == true || this->my_tail == 0 ) {
                __TBB_store_with_release(op->status, FAILED);
            }
            else {
                if (mark<this->my_tail &&
                    compare(this->my_array[0].first,
                            this->my_array[this->my_tail-1].first)) {
                    // there are newly pushed elems; last one higher than top
                    // copy the data
                    *(op->elem) = this->my_array[this->my_tail-1].first;
                    --(this->my_tail);
                    __TBB_store_with_release(op->status, SUCCEEDED);
                }
                else { // extract and push the last element down heap
                    *(op->elem) = this->my_array[0].first; // copy the data
                    if (mark == this->my_tail) --mark;
                    --(this->my_tail);
                    __TBB_store_with_release(op->status, SUCCEEDED);
                    this->my_array[0].first=this->my_array[this->my_tail].first;
                    if (this->my_tail > 1) // don't reheap for heap of size 1
                        reheap();
                }
            }
        }
        /* override */ void internal_reserve(prio_operation *op) {
            if (this->my_reserved == true || this->my_tail == 0) {
                __TBB_store_with_release(op->status, FAILED);
            }
            else {
                this->my_reserved = true;
                *(op->elem) = reserved_item = this->my_array[0].first;
                if (mark == this->my_tail) --mark;
                --(this->my_tail);
                __TBB_store_with_release(op->status, SUCCEEDED);
                this->my_array[0].first = this->my_array[this->my_tail].first;
                if (this->my_tail > 1) // don't reheap for heap of size 1
                    reheap();
            }
        }
        /* override */ void internal_consume(prio_operation *op) {
            this->my_reserved = false;
            __TBB_store_with_release(op->status, SUCCEEDED);
        }
        /* override */ void internal_release(prio_operation *op) {
            if (this->my_tail >= this->my_array_size)
                this->grow_my_array( this->my_tail + 1 );
            this->my_array[this->my_tail] = std::make_pair(reserved_item, true);
            ++(this->my_tail);
            this->my_reserved = false;
            __TBB_store_with_release(op->status, SUCCEEDED);
            heapify();
        }
    private:
        Compare compare;
        size_type mark;
        input_type reserved_item;

        void heapify() {
            if (!mark) mark = 1;
            for (; mark<this->my_tail; ++mark) { // for each unheaped element
                size_type cur_pos = mark;
                input_type to_place = this->my_array[mark].first;
                do { // push to_place up the heap
                    size_type parent = (cur_pos-1)>>1;
                    if (!compare(this->my_array[parent].first, to_place))
                        break;
                    this->my_array[cur_pos].first = this->my_array[parent].first;
                    cur_pos = parent;
                } while( cur_pos );
                this->my_array[cur_pos].first = to_place;
            }
        }

        void reheap() {
            size_type cur_pos=0, child=1;
            while (child < mark) {
                size_type target = child;
                if (child+1<mark &&
                    compare(this->my_array[child].first,
                            this->my_array[child+1].first))
                    ++target;
                // target now has the higher priority child
                if (compare(this->my_array[target].first,
                            this->my_array[this->my_tail].first))
                    break;
                this->my_array[cur_pos].first = this->my_array[target].first;
                cur_pos = target;
                child = (cur_pos<<1)+1;
            }
            this->my_array[cur_pos].first = this->my_array[this->my_tail].first;
        }
    };

    //! Forwards messages only if the threshold has not been reached
    /** This node forwards items until its threshold is reached.
        It contains no buffering.  If the downstream node rejects, the
        message is dropped. */
    template< typename T >
    class limiter_node : public graph_node, public receiver< T >, public sender< T >, internal::no_copy {
    public:

        typedef T input_type;
        typedef T output_type;
        typedef sender< input_type > predecessor_type;
        typedef receiver< output_type > successor_type;

    private:

        task *my_root_task;
        size_t my_threshold;
        size_t my_count;
        internal::predecessor_cache< T > my_predecessors;
        spin_mutex my_mutex;
        internal::broadcast_cache< T > my_successors;

        friend class internal::forward_task< limiter_node<T> >;

        // Let decrementer call decrement_counter()
        friend class internal::decrementer< limiter_node<T> >;

        void decrement_counter() {
            input_type v;
            
            // If we can't get / put an item immediately then drop the count
            if ( my_predecessors.get_item( v ) == false 
                 || my_successors.try_put(v) == false ) {
                spin_mutex::scoped_lock lock(my_mutex);
                --my_count;
                if ( !my_predecessors.empty() ) 
                    task::enqueue( * new ( task::allocate_additional_child_of( *my_root_task ) ) 
                                internal::forward_task< limiter_node<T> >( *this ) );
            }
        }

        void forward() {
            {
                spin_mutex::scoped_lock lock(my_mutex);
                if ( my_count < my_threshold ) 
                    ++my_count;
                else
                    return;
            }
            decrement_counter();
        }

    public:

        //! The internal receiver< continue_msg > that decrements the count
        internal::decrementer< limiter_node<T> > decrement;

        //! Constructor
        limiter_node( graph &g, size_t threshold, int number_of_decrement_predecessors = 0 ) : 
           my_root_task(g.root_task()), my_threshold(threshold), my_count(0), decrement(number_of_decrement_predecessors) {
            my_predecessors.set_owner(this);
            my_successors.set_owner(this);
            decrement.set_owner(this);
        }

        //! Replace the current successor with this new successor
        /* override */ bool register_successor( receiver<output_type> &r ) {
            my_successors.register_successor(r);
            return true;
        }

        //! Removes a successor from this node
        /** r.remove_predecessor(*this) is also called. */
        /* override */ bool remove_successor( receiver<output_type> &r ) {
            r.remove_predecessor(*this);
            my_successors.remove_successor(r);
            return true;
        }

        //! Puts an item to this receiver
        /* override */ bool try_put( T t ) {
            {
                spin_mutex::scoped_lock lock(my_mutex);
                if ( my_count >= my_threshold ) 
                    return false;
                else
                    ++my_count; 
            }

            bool msg = my_successors.try_put(t);

            if ( msg != true ) {
                spin_mutex::scoped_lock lock(my_mutex);
                --my_count;
                if ( !my_predecessors.empty() ) 
                    task::enqueue( * new ( task::allocate_additional_child_of( *my_root_task ) ) 
                                internal::forward_task< limiter_node<T> >( *this ) );
            }

            return msg;
        }

        //! Removes src from the list of cached predecessors.
        /* override */ bool register_predecessor( predecessor_type &src ) {
            spin_mutex::scoped_lock lock(my_mutex);
            my_predecessors.add( src );
            if ( my_count < my_threshold && !my_successors.empty() ) 
                task::enqueue( * new ( task::allocate_additional_child_of( *my_root_task ) ) 
                               internal::forward_task< limiter_node<T> >( *this ) );
            return true;
        }

        //! Removes src from the list of cached predecessors.
        /* override */ bool remove_predecessor( predecessor_type &src ) {
            my_predecessors.remove( src );
            return true;
        }

    };

    namespace internal {

    struct forwarding_base {
        forwarding_base(task *rt) : my_root_task(rt) {}
        virtual ~forwarding_base() {}
        virtual void decrement_port_count() = 0;
        virtual void increment_port_count() = 0;
        // moved here so input ports can queue tasks
        task* my_root_task;
    };

    template< int N >
    struct join_helper {

        template< typename TupleType, typename PortType >
        static inline void set_join_node_pointer(TupleType &my_input, PortType *port) {
            std::get<N-1>( my_input ).set_join_node_pointer(port);
            join_helper<N-1>::set_join_node_pointer( my_input, port );
        }
        template< typename TupleType >
        static inline void consume_reservations( TupleType &my_input ) {
            std::get<N-1>( my_input ).consume();
            join_helper<N-1>::consume_reservations( my_input );
        }

        template< typename TupleType >
        static inline void release_my_reservation( TupleType &my_input ) {
            std::get<N-1>( my_input ).release();
        }

        template <typename TupleType>
        static inline void release_reservations( TupleType &my_input) {
            join_helper<N-1>::release_reservations(my_input);
            release_my_reservation(my_input);
        }

        template< typename InputTuple, typename OutputTuple >
        static inline bool reserve( InputTuple &my_input, OutputTuple &out) {
            if ( !std::get<N-1>( my_input ).reserve( std::get<N-1>( out ) ) ) return false;
            if ( !join_helper<N-1>::reserve( my_input, out ) ) {
                release_my_reservation( my_input );
                return false;
            }
            return true;
        }

        template<typename InputTuple, typename OutputTuple>
        static inline bool get_my_item( InputTuple &my_input, OutputTuple &out) {
            bool res = std::get<N-1>(my_input).get_item(std::get<N-1>(out) ); // may fail
            return join_helper<N-1>::get_my_item(my_input, out) && res;       // do get on other inputs before returning
        }

        template<typename InputTuple, typename OutputTuple>
        static inline bool get_items(InputTuple &my_input, OutputTuple &out) {
            return get_my_item(my_input, out);
        }

        template<typename InputTuple>
        static inline void reset_my_port(InputTuple &my_input) {
            join_helper<N-1>::reset_my_port(my_input);
            std::get<N-1>(my_input).reset_port();
        }

        template<typename InputTuple>
        static inline void reset_ports(InputTuple& my_input) {
            reset_my_port(my_input);
        }
    };

    template< >
    struct join_helper<1> {

        template< typename TupleType, typename PortType >
        static inline void set_join_node_pointer(TupleType &my_input, PortType *port) {
            std::get<0>( my_input ).set_join_node_pointer(port);
        }

        template< typename TupleType >
        static inline void consume_reservations( TupleType &my_input ) {
            std::get<0>( my_input ).consume();
        }

        template< typename TupleType >
        static inline void release_my_reservation( TupleType &my_input ) {
            std::get<0>( my_input ).release();
        }
        
        template<typename TupleType>
        static inline void release_reservations( TupleType &my_input) {
            release_my_reservation(my_input);
        }

        template< typename InputTuple, typename OutputTuple >
        static inline bool reserve( InputTuple &my_input, OutputTuple &out) {
            return std::get<0>( my_input ).reserve( std::get<0>( out ) );
        }

        template<typename InputTuple, typename OutputTuple>
        static inline bool get_my_item( InputTuple &my_input, OutputTuple &out) {
            return std::get<0>(my_input).get_item(std::get<0>(out));
        }

        template<typename InputTuple, typename OutputTuple>
        static inline bool get_items(InputTuple &my_input, OutputTuple &out) {
            return get_my_item(my_input, out);
        }

        template<typename InputTuple>
        static inline void reset_my_port(InputTuple &my_input) {
            std::get<0>(my_input).reset_port();
        }

        template<typename InputTuple>
        static inline void reset_ports(InputTuple& my_input) {
            reset_my_port(my_input);
        }
    };

    namespace join_policy_namespace {
        enum join_policy { reserving
            , queueing
        };
    }
    using namespace join_policy_namespace;

    //! The two-phase join port
    template< typename T >
    class reserving_port : public receiver<T> {
    public:
        typedef T input_type;
        typedef sender<T> predecessor_type;
    private:
        // ----------- Aggregator ------------
        enum op_type { reg_pred, rem_pred, res_item, rel_res, con_res };
        enum op_stat {WAIT=0, SUCCEEDED, FAILED};
        typedef reserving_port<T> my_class;

        class reserving_port_operation : public aggregated_operation<reserving_port_operation> {
        public:
            char type;
            union {
                T *my_arg;
                predecessor_type *my_pred;
            };
            reserving_port_operation(const T& e, op_type t) :
                type(char(t)), my_arg(const_cast<T*>(&e)) {}
            reserving_port_operation(const predecessor_type &s, op_type t) : type(char(t)), 
                my_pred(const_cast<predecessor_type *>(&s)) {}
            reserving_port_operation(op_type t) : type(char(t)) {}
        };

        typedef internal::aggregating_functor<my_class, reserving_port_operation> my_handler;
        friend class internal::aggregating_functor<my_class, reserving_port_operation>;
        aggregator<my_handler, reserving_port_operation> my_aggregator;

        void handle_operations(reserving_port_operation* op_list) {
            reserving_port_operation *current;
            bool no_predecessors;
            while(op_list) {
                current = op_list;
                op_list = op_list->next;
                switch(current->type) {
                case reg_pred:
                    no_predecessors = my_predecessors.empty();
                    my_predecessors.add(*(current->my_pred));
                    if ( no_predecessors ) {
                        my_join->decrement_port_count( ); // may try to forward
                    }
                    __TBB_store_with_release(current->status, SUCCEEDED);
                    break;
                case rem_pred:
                    my_predecessors.remove(*(current->my_pred));
                    if(my_predecessors.empty()) my_join->increment_port_count();
                    __TBB_store_with_release(current->status, SUCCEEDED);
                    break;
                case res_item:
                    if ( reserved ) {
                        __TBB_store_with_release(current->status, FAILED);
                    }
                    else if ( my_predecessors.try_reserve( *(current->my_arg) ) ) {
                        reserved = true;
                        __TBB_store_with_release(current->status, SUCCEEDED);
                    } else {
                        if ( my_predecessors.empty() ) {
                            my_join->increment_port_count();
                        }
                        __TBB_store_with_release(current->status, FAILED);
                    }
                    break;
                case rel_res:
                    reserved = false;
                    my_predecessors.try_release( );
                    __TBB_store_with_release(current->status, SUCCEEDED);
                    break;
                case con_res:
                    reserved = false;
                    my_predecessors.try_consume( );
                    __TBB_store_with_release(current->status, SUCCEEDED);
                    break;
                }
            }
        }

    public:

        //! Constructor
        reserving_port() : reserved(false) {
            my_join = NULL;
            my_predecessors.set_owner( this );
            my_aggregator.initialize_handler(my_handler(this));
        }

        // copy constructor
        reserving_port(const reserving_port& /* other */) : receiver<T>() {
            reserved = false;
            my_join = NULL;
            my_predecessors.set_owner( this );
            my_aggregator.initialize_handler(my_handler(this));
        }

        void set_join_node_pointer(forwarding_base *join) {
            my_join = join;
        }

        // always rejects, so arc is reversed (and reserves can be done.)
        bool try_put( T ) {
            return false;
        }

        //! Add a predecessor
        bool register_predecessor( sender<T> &src ) {
            reserving_port_operation op_data(src, reg_pred);
            my_aggregator.execute(&op_data);
            return op_data.status == SUCCEEDED;
        }

        //! Remove a predecessor
        bool remove_predecessor( sender<T> &src ) {
            reserving_port_operation op_data(src, rem_pred);
            my_aggregator.execute(&op_data);
            return op_data.status == SUCCEEDED;
        }

        //! Reserve an item from the port
        bool reserve( T &v ) {
            reserving_port_operation op_data(v, res_item);
            my_aggregator.execute(&op_data);
            return op_data.status == SUCCEEDED;
        }

        //! Release the port
        void release( ) {
            reserving_port_operation op_data(rel_res);
            my_aggregator.execute(&op_data);
        }

        //! Complete use of the port
        void consume( ) {
            reserving_port_operation op_data(con_res);
            my_aggregator.execute(&op_data);
        }

    private:
        forwarding_base *my_join;
        reservable_predecessor_cache< T, null_mutex > my_predecessors;
        bool reserved;
    };

    //! queueing join_port
    template<typename T>
    class queueing_port : public receiver<T>, public item_buffer<T> {
    public:
        typedef T input_type;
        typedef sender<T> predecessor_type;
        typedef queueing_port<T> my_node_type;

// ----------- Aggregator ------------
    private:
        enum op_type { try__put, get__item, res_port };
        enum op_stat {WAIT=0, SUCCEEDED, FAILED};
        typedef queueing_port<T> my_class;

        class queueing_port_operation : public aggregated_operation<queueing_port_operation> {
        public:
            char type;
            union {
                T my_val;
                T *my_arg;
            };
            // constructor for value parameter
            queueing_port_operation(const T& e, op_type t) :
                // type(char(t)), my_val(const_cast<T>(e)) {}
                type(char(t)), my_val(e) {}
            // constructor for pointer parameter
            queueing_port_operation(const T* p, op_type t) :
                type(char(t)), my_arg(const_cast<T*>(p)) {}
            // constructor with no parameter
            queueing_port_operation(op_type t) : type(char(t)) {}
        };

        typedef internal::aggregating_functor<my_class, queueing_port_operation> my_handler;
        friend class internal::aggregating_functor<my_class, queueing_port_operation>;
        aggregator<my_handler, queueing_port_operation> my_aggregator;

        void handle_operations(queueing_port_operation* op_list) {
            queueing_port_operation *current;
            bool was_empty;
            while(op_list) {
                current = op_list;
                op_list = op_list->next;
                switch(current->type) {
                case try__put:
                    was_empty = this->buffer_empty();
                    this->push_back(current->my_val);
                    if (was_empty) my_join->decrement_port_count();
                    __TBB_store_with_release(current->status, SUCCEEDED);
                    break;
                case get__item:
                    if(!this->buffer_empty()) {
                        this->fetch_front(*(current->my_arg));
                        __TBB_store_with_release(current->status, SUCCEEDED);
                    }
                    else {
                        __TBB_store_with_release(current->status, FAILED);
                    }
                    break;
                case res_port:
                    __TBB_ASSERT(this->item_valid(this->my_head), "No item to reset");
                    this->invalidate_front(); ++(this->my_head);
                    if(this->item_valid(this->my_head)) {
                        my_join->decrement_port_count();
                    }
                    __TBB_store_with_release(current->status, SUCCEEDED);
                    break;
                }
            }
        }
// ------------ End Aggregator ---------------
    public:

        //! Constructor
        queueing_port() : item_buffer<T>() {
            my_join = NULL;
            my_aggregator.initialize_handler(my_handler(this));
        }

        //! copy constructor
        queueing_port(const queueing_port& /* other */) : receiver<T>(), item_buffer<T>() {
            my_join = NULL;
            my_aggregator.initialize_handler(my_handler(this));
        }

        //! record parent for tallying available items
        void set_join_node_pointer(forwarding_base *join) {
            my_join = join;
        }

        /*override*/bool try_put(T v) {
            queueing_port_operation op_data(v, try__put);
            my_aggregator.execute(&op_data);
            return op_data.status == SUCCEEDED;
        }


        bool get_item( T &v ) {
            queueing_port_operation op_data(&v, get__item);
            my_aggregator.execute(&op_data);
            return op_data.status == SUCCEEDED;
        }

        // reset_port is called when item is accepted by successor, but
        // is initiated by join_node.
        void reset_port() {
            queueing_port_operation op_data(res_port);
            my_aggregator.execute(&op_data);
            return;
        }

    private:
        forwarding_base *my_join;
    };

    template<join_policy JP, typename InputTuple, typename OutputTuple>
    class join_node_base;

    //! join_node_FE : implements input port policy
    template<join_policy JP, typename InputTuple, typename OutputTuple>
    class join_node_FE;

    template<typename InputTuple, typename OutputTuple>
    class join_node_FE<reserving, InputTuple, OutputTuple> : public forwarding_base {
    public:
        static const int N = std::tuple_size<OutputTuple>::value;
        typedef OutputTuple output_type;
        typedef InputTuple input_type;
        typedef join_node_base<reserving, InputTuple, OutputTuple> my_node_type; // for forwarding

        join_node_FE(graph &g) : forwarding_base(g.root_task()), my_node(NULL) {
            ports_with_no_inputs = N;
            join_helper<N>::set_join_node_pointer(my_inputs, this);
        }

        void set_my_node(my_node_type *new_my_node) { my_node = new_my_node; }

       void increment_port_count() {
            ++ports_with_no_inputs;
        }

        // if all input_ports have predecessors, spawn forward to try and consume tuples
        void decrement_port_count() {
            if(ports_with_no_inputs.fetch_and_decrement() == 1) {
                task::enqueue( * new ( task::allocate_additional_child_of( *(this->my_root_task) ) )
                    forward_task<my_node_type>(*my_node) );
            }
        }

        input_type &inputs() { return my_inputs; }
    protected:
        // all methods on input ports should be called under mutual exclusion from join_node_base.

        bool tuple_build_may_succeed() {
            return !ports_with_no_inputs;
        }

        bool try_to_make_tuple(output_type &out) {
            if(ports_with_no_inputs) return false;
            return join_helper<N>::reserve(my_inputs, out);
        }

        void tuple_accepted() {
            join_helper<N>::consume_reservations(my_inputs);
        }
        void tuple_rejected() {
            join_helper<N>::release_reservations(my_inputs);
        }

        input_type my_inputs;
        my_node_type *my_node;
        atomic<size_t> ports_with_no_inputs;
    };

    template<typename InputTuple, typename OutputTuple>
    class join_node_FE<queueing, InputTuple, OutputTuple> : public forwarding_base {
    public:
        static const int N = std::tuple_size<OutputTuple>::value;
        typedef OutputTuple output_type;
        typedef InputTuple input_type;
        typedef join_node_base<queueing, InputTuple, OutputTuple> my_node_type; // for forwarding

        join_node_FE(graph &g) : forwarding_base(g.root_task()), my_node(NULL) {
            ports_with_no_items = N;
            join_helper<N>::set_join_node_pointer(my_inputs, this);
        }

        // needed for forwarding
        void set_my_node(my_node_type *new_my_node) { my_node = new_my_node; }

        void reset_port_count() {
            ports_with_no_items = N;
        }

        // if all input_ports have items, spawn forward to try and consume tuples
        void decrement_port_count() {
            if(ports_with_no_items.fetch_and_decrement() == 1) {
                task::enqueue( * new ( task::allocate_additional_child_of( *(this->my_root_task) ) )
                    forward_task<my_node_type>(*my_node) );
            }
        }

        void increment_port_count() { __TBB_ASSERT(false, NULL); }  // should never be called

        input_type &inputs() { return my_inputs; }
    protected:
        // all methods on input ports should be called under mutual exclusion from join_node_base.

        bool tuple_build_may_succeed() {
            return !ports_with_no_items;
        }

        bool try_to_make_tuple(output_type &out) {
            if(ports_with_no_items) return false;
            return join_helper<N>::get_items(my_inputs, out);
        }

        void tuple_accepted() {
            reset_port_count();
            join_helper<N>::reset_ports(my_inputs);
        }
        void tuple_rejected() {
            // nothing to do.
        }

        input_type my_inputs;
        my_node_type *my_node;
        atomic<size_t> ports_with_no_items;
    };

    //! join_node_base
    template<join_policy JP, typename InputTuple, typename OutputTuple>
    class join_node_base : public graph_node, public join_node_FE<JP, InputTuple, OutputTuple>,
                           public sender<OutputTuple>, no_copy {
    public:
        typedef OutputTuple output_type;

        typedef receiver<output_type> successor_type;
        typedef join_node_FE<JP, InputTuple, OutputTuple> input_ports_type;
        using input_ports_type::tuple_build_may_succeed;
        using input_ports_type::try_to_make_tuple;
        using input_ports_type::tuple_accepted;
        using input_ports_type::tuple_rejected;

    private:
        // ----------- Aggregator ------------
        enum op_type { reg_succ, rem_succ, try__get, do_fwrd };
        enum op_stat {WAIT=0, SUCCEEDED, FAILED};
        typedef join_node_base<JP,InputTuple,OutputTuple> my_class;

        class join_node_base_operation : public aggregated_operation<join_node_base_operation> {
        public:
            char type;
            union {
                output_type *my_arg;
                successor_type *my_succ;
            };
            join_node_base_operation(const output_type& e, op_type t) :
                type(char(t)), my_arg(const_cast<output_type*>(&e)) {}
            join_node_base_operation(const successor_type &s, op_type t) : type(char(t)), 
                my_succ(const_cast<successor_type *>(&s)) {}
            join_node_base_operation(op_type t) : type(char(t)) {}
        };

        typedef internal::aggregating_functor<my_class, join_node_base_operation> my_handler;
        friend class internal::aggregating_functor<my_class, join_node_base_operation>;
        bool forwarder_busy;
        aggregator<my_handler, join_node_base_operation> my_aggregator;

        void handle_operations(join_node_base_operation* op_list) {
            join_node_base_operation *current;
            while(op_list) {
                current = op_list;
                op_list = op_list->next;
                switch(current->type) {
                case reg_succ:
                    my_successors.register_successor(*(current->my_succ));
                    if(tuple_build_may_succeed() && !forwarder_busy) {
                        task::enqueue( * new ( task::allocate_additional_child_of(*(this->my_root_task)) )
                                forward_task<join_node_base<JP,InputTuple,OutputTuple> >(*this));
                        forwarder_busy = true;
                    }
                    __TBB_store_with_release(current->status, SUCCEEDED);
                    break;
                case rem_succ:
                    my_successors.remove_successor(*(current->my_succ));
                    __TBB_store_with_release(current->status, SUCCEEDED);
                    break;
                case try__get:
                    if(tuple_build_may_succeed()) {
                        if(try_to_make_tuple(*(current->my_arg))) {
                            tuple_accepted();
                            __TBB_store_with_release(current->status, SUCCEEDED);
                        }
                        else __TBB_store_with_release(current->status, FAILED);
                    }
                    else __TBB_store_with_release(current->status, FAILED);
                    break;
                case do_fwrd: {
                        bool build_succeeded;
                        output_type out;
                        if(tuple_build_may_succeed()) {
                            do {
                                build_succeeded = try_to_make_tuple(out);
                                if(build_succeeded) {
                                    if(my_successors.try_put(out)) {
                                        tuple_accepted();
                                    }
                                    else {
                                        tuple_rejected();
                                        build_succeeded = false;
                                    }
                                }
                            } while(build_succeeded);
                        }
                        __TBB_store_with_release(current->status, SUCCEEDED);
                        forwarder_busy = false;
                    }
                    break;
                }
            }
        }
        // ---------- end aggregator -----------
    public:

        join_node_base(graph &g) : input_ports_type(g), forwarder_busy(false) {
            my_successors.set_owner(this);
            input_ports_type::set_my_node(this);
            my_aggregator.initialize_handler(my_handler(this));
        }

        bool register_successor(successor_type &r) {
            join_node_base_operation op_data(r, reg_succ);
            my_aggregator.execute(&op_data);
            return op_data.status == SUCCEEDED;
        }

        bool remove_successor( successor_type &r) {
            join_node_base_operation op_data(r, rem_succ);
            my_aggregator.execute(&op_data);
            return op_data.status == SUCCEEDED;
        }

        bool try_get( output_type &v) {
            join_node_base_operation op_data(v, try__get);
            my_aggregator.execute(&op_data);
            return op_data.status == SUCCEEDED;
        }

    private:
        broadcast_cache<output_type, null_rw_mutex> my_successors;

        friend class forward_task< join_node_base<JP, InputTuple, OutputTuple> >;

        void forward() {
            join_node_base_operation op_data(do_fwrd);
            my_aggregator.execute(&op_data);
        }
    };

    //! unfolded_join_node : passes input_ports_tuple_type to join_node_base.  We build the input port type
    //  using tuple_element.
    template<int N, typename OutputTuple, join_policy JP>
    class unfolded_join_node;

    template<typename OutputTuple>
    class unfolded_join_node<2,OutputTuple,reserving> : public internal::join_node_base<reserving,
        std::tuple<
                reserving_port<typename std::tuple_element<0,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<1,OutputTuple>::type> >,
        OutputTuple
                  >
                  {
    public:
        typedef typename std::tuple<
                reserving_port<typename std::tuple_element<0,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<1,OutputTuple>::type> > input_ports_tuple_type;
        typedef OutputTuple output_type;
    private:
        typedef join_node_base<reserving, input_ports_tuple_type, output_type > base_type;
    public:
        unfolded_join_node(graph &g) : base_type(g) {}
    };

    template<typename OutputTuple>
    class unfolded_join_node<3,OutputTuple,reserving> : public internal::join_node_base<reserving,
        std::tuple<
                reserving_port<typename std::tuple_element<0,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<1,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<2,OutputTuple>::type> >,
        OutputTuple
                    >
                    {
    public:
        typedef typename std::tuple<
                reserving_port<typename std::tuple_element<0,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<1,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<2,OutputTuple>::type> > input_ports_tuple_type;
        typedef OutputTuple output_type;
    private:
        typedef join_node_base<reserving, input_ports_tuple_type, output_type > base_type;
    public:
        unfolded_join_node(graph &g) : base_type(g) {}
    };

    template<typename OutputTuple>
    class unfolded_join_node<4,OutputTuple,reserving> : public internal::join_node_base<reserving,
        std::tuple<
                reserving_port<typename std::tuple_element<0,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<1,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<2,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<3,OutputTuple>::type> >,
        OutputTuple
                    > {
    public:
        typedef typename std::tuple<
                reserving_port<typename std::tuple_element<0,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<1,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<2,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<3,OutputTuple>::type> > input_ports_tuple_type;
        typedef OutputTuple output_type;
    private:
        typedef join_node_base<reserving, input_ports_tuple_type, output_type > base_type;
    public:
        unfolded_join_node(graph &g) : base_type(g) {}
    };

    template<typename OutputTuple>
    class unfolded_join_node<5,OutputTuple,reserving> : public internal::join_node_base<reserving,
        std::tuple<
                reserving_port<typename std::tuple_element<0,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<1,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<2,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<3,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<4,OutputTuple>::type> >,
        OutputTuple
                > {
    public:
        typedef typename std::tuple<
                reserving_port<typename std::tuple_element<0,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<1,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<2,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<3,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<4,OutputTuple>::type> > input_ports_tuple_type;
        typedef OutputTuple output_type;
    private:
        typedef join_node_base<reserving, input_ports_tuple_type, output_type > base_type;
    public:
        unfolded_join_node(graph &g) : base_type(g) {}
    };

    template<typename OutputTuple>
    class unfolded_join_node<6,OutputTuple,reserving> : public internal::join_node_base<reserving,
        std::tuple<
                reserving_port<typename std::tuple_element<0,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<1,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<2,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<3,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<4,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<5,OutputTuple>::type> >,
        OutputTuple
                    > {
    public:
        typedef typename std::tuple<
                reserving_port<typename std::tuple_element<0,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<1,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<2,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<3,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<4,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<5,OutputTuple>::type> > input_ports_tuple_type;
        typedef OutputTuple output_type;
    private:
        typedef join_node_base<reserving, input_ports_tuple_type, output_type > base_type;
    public:
        unfolded_join_node(graph &g) : base_type(g) {}
    };

    template<typename OutputTuple>
    class unfolded_join_node<7,OutputTuple,reserving> : public internal::join_node_base<reserving,
        std::tuple<
                reserving_port<typename std::tuple_element<0,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<1,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<2,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<3,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<4,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<5,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<6,OutputTuple>::type> >,
        OutputTuple
                > {
    public:
        typedef typename std::tuple<
                reserving_port<typename std::tuple_element<0,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<1,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<2,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<3,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<4,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<5,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<6,OutputTuple>::type> > input_ports_tuple_type;
        typedef OutputTuple output_type;
    private:
        typedef join_node_base<reserving, input_ports_tuple_type, output_type > base_type;
    public:
        unfolded_join_node(graph &g) : base_type(g) {}
    };

    template<typename OutputTuple>
    class unfolded_join_node<8,OutputTuple,reserving> : public internal::join_node_base<reserving,
        std::tuple<
                reserving_port<typename std::tuple_element<0,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<1,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<2,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<3,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<4,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<5,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<6,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<7,OutputTuple>::type> >,
        OutputTuple
                > {
    public:
        typedef typename std::tuple<
                reserving_port<typename std::tuple_element<0,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<1,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<2,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<3,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<4,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<5,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<6,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<7,OutputTuple>::type> > input_ports_tuple_type;
        typedef OutputTuple output_type;
    private:
        typedef join_node_base<reserving, input_ports_tuple_type, output_type > base_type;
    public:
        unfolded_join_node(graph &g) : base_type(g) {}
    };

    template<typename OutputTuple>
    class unfolded_join_node<9,OutputTuple,reserving> : public internal::join_node_base<reserving,
        std::tuple<
                reserving_port<typename std::tuple_element<0,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<1,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<2,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<3,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<4,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<5,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<6,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<7,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<8,OutputTuple>::type> >,
        OutputTuple
                > {
    public:
        typedef typename std::tuple<
                reserving_port<typename std::tuple_element<0,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<1,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<2,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<3,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<4,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<5,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<6,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<7,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<8,OutputTuple>::type> > input_ports_tuple_type;
        typedef OutputTuple output_type;
    private:
        typedef join_node_base<reserving, input_ports_tuple_type, output_type > base_type;
    public:
        unfolded_join_node(graph &g) : base_type(g) {}
    };

    template<typename OutputTuple>
    class unfolded_join_node<10,OutputTuple,reserving> : public internal::join_node_base<reserving,
        std::tuple<
                reserving_port<typename std::tuple_element<0,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<1,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<2,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<3,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<4,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<5,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<6,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<7,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<8,OutputTuple>::type>,
                reserving_port<typename std::tuple_element<9,OutputTuple>::type> >,
        OutputTuple
                > {
    public:
        typedef typename std::tuple<
                reserving_port<typename std::tuple_element<0,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<1,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<2,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<3,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<4,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<5,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<6,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<7,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<8,OutputTuple>::type>, 
                reserving_port<typename std::tuple_element<9,OutputTuple>::type> > input_ports_tuple_type;
        typedef OutputTuple output_type;
    private:
        typedef join_node_base<reserving, input_ports_tuple_type, output_type > base_type;
    public:
        unfolded_join_node(graph &g) : base_type(g) {}
    };

    template<typename OutputTuple>
    class unfolded_join_node<2,OutputTuple,queueing> : public internal::join_node_base<queueing,
        std::tuple<
                queueing_port<typename std::tuple_element<0,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<1,OutputTuple>::type> >,
        OutputTuple
                  >
                  {
    public:
        typedef typename std::tuple<
                queueing_port<typename std::tuple_element<0,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<1,OutputTuple>::type> > input_ports_tuple_type;
        typedef OutputTuple output_type;
    private:
        typedef join_node_base<queueing, input_ports_tuple_type, output_type > base_type;
    public:
        unfolded_join_node(graph &g) : base_type(g) {}
    };

    template<typename OutputTuple>
    class unfolded_join_node<3,OutputTuple,queueing> : public internal::join_node_base<queueing,
        std::tuple<
                queueing_port<typename std::tuple_element<0,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<1,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<2,OutputTuple>::type> >,
        OutputTuple
                    >
                    {
    public:
        typedef typename std::tuple<
                queueing_port<typename std::tuple_element<0,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<1,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<2,OutputTuple>::type> > input_ports_tuple_type;
        typedef OutputTuple output_type;
    private:
        typedef join_node_base<queueing, input_ports_tuple_type, output_type > base_type;
    public:
        unfolded_join_node(graph &g) : base_type(g) {}
    };

    template<typename OutputTuple>
    class unfolded_join_node<4,OutputTuple,queueing> : public internal::join_node_base<queueing,
        std::tuple<
                queueing_port<typename std::tuple_element<0,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<1,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<2,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<3,OutputTuple>::type> >,
        OutputTuple
                    > {
    public:
        typedef typename std::tuple<
                queueing_port<typename std::tuple_element<0,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<1,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<2,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<3,OutputTuple>::type> > input_ports_tuple_type;
        typedef OutputTuple output_type;
    private:
        typedef join_node_base<queueing, input_ports_tuple_type, output_type > base_type;
    public:
        unfolded_join_node(graph &g) : base_type(g) {}
    };

    template<typename OutputTuple>
    class unfolded_join_node<5,OutputTuple,queueing> : public internal::join_node_base<queueing,
        std::tuple<
                queueing_port<typename std::tuple_element<0,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<1,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<2,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<3,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<4,OutputTuple>::type> >,
        OutputTuple
                > {
    public:
        typedef typename std::tuple<
                queueing_port<typename std::tuple_element<0,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<1,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<2,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<3,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<4,OutputTuple>::type> > input_ports_tuple_type;
        typedef OutputTuple output_type;
    private:
        typedef join_node_base<queueing, input_ports_tuple_type, output_type > base_type;
    public:
        unfolded_join_node(graph &g) : base_type(g) {}
    };

    template<typename OutputTuple>
    class unfolded_join_node<6,OutputTuple,queueing> : public internal::join_node_base<queueing,
        std::tuple<
                queueing_port<typename std::tuple_element<0,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<1,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<2,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<3,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<4,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<5,OutputTuple>::type> >,
        OutputTuple
                    > {
    public:
        typedef typename std::tuple<
                queueing_port<typename std::tuple_element<0,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<1,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<2,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<3,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<4,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<5,OutputTuple>::type> > input_ports_tuple_type;
        typedef OutputTuple output_type;
    private:
        typedef join_node_base<queueing, input_ports_tuple_type, output_type > base_type;
    public:
        unfolded_join_node(graph &g) : base_type(g) {}
    };

    template<typename OutputTuple>
    class unfolded_join_node<7,OutputTuple,queueing> : public internal::join_node_base<queueing,
        std::tuple<
                queueing_port<typename std::tuple_element<0,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<1,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<2,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<3,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<4,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<5,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<6,OutputTuple>::type> >,
        OutputTuple
                > {
    public:
        typedef typename std::tuple<
                queueing_port<typename std::tuple_element<0,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<1,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<2,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<3,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<4,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<5,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<6,OutputTuple>::type> > input_ports_tuple_type;
        typedef OutputTuple output_type;
    private:
        typedef join_node_base<queueing, input_ports_tuple_type, output_type > base_type;
    public:
        unfolded_join_node(graph &g) : base_type(g) {}
    };

    template<typename OutputTuple>
    class unfolded_join_node<8,OutputTuple,queueing> : public internal::join_node_base<queueing,
        std::tuple<
                queueing_port<typename std::tuple_element<0,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<1,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<2,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<3,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<4,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<5,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<6,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<7,OutputTuple>::type> >,
        OutputTuple
                > {
    public:
        typedef typename std::tuple<
                queueing_port<typename std::tuple_element<0,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<1,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<2,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<3,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<4,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<5,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<6,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<7,OutputTuple>::type> > input_ports_tuple_type;
        typedef OutputTuple output_type;
    private:
        typedef join_node_base<queueing, input_ports_tuple_type, output_type > base_type;
    public:
        unfolded_join_node(graph &g) : base_type(g) {}
    };

    template<typename OutputTuple>
    class unfolded_join_node<9,OutputTuple,queueing> : public internal::join_node_base<queueing,
        std::tuple<
                queueing_port<typename std::tuple_element<0,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<1,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<2,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<3,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<4,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<5,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<6,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<7,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<8,OutputTuple>::type> >,
        OutputTuple
                > {
    public:
        typedef typename std::tuple<
                queueing_port<typename std::tuple_element<0,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<1,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<2,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<3,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<4,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<5,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<6,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<7,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<8,OutputTuple>::type> > input_ports_tuple_type;
        typedef OutputTuple output_type;
    private:
        typedef join_node_base<queueing, input_ports_tuple_type, output_type > base_type;
    public:
        unfolded_join_node(graph &g) : base_type(g) {}
    };

    template<typename OutputTuple>
    class unfolded_join_node<10,OutputTuple,queueing> : public internal::join_node_base<queueing,
        std::tuple<
                queueing_port<typename std::tuple_element<0,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<1,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<2,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<3,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<4,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<5,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<6,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<7,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<8,OutputTuple>::type>,
                queueing_port<typename std::tuple_element<9,OutputTuple>::type> >,
        OutputTuple
                > {
    public:
        typedef typename std::tuple<
                queueing_port<typename std::tuple_element<0,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<1,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<2,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<3,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<4,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<5,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<6,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<7,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<8,OutputTuple>::type>, 
                queueing_port<typename std::tuple_element<9,OutputTuple>::type> > input_ports_tuple_type;
        typedef OutputTuple output_type;
    private:
        typedef join_node_base<queueing, input_ports_tuple_type, output_type > base_type;
    public:
        unfolded_join_node(graph &g) : base_type(g) {}
    };

    //! templated function to refer to input ports of the join node
    template<size_t N, typename JNT>
    typename std::tuple_element<N, typename JNT::input_ports_tuple_type>::type &input_port(JNT &jn) {
        return std::get<N>(jn.inputs());
    }

    } // namespace internal

using namespace internal::join_policy_namespace;
using internal::input_port;

template<typename OutputTuple, join_policy JP=reserving>
class join_node: public internal::unfolded_join_node<std::tuple_size<OutputTuple>::value, OutputTuple, JP> {
private:
    static const int N = std::tuple_size<OutputTuple>::value;
    typedef typename internal::unfolded_join_node<N, OutputTuple, JP> unfolded_type;
public:
    typedef OutputTuple output_type;
    typedef typename unfolded_type::input_ports_tuple_type input_ports_tuple_type;
    join_node(graph &g) : unfolded_type(g) { }
};

    //
    // Making edges
    //
  
    //! Makes an edge between a single predecessor and a single successor
    template< typename T >
    inline void make_edge( sender<T> &p, receiver<T> &s ) {
        p.register_successor( s );
    }

    //! Makes edges between a single predecessor and multiple successors
    template< typename T, typename SIterator >
    inline void make_edges( sender<T> &p, SIterator s_begin, SIterator s_end ) {
        for ( SIterator i = s_begin; i != s_end; ++i ) {
            make_edge( p, **i );
        }
    }

    //! Makes edges between a set of predecessors and a single successor
    template< typename T, typename PIterator >
    inline void make_edges( PIterator p_begin, PIterator p_end, receiver<T> &s ) {
        for ( PIterator i = p_begin; i != p_end; ++i ) {
            make_edge( **i, s );
        }
    }

}

#endif

