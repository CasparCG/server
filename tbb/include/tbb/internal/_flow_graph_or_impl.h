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

#ifndef __TBB__flow_graph_or_impl_H
#define __TBB__flow_graph_or_impl_H

namespace internal {

    // Output of the or_node is a struct containing a union, and will be of
    // the form
    //
    //  struct {
    //     size_t indx;
    //     union {
    //         T0 result0;
    //         T1 result1;
    //         ...
    //         Tn resultn;
    //     };
    //  };
    //
    //  where the value of indx will indicate which result was put to the
    //  successor.  indx == 0 => result0 and so on.
    //
    //  The order of the items in the union is determined by the tuple that
    //  defines the input port types (the same way a join_node's inputs are
    //  defined.)  So the union ordering corresponds to the ordering of the 
    //  input ports of the node.
    //
    //  the types of each element of the union are represented by tuple_types,
    //  a typedef in the or_node.  So the 2nd type in the union that is the
    //  output type for an or_node OrType is
    //
    //      std::tuple_element<1,OrType::tuple_types>::type

    template<int N, typename OutputTuple>
    struct or_output_type;

    template<typename OutputTuple>
    struct or_output_type<2, OutputTuple> {
        typedef OutputTuple tuple_types;
        typedef struct {
            size_t indx;
            union {
                typename std::tuple_element<0,OutputTuple>::type result0;
                typename std::tuple_element<1,OutputTuple>::type result1;
            };
        } type;
    };

    template<typename OutputTuple>
    struct or_output_type<3, OutputTuple> {
        typedef struct {
            typedef OutputTuple tuple_types;
            size_t indx;
            union {
                typename std::tuple_element<0,OutputTuple>::type result0;
                typename std::tuple_element<1,OutputTuple>::type result1;
                typename std::tuple_element<2,OutputTuple>::type result2;
            };
        } type;
    };

    template<typename OutputTuple>
    struct or_output_type<4, OutputTuple> {
        typedef struct {
            typedef OutputTuple tuple_types;
            size_t indx;
            union {
                typename std::tuple_element<0,OutputTuple>::type result0;
                typename std::tuple_element<1,OutputTuple>::type result1;
                typename std::tuple_element<2,OutputTuple>::type result2;
                typename std::tuple_element<3,OutputTuple>::type result3;
            };
        } type;
    };

    template<typename OutputTuple>
    struct or_output_type<5, OutputTuple> {
        typedef struct {
            typedef OutputTuple tuple_types;
            size_t indx;
            union {
                typename std::tuple_element<0,OutputTuple>::type result0;
                typename std::tuple_element<1,OutputTuple>::type result1;
                typename std::tuple_element<2,OutputTuple>::type result2;
                typename std::tuple_element<3,OutputTuple>::type result3;
                typename std::tuple_element<4,OutputTuple>::type result4;
            };
        } type;
    };

    template<typename OutputTuple>
    struct or_output_type<6, OutputTuple> {
        typedef struct {
            typedef OutputTuple tuple_types;
            size_t indx;
            union {
                typename std::tuple_element<0,OutputTuple>::type result0;
                typename std::tuple_element<1,OutputTuple>::type result1;
                typename std::tuple_element<2,OutputTuple>::type result2;
                typename std::tuple_element<3,OutputTuple>::type result3;
                typename std::tuple_element<4,OutputTuple>::type result4;
                typename std::tuple_element<5,OutputTuple>::type result5;
            };
        } type;
    };

    template<typename OutputTuple>
    struct or_output_type<7, OutputTuple> {
        typedef struct {
            typedef OutputTuple tuple_types;
            size_t indx;
            union {
                typename std::tuple_element<0,OutputTuple>::type result0;
                typename std::tuple_element<1,OutputTuple>::type result1;
                typename std::tuple_element<2,OutputTuple>::type result2;
                typename std::tuple_element<3,OutputTuple>::type result3;
                typename std::tuple_element<4,OutputTuple>::type result4;
                typename std::tuple_element<5,OutputTuple>::type result5;
                typename std::tuple_element<6,OutputTuple>::type result6;
            };
        } type;
    };

    template<typename OutputTuple>
    struct or_output_type<8, OutputTuple> {
        typedef struct {
            typedef OutputTuple tuple_types;
            size_t indx;
            union {
                typename std::tuple_element<0,OutputTuple>::type result0;
                typename std::tuple_element<1,OutputTuple>::type result1;
                typename std::tuple_element<2,OutputTuple>::type result2;
                typename std::tuple_element<3,OutputTuple>::type result3;
                typename std::tuple_element<4,OutputTuple>::type result4;
                typename std::tuple_element<5,OutputTuple>::type result5;
                typename std::tuple_element<6,OutputTuple>::type result6;
                typename std::tuple_element<7,OutputTuple>::type result7;
            };
        } type;
    };

    template<typename OutputTuple>
    struct or_output_type<9, OutputTuple> {
        typedef struct {
            typedef OutputTuple tuple_types;
            size_t indx;
            union {
                typename std::tuple_element<0,OutputTuple>::type result0;
                typename std::tuple_element<1,OutputTuple>::type result1;
                typename std::tuple_element<2,OutputTuple>::type result2;
                typename std::tuple_element<3,OutputTuple>::type result3;
                typename std::tuple_element<4,OutputTuple>::type result4;
                typename std::tuple_element<5,OutputTuple>::type result5;
                typename std::tuple_element<6,OutputTuple>::type result6;
                typename std::tuple_element<7,OutputTuple>::type result7;
                typename std::tuple_element<8,OutputTuple>::type result8;
            };
        } type;
    };

    template<typename OutputTuple>
    struct or_output_type<10, OutputTuple> {
        typedef struct {
            typedef OutputTuple tuple_types;
            size_t indx;
            union {
                typename std::tuple_element<0,OutputTuple>::type result0;
                typename std::tuple_element<1,OutputTuple>::type result1;
                typename std::tuple_element<2,OutputTuple>::type result2;
                typename std::tuple_element<3,OutputTuple>::type result3;
                typename std::tuple_element<4,OutputTuple>::type result4;
                typename std::tuple_element<5,OutputTuple>::type result5;
                typename std::tuple_element<6,OutputTuple>::type result6;
                typename std::tuple_element<7,OutputTuple>::type result7;
                typename std::tuple_element<8,OutputTuple>::type result8;
                typename std::tuple_element<9,OutputTuple>::type result9;
            };
        } type;
    };

    template<typename TupleTypes, int N>
        struct or_item_helper;

    // or_item_helper takes 0-9 as its template parameter
    template<typename TupleTypes>
    struct or_item_helper<TupleTypes,0> {
        template<typename OutputType>
        static inline void create_output_value(OutputType &o, void *v) {
            o.indx = 0;
            o.result0 = *(reinterpret_cast<typename std::tuple_element<0,TupleTypes>::type *>(v));
        }
    };

    template<typename TupleTypes>
    struct or_item_helper<TupleTypes,1> {
        template<typename OutputType>
        static inline void create_output_value(OutputType &o, void *v) {
            o.indx = 1;
            o.result1 = *(reinterpret_cast<typename std::tuple_element<1,TupleTypes>::type *>(v));
        }
    };

    template<typename TupleTypes>
    struct or_item_helper<TupleTypes,2> {
        template<typename OutputType>
        static inline void create_output_value(OutputType &o, void *v) {
            o.indx = 2;
            o.result2 = *(reinterpret_cast<typename std::tuple_element<2,TupleTypes>::type *>(v));
        }
    };

    template<typename TupleTypes>
    struct or_item_helper<TupleTypes,3> {
        template<typename OutputType>
        static inline void create_output_value(OutputType &o, void *v) {
            o.indx = 3;
            o.result3 = *(reinterpret_cast<typename std::tuple_element<3,TupleTypes>::type *>(v));
        }
    };

    template<typename TupleTypes>
    struct or_item_helper<TupleTypes,4> {
        template<typename OutputType>
        static inline void create_output_value(OutputType &o, void *v) {
            o.indx = 4;
            o.result4 = *(reinterpret_cast<typename std::tuple_element<4,TupleTypes>::type *>(v));
        }
    };

    template<typename TupleTypes>
    struct or_item_helper<TupleTypes,5> {
        template<typename OutputType>
        static inline void create_output_value(OutputType &o, void *v) {
            o.indx = 5;
            o.result5 = *(reinterpret_cast<typename std::tuple_element<5,TupleTypes>::type *>(v));
        }
    };

    template<typename TupleTypes>
    struct or_item_helper<TupleTypes,6> {
        template<typename OutputType>
        static inline void create_output_value(OutputType &o, void *v) {
            o.indx = 6;
            o.result6 = *(reinterpret_cast<typename std::tuple_element<6,TupleTypes>::type *>(v));
        }
    };

    template<typename TupleTypes>
    struct or_item_helper<TupleTypes,7> {
        template<typename OutputType>
        static inline void create_output_value(OutputType &o, void *v) {
            o.indx = 7;
            o.result7 = *(reinterpret_cast<typename std::tuple_element<7,TupleTypes>::type *>(v));
        }
    };

    template<typename TupleTypes>
    struct or_item_helper<TupleTypes,8> {
        template<typename OutputType>
        static inline void create_output_value(OutputType &o, void *v) {
            o.indx = 8;
            o.result8 = *(reinterpret_cast<typename std::tuple_element<8,TupleTypes>::type *>(v));
        }
    };

    template<typename TupleTypes>
    struct or_item_helper<TupleTypes,9> {
        template<typename OutputType>
        static inline void create_output_value(OutputType &o, void *v) {
            o.indx = 9;
            o.result9 = *(reinterpret_cast<typename std::tuple_element<9,TupleTypes>::type *>(v));
        }
    };

    template<typename TupleTypes,int N>
    struct or_helper {
        template<typename OutputType>
        static inline void create_output(OutputType &o, size_t i, void* v) {
            if(i == N-1) {
                or_item_helper<TupleTypes,N-1>::create_output_value(o,v);
            }
            else
                or_helper<TupleTypes,N-1>::create_output(o,i,v);
        }
        template<typename PortTuple, typename PutBase>
        static inline void set_or_node_pointer(PortTuple &my_input, PutBase *p) {
            std::get<N-1>(my_input).set_up(p, N-1);
            or_helper<TupleTypes,N-1>::set_or_node_pointer(my_input, p);
        }
    };

    template<typename TupleTypes>
    struct or_helper<TupleTypes,1> {
        template<typename OutputType>
        static inline void create_output(OutputType &o, size_t i, void* v) {
            if(i == 0) {
                or_item_helper<TupleTypes,0>::create_output_value(o,v);
            }
        }
        template<typename PortTuple, typename PutBase>
        static inline void set_or_node_pointer(PortTuple &my_input, PutBase *p) {
            std::get<0>(my_input).set_up(p, 0);
        }
    };

    struct put_base {
        virtual bool try_put_with_index(size_t index, void *v) = 0;
        virtual ~put_base() { }
    };

    template<typename T>
    class or_input_port : public receiver<T> {
    private:
        size_t my_index;
        put_base *my_or_node;
    public:
        void set_up(put_base *p, size_t i) { my_index = i; my_or_node = p; }
        bool try_put(const T &v) {
            return my_or_node->try_put_with_index(my_index, reinterpret_cast<void *>(const_cast<T*>(&v)));
        }
    };

    template<size_t N, typename OutputTuple>
    struct or_input_type;

    template<typename OutputTuple>
    struct or_input_type<2,OutputTuple> {
        typedef typename std::tuple<
            or_input_port<typename std::tuple_element<0,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<1,OutputTuple>::type>
        > type;
    };

    template<typename OutputTuple>
    struct or_input_type<3,OutputTuple> {
        typedef typename std::tuple<
            or_input_port<typename std::tuple_element<0,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<1,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<2,OutputTuple>::type>
        > type;
    };

    template<typename OutputTuple>
    struct or_input_type<4,OutputTuple> {
        typedef typename std::tuple<
            or_input_port<typename std::tuple_element<0,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<1,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<2,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<3,OutputTuple>::type>
        > type;
    };

    template<typename OutputTuple>
    struct or_input_type<5,OutputTuple> {
        typedef typename std::tuple<
            or_input_port<typename std::tuple_element<0,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<1,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<2,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<3,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<4,OutputTuple>::type>
        > type;
    };

    template<typename OutputTuple>
    struct or_input_type<6,OutputTuple> {
        typedef typename std::tuple<
            or_input_port<typename std::tuple_element<0,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<1,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<2,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<3,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<4,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<5,OutputTuple>::type>
        > type;
    };

    template<typename OutputTuple>
    struct or_input_type<7,OutputTuple> {
        typedef typename std::tuple<
            or_input_port<typename std::tuple_element<0,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<1,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<2,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<3,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<4,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<5,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<6,OutputTuple>::type>
        > type;
    };

    template<typename OutputTuple>
    struct or_input_type<8,OutputTuple> {
        typedef typename std::tuple<
            or_input_port<typename std::tuple_element<0,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<1,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<2,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<3,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<4,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<5,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<6,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<7,OutputTuple>::type>
        > type;
    };

    template<typename OutputTuple>
    struct or_input_type<9,OutputTuple> {
        typedef typename std::tuple<
            or_input_port<typename std::tuple_element<0,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<1,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<2,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<3,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<4,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<5,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<6,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<7,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<8,OutputTuple>::type>
        > type;
    };

    template<typename OutputTuple>
    struct or_input_type<10,OutputTuple> {
        typedef typename std::tuple<
            or_input_port<typename std::tuple_element<0,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<1,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<2,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<3,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<4,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<5,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<6,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<7,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<8,OutputTuple>::type>,
            or_input_port<typename std::tuple_element<9,OutputTuple>::type>
        > type;
    };

    template<typename InputTuple, typename OutputType, typename StructTypes>
    class or_node_FE : public put_base {
    public:
        static const int N = std::tuple_size<InputTuple>::value;
        typedef OutputType output_type;
        typedef InputTuple input_type;

        or_node_FE( ) {
            or_helper<StructTypes,N>::set_or_node_pointer(my_inputs, this);
        }

        input_type &inputs() { return my_inputs; }
    protected:
        input_type my_inputs;
    };

    //! or_node_base
    template<typename InputTuple, typename OutputType, typename StructTypes>
    class or_node_base : public graph_node, public or_node_FE<InputTuple, OutputType,StructTypes>,
                           public sender<OutputType> {
    public:
        static const size_t N = std::tuple_size<InputTuple>::value;
        typedef OutputType output_type;
        typedef StructTypes tuple_types;
        typedef receiver<output_type> successor_type;
        typedef or_node_FE<InputTuple, output_type,StructTypes> input_ports_type;

    private:
        // ----------- Aggregator ------------
        enum op_type { reg_succ, rem_succ, try__put };
        enum op_stat {WAIT=0, SUCCEEDED, FAILED};
        typedef or_node_base<InputTuple,output_type,StructTypes> my_class;

        class or_node_base_operation : public aggregated_operation<or_node_base_operation> {
        public:
            char type;
            size_t indx;
            union {
                void *my_arg;
                successor_type *my_succ;
            };
            or_node_base_operation(size_t i, const void* e, op_type t) :
                type(char(t)), indx(i), my_arg(const_cast<void *>(e)) {}
            or_node_base_operation(const successor_type &s, op_type t) : type(char(t)), 
                my_succ(const_cast<successor_type *>(&s)) {}
            or_node_base_operation(op_type t) : type(char(t)) {}
        };

        typedef internal::aggregating_functor<my_class, or_node_base_operation> my_handler;
        friend class internal::aggregating_functor<my_class, or_node_base_operation>;
        aggregator<my_handler, or_node_base_operation> my_aggregator;

        void handle_operations(or_node_base_operation* op_list) {
            or_node_base_operation *current;
            while(op_list) {
                current = op_list;
                op_list = op_list->next;
                switch(current->type) {

                case reg_succ:
                    my_successors.register_successor(*(current->my_succ));
                    __TBB_store_with_release(current->status, SUCCEEDED);
                    break;

                case rem_succ:
                    my_successors.remove_successor(*(current->my_succ));
                    __TBB_store_with_release(current->status, SUCCEEDED);
                    break;

                case try__put:
                    output_type oval;
                    or_helper<tuple_types,N>::create_output(oval,current->indx,current->my_arg);
                    if(my_successors.try_put(oval)) {
                            __TBB_store_with_release(current->status, SUCCEEDED);
                    }
                    else __TBB_store_with_release(current->status, FAILED);
                    break;
                }
            }
        }
        // ---------- end aggregator -----------
    public:
        or_node_base( ) : input_ports_type() {
            my_successors.set_owner(this);
            my_aggregator.initialize_handler(my_handler(this));
        }

        or_node_base( const or_node_base& /*other*/) : input_ports_type() {
            my_successors.set_owner(this);
            my_aggregator.initialize_handler(my_handler(this));
        }

        bool register_successor(successor_type &r) {
            or_node_base_operation op_data(r, reg_succ);
            my_aggregator.execute(&op_data);
            return op_data.status == SUCCEEDED;
        }

        bool remove_successor( successor_type &r) {
            or_node_base_operation op_data(r, rem_succ);
            my_aggregator.execute(&op_data);
            return op_data.status == SUCCEEDED;
        }

        bool try_put_with_index(size_t indx, void *v) {
            or_node_base_operation op_data(indx, v, try__put);
            my_aggregator.execute(&op_data);
            return op_data.status == SUCCEEDED;
        }

    private:
        broadcast_cache<output_type, null_rw_mutex> my_successors;
    };

    // type generators
    template<typename OutputTuple>
    struct or_types {
        static const int N = std::tuple_size<OutputTuple>::value;
        typedef typename or_input_type<N,OutputTuple>::type input_ports_tuple_type;
        typedef typename or_output_type<N, OutputTuple>::type output_type;
        typedef internal::or_node_FE<input_ports_tuple_type,output_type,OutputTuple> or_FE_type;
        typedef internal::or_node_base<input_ports_tuple_type, output_type, OutputTuple> or_base_type;
    };

    //! unfolded_or_node : passes input_ports_tuple_type to or_node_base.  We build the input port type
    //  using tuple_element.  The class PT is the port type (reserving_port, queueing_port, tag_matching_port)
    //  and should match the graph_buffer_policy.
    template<typename OutputTuple>
    class unfolded_or_node;

    template<class OutputTuple>
    class unfolded_or_node : public or_types<OutputTuple>::or_base_type {
    public:
        // static const int N = std::tuple_size<OutputTuple>::value;
        typedef typename or_types<OutputTuple>::input_ports_tuple_type input_ports_tuple_type;
        typedef OutputTuple tuple_types;
        typedef typename or_types<OutputTuple>::output_type output_type;
    private:
        typedef typename or_types<OutputTuple>::or_base_type base_type;
    public:
        unfolded_or_node() : base_type() {}
    };


} /* namespace internal */

#endif  /* __TBB__flow_graph_or_impl_H */
