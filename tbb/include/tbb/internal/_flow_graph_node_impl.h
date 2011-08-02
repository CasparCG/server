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

#ifndef __TBB__graph_node_internal_H
#define __TBB__graph_node_internal_H

#include "_flow_graph_item_buffer_impl.h"

//! @cond INTERNAL
namespace internal {

    using tbb::internal::aggregated_operation;
    using tbb::internal::aggregating_functor;
    using tbb::internal::aggregator;

     template< typename T, typename A >
     class function_input_queue : public item_buffer<T,A> {
     public:
         bool pop( T& t ) {
             return this->pop_front( t );
         }

         bool push( T& t ) {
             return this->push_back( t );
         }
     };

    //! Implements methods for a function node that takes a type T as input
    template< typename Input, typename Output, typename A >
    class function_input : public receiver<Input>, tbb::internal::no_assign {
        typedef sender<Input> predecessor_type;
        enum op_stat {WAIT=0, SUCCEEDED, FAILED};
        enum op_type {reg_pred, rem_pred, app_body, tryput, try_fwd};
        typedef function_input<Input, Output, A> my_class;
        
    public:

        //! The input type of this receiver
        typedef Input input_type;
        //! The output type of this receiver
        typedef Output output_type;
        
        //! Constructor for function_input
        template< typename Body >
        function_input( graph &g, size_t max_concurrency, Body& body, function_input_queue<input_type,A> *q = NULL )
            : my_root_task(g.root_task()), my_max_concurrency(max_concurrency), my_concurrency(0),
              my_body( new internal::function_body_leaf< input_type, output_type, Body>(body) ),
              my_queue(q), forwarder_busy(false) {
            my_predecessors.set_owner(this);
            my_aggregator.initialize_handler(my_handler(this));
        }
        
        //! Copy constructor
        function_input( const function_input& src, function_input_queue<input_type,A> *q = NULL ) : 
#if (__TBB_GCC_VERSION < 40202 )
            receiver<Input>(), tbb::internal::no_assign(),
#endif
            my_root_task( src.my_root_task), my_max_concurrency(src.my_max_concurrency),
            my_concurrency(0), my_body( src.my_body->clone() ), my_queue(q), forwarder_busy(false)
        {
            my_predecessors.set_owner(this);
            my_aggregator.initialize_handler(my_handler(this));
        }

        //! Destructor
        virtual ~function_input() { 
            delete my_body; 
            if ( my_queue ) delete my_queue;
        }
        
        //! Put to the node
        virtual bool try_put( const input_type &t ) {
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

        template< typename Body >
        Body copy_function_object() {
            internal::function_body<input_type, output_type> &body_ref = *this->my_body;
            return dynamic_cast< internal::function_body_leaf<input_type, output_type, Body> & >(body_ref).get_body(); 
        } 
        
    protected:

        task *my_root_task;
        const size_t my_max_concurrency;
        size_t my_concurrency;
        function_body<input_type, output_type> *my_body;
        function_input_queue<input_type, A> *my_queue;
        predecessor_cache<input_type, null_mutex > my_predecessors;
        
        virtual broadcast_cache<output_type > &successors() = 0;

    private:

        friend class apply_body_task< my_class, input_type >;
        friend class forward_task< my_class >;
        
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
                        bool item_was_retrieved = false;
                        if ( my_queue )
                            item_was_retrieved = my_queue->pop(i);
                        else
                            item_was_retrieved = my_predecessors.get_item(i);
                        if (item_was_retrieved) {
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
           } else if ( my_queue && my_queue->push(*(op->elem)) ) { 
               __TBB_store_with_release(op->status, SUCCEEDED);
           } else {
               __TBB_store_with_release(op->status, FAILED);
           }
        }
        
        //! Tries to spawn bodies if available and if concurrency allows
        void internal_forward(my_operation *op) {
            if (my_concurrency<my_max_concurrency || !my_max_concurrency) {
                input_type i;
                bool item_was_retrieved = false;
                if ( my_queue )
                    item_was_retrieved = my_queue->pop(i);
                else
                    item_was_retrieved = my_predecessors.get_item(i);
                if (item_was_retrieved) {
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
       inline void spawn_body_task( const input_type &input ) {
           task::enqueue(*new(task::allocate_additional_child_of(*my_root_task)) apply_body_task< my_class, input_type >(*this, input));
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
           task::enqueue(*new(task::allocate_additional_child_of(*my_root_task)) forward_task< my_class >(*this));
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

        continue_input( const continue_input& src ) : continue_receiver(src), 
            my_root_task(src.my_root_task), my_body( src.my_body->clone() ) {}

        template< typename Body >
        Body copy_function_object() {
            internal::function_body<input_type, output_type> &body_ref = *my_body;
            return dynamic_cast< internal::function_body_leaf<input_type, output_type, Body> & >(body_ref).get_body(); 
        } 

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

#endif

