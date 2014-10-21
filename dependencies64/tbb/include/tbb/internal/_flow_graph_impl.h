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

#ifndef __TBB__flow_graph_impl_H
#define __TBB__flow_graph_impl_H

#ifndef __TBB_flow_graph_H
#error Do not #include this internal file directly; use public TBB headers instead.
#endif

namespace internal {

    namespace graph_policy_namespace {
        enum graph_buffer_policy { rejecting, reserving, queueing, tag_matching };
    }

    //! A functor that takes no input and generates a value of type Output
    template< typename Output >
    class source_body : tbb::internal::no_assign {
    public:
        virtual ~source_body() {}
        virtual bool operator()(Output &output) = 0;
        virtual source_body* clone() = 0;
    };
    
    //! The leaf for source_body
    template< typename Output, typename Body>
    class source_body_leaf : public source_body<Output> {
    public:
        source_body_leaf( const Body &_body ) : body(_body), init_body(_body) { }
        /*override*/ bool operator()(Output &output) { return body( output ); }
        /*override*/ source_body_leaf* clone() { 
            return new source_body_leaf< Output, Body >(init_body); 
        }
    private:
        Body body;
        Body init_body;
    };
    
    //! A functor that takes an Input and generates an Output
    template< typename Input, typename Output >
    class function_body : tbb::internal::no_assign {
    public:
        virtual ~function_body() {}
        virtual Output operator()(const Input &input) = 0;
        virtual function_body* clone() = 0;
    };
    
    //! the leaf for function_body
    template <typename Input, typename Output, typename B>
    class function_body_leaf : public function_body< Input, Output > {
    public:
        function_body_leaf( const B &_body ) : body(_body), init_body(_body) { }
        Output operator()(const Input &i) { return body(i); }
        B get_body() { return body; }
        /*override*/ function_body_leaf* clone() {
            return new function_body_leaf< Input, Output, B >(init_body);
        }
    private:
        B body;
        B init_body;
    };
    
    //! the leaf for function_body specialized for Input and output of continue_msg
    template <typename B>
    class function_body_leaf< continue_msg, continue_msg, B> : public function_body< continue_msg, continue_msg > {
    public:
        function_body_leaf( const B &_body ) : body(_body), init_body(_body) { }
        continue_msg operator()( const continue_msg &i ) { 
            body(i); 
            return i; 
        }
        B get_body() { return body; }
        /*override*/ function_body_leaf* clone() {
           return new function_body_leaf< continue_msg, continue_msg, B >(init_body);
        }    
    private:
        B body;
        B init_body;
    };
    
    //! the leaf for function_body specialized for Output of continue_msg
    template <typename Input, typename B>
    class function_body_leaf< Input, continue_msg, B> : public function_body< Input, continue_msg > {
    public:
        function_body_leaf( const B &_body ) : body(_body), init_body(_body) { }
        continue_msg operator()(const Input &i) { 
            body(i); 
            return continue_msg();
        }
        B get_body() { return body; }
        /*override*/ function_body_leaf* clone() {
            return new function_body_leaf< Input, continue_msg, B >(init_body);
        }    
    private:
        B body;
        B init_body;
    };
    
    //! the leaf for function_body specialized for Input of continue_msg
    template <typename Output, typename B>
    class function_body_leaf< continue_msg, Output, B > : public function_body< continue_msg, Output > {
    public:
        function_body_leaf( const B &_body ) : body(_body), init_body(_body) { }
        Output operator()(const continue_msg &i) { 
            return body(i); 
        }
        B get_body() { return body; }
        /*override*/ function_body_leaf* clone() {
            return new function_body_leaf< continue_msg, Output, B >(init_body);
        }    
    private:
        B body;
        B init_body;
    };

# if TBB_PREVIEW_GRAPH_NODES
    //! function_body that takes an Input and a set of output ports
    template<typename Input, typename OutputSet>
    class multioutput_function_body {
    public:
        virtual ~multioutput_function_body () {}
        virtual void operator()(const Input &/* input*/, OutputSet &/*oset*/) = 0;
        virtual multioutput_function_body* clone() = 0;
    };

    //! leaf for multi-output function.  OutputSet can be a std::tuple or a vector.
    template<typename Input, typename OutputSet, typename B>
    class multioutput_function_body_leaf : public multioutput_function_body<Input, OutputSet> {
    public:
        multioutput_function_body_leaf(const B &_body) : body(_body), init_body(_body) { }
        void operator()(const Input &input, OutputSet &oset) {
            body(input, oset); // body should explicitly put() to one or more of oset.
        }
        B get_body() { return body; }
        /*override*/ multioutput_function_body_leaf* clone() {
            return new multioutput_function_body_leaf<Input, OutputSet,B>(init_body);
        }
    private:
        B body;
        B init_body;
    };
#endif  // TBB_PREVIEW_GRAPH_NODES
    
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
        
        apply_body_task( NodeType &n, const Input &i ) : my_node(n), my_input(i) {}
        
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
    class successor_cache : tbb::internal::no_copy {
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
        
        virtual bool try_put( const T &t ) = 0; 
     };
    
    //! An abstract cache of succesors, specialized to continue_msg
    template<>
    class successor_cache< continue_msg > : tbb::internal::no_copy {
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
            if ( my_owner ) {
                continue_receiver *cr = dynamic_cast< continue_receiver * >(&r);
                if ( cr ) 
                    cr->register_predecessor( *my_owner );
            }
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
    
        virtual bool try_put( const continue_msg &t ) = 0; 
        
     };
    
    //! A cache of successors that are broadcast to
    template<typename T, typename M=spin_rw_mutex>
    class broadcast_cache : public successor_cache<T, M> {
        typedef M my_mutex_type;
        typedef std::list< receiver<T> * > my_successors_type;
        
    public:
        
        broadcast_cache( ) {}
        
        bool try_put( const T &t ) {
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
        
        bool try_put( const T &t ) {
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
    class decrementer : public continue_receiver, tbb::internal::no_copy {
        
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

#endif // __TBB__flow_graph_impl_H

