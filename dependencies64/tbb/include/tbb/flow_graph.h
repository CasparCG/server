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

#ifndef __TBB_flow_graph_H
#define __TBB_flow_graph_H

#include "tbb_stddef.h"
#include "atomic.h"
#include "spin_mutex.h"
#include "null_mutex.h"
#include "spin_rw_mutex.h"
#include "null_rw_mutex.h"
#include "task.h"
#include "concurrent_vector.h"
#include "internal/_aggregator_impl.h"

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
namespace flow {

//! An enumeration the provides the two most common concurrency levels: unlimited and serial
enum concurrency { unlimited = 0, serial = 1 };

namespace interface6 {

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
    virtual bool try_put( const T& t ) = 0;
        
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
        my_predecessor_count = my_initial_predecessor_count = number_of_predecessors;
        my_current_count = 0;
    }
        
    //! Copy constructor
    continue_receiver( const continue_receiver& src ) : receiver<continue_msg>() { 
        my_predecessor_count = my_initial_predecessor_count = src.my_initial_predecessor_count;
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
    /* override */ bool try_put( const input_type & ) {
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
    int my_initial_predecessor_count;
        
    //! Does whatever should happen when the threshold is reached
    /** This should be very fast or else spawn a task.  This is
        called while the sender is blocked in the try_put(). */
    virtual void execute() = 0;
        
};

#include "internal/_flow_graph_impl.h"
using namespace internal::graph_policy_namespace;

//! The graph class
/** This class serves as a handle to the graph */
class graph : tbb::internal::no_copy {
        
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

#include "internal/_flow_graph_node_impl.h"

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
        : my_root_task(g.root_task()), my_active(is_active), init_my_active(is_active),
        my_body( new internal::source_body_leaf< output_type, Body>(body) ),
        my_reserved(false), my_has_cached_item(false) 
    { 
        my_successors.set_owner(this);
    }
        
    //! Copy constructor
    source_node( const source_node& src ) :
        graph_node(), sender<Output>(),
        my_root_task( src.my_root_task), my_active(src.init_my_active),
        init_my_active(src.init_my_active), my_body( src.my_body->clone() ),
        my_reserved(false), my_has_cached_item(false)
    {
        my_successors.set_owner(this);
    }

    //! The destructor
    ~source_node() { delete my_body; }
        
    //! Add a new successor to this node
    /* override */ bool register_successor( receiver<output_type> &r ) {
        spin_mutex::scoped_lock lock(my_mutex);
        my_successors.register_successor(r);
        if ( my_active )
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
        my_active = true;
        if ( !my_successors.empty() )
            spawn_put();
    }
        
private:
        
    task *my_root_task;
    spin_mutex my_mutex;
    bool my_active;
    bool init_my_active;
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
template < typename Input, typename Output = continue_msg, graph_buffer_policy = queueing, typename Allocator=cache_aligned_allocator<Input> >
class function_node : public graph_node, public internal::function_input<Input,Output,Allocator>, public internal::function_output<Output> {
public:
        
    typedef Input input_type;
    typedef Output output_type;
    typedef sender< input_type > predecessor_type;
    typedef receiver< output_type > successor_type;
    typedef internal::function_output<output_type> fOutput_type;
        
    //! Constructor
    template< typename Body >
    function_node( graph &g, size_t concurrency, Body body )
    : internal::function_input<input_type,output_type,Allocator>( g, concurrency, body ) {
    }

    //! Copy constructor
    function_node( const function_node& src ) : 
        graph_node(), internal::function_input<input_type,output_type,Allocator>( src ),
        fOutput_type() {}
        
protected:

    /* override */ internal::broadcast_cache<output_type> &successors () { return fOutput_type::my_successors; }
        
};

//! Implements a function node that supports Input -> Output
template < typename Input, typename Output, typename Allocator >
class function_node<Input,Output,queueing,Allocator> : public graph_node, public internal::function_input<Input,Output,Allocator>, public internal::function_output<Output> {
public:
        
    typedef Input input_type;
    typedef Output output_type;
    typedef sender< input_type > predecessor_type;
    typedef receiver< output_type > successor_type;
    typedef internal::function_input<input_type,output_type,Allocator> fInput_type;
    typedef internal::function_input_queue<input_type, Allocator> queue_type;
    typedef internal::function_output<output_type> fOutput_type;
        
    //! Constructor
    template< typename Body >
    function_node( graph &g, size_t concurrency, Body body ) : fInput_type( g, concurrency, body, new queue_type() ) {
    }

    //! Copy constructor
    function_node( const function_node& src ) : 
        graph_node(), fInput_type( src, new queue_type() ) , fOutput_type() { }

protected:

    /* override */ internal::broadcast_cache<output_type> &successors () { return fOutput_type::my_successors; }
        
};

#include "tbb/internal/_flow_graph_types_impl.h"

#if TBB_PREVIEW_GRAPH_NODES
//! implements a function node that supports Input -> (set of outputs)
// Output is a tuple of output types.
template < typename Input, typename Output, graph_buffer_policy = queueing, typename Allocator=cache_aligned_allocator<Input> >
class multioutput_function_node : 
    public graph_node, 
    public internal::multioutput_function_input
    <  
        Input, 
        typename internal::wrap_tuple_elements<
            std::tuple_size<Output>::value,  // #elements in tuple
            internal::function_output,  // wrap this around each element
            Output // the tuple providing the types
        >::type,
        Allocator
    > {
private:
    static const int N = std::tuple_size<Output>::value;
public:
    typedef Input input_type;
    typedef typename internal::wrap_tuple_elements<N,internal::function_output, Output>::type ports_type;
private:
    typedef typename internal::multioutput_function_input<input_type, ports_type, Allocator> base_type;
    typedef typename internal::function_input_queue<input_type,Allocator> queue_type;
public:
    template<typename Body>
    multioutput_function_node( graph &g, size_t concurrency, Body body ) : base_type(g,concurrency, body) {}
    multioutput_function_node( const multioutput_function_node &other) :
        graph_node(), base_type(other) {}
    // all the guts are in multioutput_function_input...

};  // multioutput_function_node
        
template < typename Input, typename Output, typename Allocator >
class multioutput_function_node<Input,Output,queueing,Allocator> : public graph_node, public internal::multioutput_function_input<Input, 
    typename internal::wrap_tuple_elements<std::tuple_size<Output>::value, internal::function_output, Output>::type, Allocator> {
    static const int N = std::tuple_size<Output>::value;
public:
    typedef Input input_type;
    typedef typename internal::wrap_tuple_elements<N, internal::function_output, Output>::type ports_type;
private:
    typedef typename internal::multioutput_function_input<input_type, ports_type, Allocator> base_type;
    typedef typename internal::function_input_queue<input_type,Allocator> queue_type;
public:

    template<typename Body>
    multioutput_function_node( graph &g, size_t concurrency, Body body) : base_type(g,concurrency, body, new queue_type()) {}
    multioutput_function_node( const multioutput_function_node &other) :
        graph_node(), base_type(other, new queue_type()) {}

};  // multioutput_function_node

//! split_node: accepts a tuple as input, forwards each element of the tuple to its
//  successors.  The node has unlimited concurrency, so though it is marked as
//  "rejecting" it does not reject inputs.
template<typename TupleType, typename Allocator=cache_aligned_allocator<TupleType> >
class split_node : public multioutput_function_node<TupleType, TupleType, rejecting, Allocator> {
    static const int N = std::tuple_size<TupleType>::value;
    typedef multioutput_function_node<TupleType,TupleType,rejecting,Allocator> base_type;
public:
    typedef typename base_type::ports_type ports_type;
private:

    struct splitting_body {
        void operator()(const TupleType& t, ports_type &p) {
            internal::emit_element<N>::emit_this(t, p);
        }
    };
public:
    typedef TupleType input_type;
    typedef Allocator allocator_type;
    split_node(graph &g) : base_type(g, unlimited, splitting_body()) { }
    split_node( const split_node & other) : base_type(other) { }
};
#endif  // TBB_PREVIEW_GRAPH_NODES

//! Implements an executable node that supports continue_msg -> Output
template <typename Output>
class continue_node : public graph_node, public internal::continue_input<Output>, public internal::function_output<Output> {
public:
        
    typedef continue_msg input_type;
    typedef Output output_type;
    typedef sender< input_type > predecessor_type;
    typedef receiver< output_type > successor_type;
    typedef internal::function_output<output_type> fOutput_type;
        
     //! Constructor for executable node with continue_msg -> Output
     template <typename Body >
     continue_node( graph &g, Body body )
             : internal::continue_input<output_type>( g, body ) {
     }
        
    //! Constructor for executable node with continue_msg -> Output
    template <typename Body >
    continue_node( graph &g, int number_of_predecessors, Body body )
        : internal::continue_input<output_type>( g, number_of_predecessors, body )
    {
    }
 
    //! Copy constructor       
    continue_node( const continue_node& src ) :
        graph_node(), internal::continue_input<output_type>(src),
        internal::function_output<Output>() { }

protected:
        
    /* override */ internal::broadcast_cache<output_type> &successors () { return fOutput_type::my_successors; }
        
};
        
template< typename T >
class overwrite_node : public graph_node, public receiver<T>, public sender<T> {
public:
        
    typedef T input_type;
    typedef T output_type;
    typedef sender< input_type > predecessor_type;
    typedef receiver< output_type > successor_type;
        
    overwrite_node() : my_buffer_is_valid(false) {
        my_successors.set_owner( this );
    }

    // Copy constructor; doesn't take anything from src; default won't work
    overwrite_node( const overwrite_node& ) : 
        graph_node(), receiver<T>(), sender<T>(), my_buffer_is_valid(false) {
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
        
    /* override */ bool try_put( const T &v ) {
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
        
    //! Constructor
    write_once_node() : overwrite_node<T>() {}

    //! Copy constructor: call base class copy constructor
    write_once_node( const write_once_node& src ) : overwrite_node<T>(src) {}

    /* override */ bool try_put( const T &v ) {
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
        
//! Forwards messages of type T to all successors
template <typename T>
class broadcast_node : public graph_node, public receiver<T>, public sender<T> {
        
    internal::broadcast_cache<T> my_successors;
        
public:
        
    typedef T input_type;
    typedef T output_type;
    typedef sender< input_type > predecessor_type;
    typedef receiver< output_type > successor_type;
        
    broadcast_node( ) {
        my_successors.set_owner( this );
    }
        
    // Copy constructor
    broadcast_node( const broadcast_node& ) : graph_node(), receiver<T>(), sender<T>() {
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
        
    /* override */ bool try_put( const T &t ) {
        my_successors.try_put(t);
        return true;
    }
        
};

#include "internal/_flow_graph_item_buffer_impl.h"

//! Forwards messages in arbitrary order
template <typename T, typename A=cache_aligned_allocator<T> >
class buffer_node : public graph_node, public reservable_item_buffer<T, A>, public receiver<T>, public sender<T> {
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

    //! Copy constructor
    buffer_node( const buffer_node& src ) :
        graph_node(), reservable_item_buffer<T>(), receiver<T>(), sender<T>(),
        my_parent( src.my_parent ) {
        forwarder_busy = false;
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
    /* override */ bool try_put(const T &t) {
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

    //! Copy constructor
    queue_node( const queue_node& src) : buffer_node<T, A>(src) {}
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

    //! Copy constructor
    sequencer_node( const sequencer_node& src ) : queue_node<T, A>(src),
        my_sequencer( src.my_sequencer->clone() ) {}
        
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

    //! Copy constructor
    priority_queue_node( const priority_queue_node &src ) : buffer_node<T, A>(src), mark(0) {}
        
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
class limiter_node : public graph_node, public receiver< T >, public sender< T > {
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
    int init_decrement_predecessors;

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
    limiter_node(graph &g, size_t threshold, int num_decrement_predecessors=0) : 
        my_root_task(g.root_task()), my_threshold(threshold), my_count(0), 
        init_decrement_predecessors(num_decrement_predecessors), 
        decrement(num_decrement_predecessors) 
    {
        my_predecessors.set_owner(this);
        my_successors.set_owner(this);
        decrement.set_owner(this);
    }
        
    //! Copy constructor
    limiter_node( const limiter_node& src ) : 
        graph_node(), receiver<T>(), sender<T>(),
        my_root_task(src.my_root_task), my_threshold(src.my_threshold), my_count(0), 
        init_decrement_predecessors(src.init_decrement_predecessors), 
        decrement(src.init_decrement_predecessors) 
    {
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
    /* override */ bool try_put( const T &t ) {
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

#include "internal/_flow_graph_join_impl.h"

using internal::reserving_port;
using internal::queueing_port;
using internal::tag_matching_port;
using internal::input_port;
using internal::tag_value;
using internal::NO_TAG;

template<typename OutputTuple, graph_buffer_policy JP=queueing> class join_node;

template<typename OutputTuple>
class join_node<OutputTuple,reserving>: public internal::unfolded_join_node<std::tuple_size<OutputTuple>::value, reserving_port, OutputTuple, reserving> {
private:
    static const int N = std::tuple_size<OutputTuple>::value;
    typedef typename internal::unfolded_join_node<N, reserving_port, OutputTuple, reserving> unfolded_type;
public:
    typedef OutputTuple output_type;
    typedef typename unfolded_type::input_ports_tuple_type input_ports_tuple_type;
    join_node(graph &g) : unfolded_type(g) { }
    join_node(const join_node &other) : unfolded_type(other) {}
};

template<typename OutputTuple>
class join_node<OutputTuple,queueing>: public internal::unfolded_join_node<std::tuple_size<OutputTuple>::value, queueing_port, OutputTuple, queueing> {
private:
    static const int N = std::tuple_size<OutputTuple>::value;
    typedef typename internal::unfolded_join_node<N, queueing_port, OutputTuple, queueing> unfolded_type;
public:
    typedef OutputTuple output_type;
    typedef typename unfolded_type::input_ports_tuple_type input_ports_tuple_type;
    join_node(graph &g) : unfolded_type(g) { }
    join_node(const join_node &other) : unfolded_type(other) {}
};

// template for tag_matching join_node
template<typename OutputTuple>
class join_node<OutputTuple, tag_matching> : public internal::unfolded_join_node<std::tuple_size<OutputTuple>::value,
      tag_matching_port, OutputTuple, tag_matching> {
private:
    static const int N = std::tuple_size<OutputTuple>::value;
    typedef typename internal::unfolded_join_node<N, tag_matching_port, OutputTuple, tag_matching> unfolded_type;
public:
    typedef OutputTuple output_type;
    typedef typename unfolded_type::input_ports_tuple_type input_ports_tuple_type;
    template<typename B0, typename B1>
    join_node(graph &g, B0 b0, B1 b1) : unfolded_type(g, b0, b1) { }
    template<typename B0, typename B1, typename B2>
    join_node(graph &g, B0 b0, B1 b1, B2 b2) : unfolded_type(g, b0, b1, b2) { }
    template<typename B0, typename B1, typename B2, typename B3>
    join_node(graph &g, B0 b0, B1 b1, B2 b2, B3 b3) : unfolded_type(g, b0, b1, b2, b3) { }
    template<typename B0, typename B1, typename B2, typename B3, typename B4>
    join_node(graph &g, B0 b0, B1 b1, B2 b2, B3 b3, B4 b4) : unfolded_type(g, b0, b1, b2, b3, b4) { }
    template<typename B0, typename B1, typename B2, typename B3, typename B4, typename B5>
    join_node(graph &g, B0 b0, B1 b1, B2 b2, B3 b3, B4 b4, B5 b5) : unfolded_type(g, b0, b1, b2, b3, b4, b5) { }
    template<typename B0, typename B1, typename B2, typename B3, typename B4, typename B5, typename B6>
    join_node(graph &g, B0 b0, B1 b1, B2 b2, B3 b3, B4 b4, B5 b5, B6 b6) : unfolded_type(g, b0, b1, b2, b3, b4, b5, b6) { }
    template<typename B0, typename B1, typename B2, typename B3, typename B4, typename B5, typename B6, typename B7>
    join_node(graph &g, B0 b0, B1 b1, B2 b2, B3 b3, B4 b4, B5 b5, B6 b6, B7 b7) : unfolded_type(g, b0, b1, b2, b3, b4, b5, b6, b7) { }
    template<typename B0, typename B1, typename B2, typename B3, typename B4, typename B5, typename B6, typename B7, typename B8>
    join_node(graph &g, B0 b0, B1 b1, B2 b2, B3 b3, B4 b4, B5 b5, B6 b6, B7 b7, B8 b8) : unfolded_type(g, b0, b1, b2, b3, b4, b5, b6, b7, b8) { }
    template<typename B0, typename B1, typename B2, typename B3, typename B4, typename B5, typename B6, typename B7, typename B8, typename B9>
    join_node(graph &g, B0 b0, B1 b1, B2 b2, B3 b3, B4 b4, B5 b5, B6 b6, B7 b7, B8 b8, B9 b9) : unfolded_type(g, b0, b1, b2, b3, b4, b5, b6, b7, b8, b9) { }
    join_node(const join_node &other) : unfolded_type(other) {}
};

#if TBB_PREVIEW_GRAPH_NODES
// or node
#include "internal/_flow_graph_or_impl.h"

template<typename InputTuple>
class or_node : public internal::unfolded_or_node<InputTuple> {
private:
    static const int N = std::tuple_size<InputTuple>::value;
public:
    typedef typename internal::or_output_type<InputTuple>::type output_type;
    typedef typename internal::unfolded_or_node<InputTuple> unfolded_type;
    or_node() : unfolded_type() { }
    // Copy constructor
    or_node( const or_node& /*other*/ ) : unfolded_type() { }
};
#endif  // TBB_PREVIEW_GRAPH_NODES

//! Makes an edge between a single predecessor and a single successor
template< typename T >
inline void make_edge( sender<T> &p, receiver<T> &s ) {
    p.register_successor( s );
}
        
//! Makes an edge between a single predecessor and a single successor
template< typename T >
inline void remove_edge( sender<T> &p, receiver<T> &s ) {
    p.remove_successor( s );
}

//! Returns a copy of the body from a function or continue node
template< typename Body, typename Node >
Body copy_body( Node &n ) {
    return n.template copy_function_object<Body>();
}
        
        
} // interface6

    using interface6::graph;
    using interface6::graph_node;
    using interface6::continue_msg;
    using interface6::sender;
    using interface6::receiver;
    using interface6::continue_receiver;

    using interface6::source_node;
    using interface6::function_node;
#if TBB_PREVIEW_GRAPH_NODES
    using interface6::multioutput_function_node;
    using interface6::split_node;
    using interface6::internal::output_port;
    using interface6::or_node;
#endif
    using interface6::continue_node;
    using interface6::overwrite_node;
    using interface6::write_once_node;
    using interface6::broadcast_node;
    using interface6::buffer_node;
    using interface6::queue_node;
    using interface6::sequencer_node;
    using interface6::priority_queue_node;
    using interface6::limiter_node;
    using namespace interface6::internal::graph_policy_namespace;
    using interface6::join_node;
    using interface6::input_port;
    using interface6::copy_body; 
    using interface6::make_edge; 
    using interface6::remove_edge; 
    using interface6::internal::NO_TAG;
    using interface6::internal::tag_value;

} // flow
} // tbb

#endif // __TBB_flow_graph_H
