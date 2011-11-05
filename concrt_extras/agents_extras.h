//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: agents_extras.h
//
//  Implementation of various useful message blocks
//
//--------------------------------------------------------------------------

#pragma once

#include <agents.h>

// bounded_buffer uses a map
#include <map>
#include <queue>

namespace Concurrency
{
    /// <summary>
    ///     Simple queue class for storing messages.
    /// </summary>
    /// <typeparam name="_Type">
    ///     The payload type of messages stored in this queue.
    /// </typeparam>
    template <class _Type>
    class MessageQueue
    {
    public:
        typedef message<_Type> _Message;

        /// <summary>
        ///     Constructs an initially empty queue.
        /// </summary>
        MessageQueue()
        {
        }

        /// <summary>
        ///     Removes and deletes any messages remaining in the queue.
        /// </summary>
        ~MessageQueue() 
        {
            _Message * _Msg = dequeue();
            while (_Msg != NULL)
            {
                delete _Msg;
                _Msg = dequeue();
            }
        }

        /// <summary>
        ///     Add an item to the queue.
        /// </summary>
        /// <param name="_Msg">
        ///     Message to add.
        /// </param>
        void enqueue(_Message *_Msg)
        {
            _M_queue.push(_Msg);
        }

        /// <summary>
        ///     Dequeue an item from the head of queue.
        /// </summary>
        /// <returns>
        ///     Returns a pointer to the message found at the head of the queue.
        /// </returns>
        _Message * dequeue()
        {
            _Message * _Msg = NULL;

            if (!_M_queue.empty())
            {
                _Msg = _M_queue.front();
                _M_queue.pop();
            }

            return _Msg;
        }

        /// <summary>
        ///     Return the item at the head of the queue, without dequeuing
        /// </summary>
        /// <returns>
        ///     Returns a pointer to the message found at the head of the queue.
        /// </returns>
        _Message * peek() const
        {
            _Message * _Msg = NULL;

            if (!_M_queue.empty())
            {
                _Msg = _M_queue.front();
            }

            return _Msg;
        }

        /// <summary>
        ///     Returns the number of items currently in the queue.
        /// </summary>
        /// <returns>
        /// Size of the queue.
        /// </returns>
        size_t count() const
        {
            return _M_queue.size();
        }

        /// <summary>
        ///     Checks to see if specified msg id is at the head of the queue.
        /// </summary>
        /// <param name="_MsgId">
        ///     Message id to check for.
        /// </param>
        /// <returns>
        ///     True if a message with specified id is at the head, false otherwise.
        /// </returns>
        bool is_head(const runtime_object_identity _MsgId) const
        {
            _Message * _Msg = peek();
            if(_Msg != NULL)
            {
                return _Msg->msg_id() == _MsgId;
            }
            return false;
        }

    private:
        
        std::queue<_Message *> _M_queue;
    };

    /// <summary>
    ///     Simple queue implementation that takes into account priority
    ///     using the comparison operator <.
    /// </summary>
    /// <typeparam name="_Type">
    ///     The payload type of messages stored in this queue.
    /// </typeparam>
    template <class _Type>
    class PriorityQueue
    {
    public:
        /// <summary>
        ///     Constructs an initially empty queue.
        /// </summary>
        PriorityQueue() : _M_pHead(NULL), _M_count(0) {}

        /// <summary>
        ///     Removes and deletes any messages remaining in the queue.
        /// </summary>
        ~PriorityQueue() 
        {
            message<_Type> * _Msg = dequeue();
            while (_Msg != NULL)
            {
                delete _Msg;
                _Msg = dequeue();
            }
        }

        /// <summary>
        ///     Add an item to the queue, comparisons using the 'payload' field
        ///     will determine the location in the queue.
        /// </summary>
        /// <param name="_Msg">
        ///     Message to add.
        /// </param>
        /// <param name="fCanReplaceHead">
        ///     True if this new message can be inserted at the head.
        /// </param>
        void enqueue(message<_Type> *_Msg, const bool fInsertAtHead = true)
        {
            MessageNode *_Element = new MessageNode();
            _Element->_M_pMsg = _Msg;

            // Find location to insert.
            MessageNode *pCurrent = _M_pHead;
            MessageNode *pPrev = NULL;
            if(!fInsertAtHead && pCurrent != NULL)
            {
                pPrev = pCurrent;
                pCurrent = pCurrent->_M_pNext;
            }
            while(pCurrent != NULL)
            {
                if(_Element->_M_pMsg->payload < pCurrent->_M_pMsg->payload)
                {
                    break;
                }
                pPrev = pCurrent;
                pCurrent = pCurrent->_M_pNext;
            }

            // Insert at head.
            if(pPrev == NULL)
            {
                _M_pHead = _Element;
            }
            else
            {
                pPrev->_M_pNext = _Element;
            }

            // Last item in queue.
            if(pCurrent == NULL)
            {
                _Element->_M_pNext = NULL;
            }
            else
            {
                _Element->_M_pNext = pCurrent;
            }

            ++_M_count;
        }

        /// <summary>
        ///     Dequeue an item from the head of queue.
        /// </summary>
        /// <returns>
        ///     Returns a pointer to the message found at the head of the queue.
        /// </returns>
        message<_Type> * dequeue()
        {
            if (_M_pHead == NULL) 
            {
                return NULL;
            }

            MessageNode *_OldHead = _M_pHead;
            message<_Type> * _Result = _OldHead->_M_pMsg;

            _M_pHead = _OldHead->_M_pNext;

            delete _OldHead;

            if(--_M_count == 0)
            {
                _M_pHead = NULL;
            }
            return _Result;
        }

        /// <summary>
        ///     Return the item at the head of the queue, without dequeuing
        /// </summary>
        /// <returns>
        ///     Returns a pointer to the message found at the head of the queue.
        /// </returns>
        message<_Type> * peek() const
        {
            if(_M_count != 0)
            {
                return _M_pHead->_M_pMsg;
            }
            return NULL;
        }

        /// <summary>
        ///     Returns the number of items currently in the queue.
        /// </summary>
        /// <returns>
        ///     Size of the queue.
        /// </returns>
        size_t count() const
        {
            return _M_count;
        }

        /// <summary>
        ///     Checks to see if specified msg id is at the head of the queue.
        /// </summary>
        /// <param name="_MsgId">
        ///     Message id to check for.
        /// </param>
        /// <returns>
        ///     True if a message with specified id is at the head, false otherwise.
        /// </returns>
        bool is_head(const runtime_object_identity _MsgId) const
        {
            if(_M_count != 0)
            {
                return _M_pHead->_M_pMsg->msg_id() == _MsgId;
            }
            return false;
        }

    private:
        
        // Used to store individual message nodes.
        struct MessageNode
        {
            MessageNode() : _M_pMsg(NULL), _M_pNext(NULL) {}
            message<_Type> * _M_pMsg;
            MessageNode * _M_pNext;
        };

        // A pointer to the head of the queue.
        MessageNode * _M_pHead;

        // The number of elements presently stored in the queue.
        size_t _M_count;
    };
	
    /// <summary>
    ///        priority_buffer is a buffer that uses a comparison operator on the 'payload' of each message to determine
    ///     order when offering to targets. Besides this it acts exactly like an unbounded_buffer.
    /// </summary>
    /// <typeparam name="_Type">
    ///     The payload type of messages stored and propagated by the buffer.
    /// </typeparam>
    template<class _Type>
    class priority_buffer : public propagator_block<multi_link_registry<ITarget<_Type>>, multi_link_registry<ISource<_Type>>>
    {
    public:

        /// <summary>
        ///     Creates an priority_buffer within the default scheduler, and places it any schedule
        ///     group of the scheduler’s choosing.
        /// </summary>
        priority_buffer() 
        {
            initialize_source_and_target();
        }

        /// <summary>
        ///     Creates an priority_buffer within the default scheduler, and places it any schedule
        ///     group of the scheduler’s choosing.
        /// </summary>
        /// <param name="_Filter">
        ///     A reference to a filter function.
        /// </param>
        priority_buffer(filter_method const& _Filter)
        {
            initialize_source_and_target();
            register_filter(_Filter);
        }

        /// <summary>
        ///     Creates an priority_buffer within the specified scheduler, and places it any schedule
        ///     group of the scheduler’s choosing.
        /// </summary>
        /// <param name="_PScheduler">
        ///     A reference to a scheduler instance.
        /// </param>
        priority_buffer(Scheduler& _PScheduler)
        {
            initialize_source_and_target(&_PScheduler);
        }

        /// <summary>
        ///     Creates an priority_buffer within the specified scheduler, and places it any schedule
        ///     group of the scheduler’s choosing.
        /// </summary>
        /// <param name="_PScheduler">
        ///     A reference to a scheduler instance.
        /// </param>
        /// <param name="_Filter">
        ///     A reference to a filter function.
        /// </param>
        priority_buffer(Scheduler& _PScheduler, filter_method const& _Filter) 
        {
            initialize_source_and_target(&_PScheduler);
            register_filter(_Filter);
        }

        /// <summary>
        ///     Creates an priority_buffer within the specified schedule group.  The scheduler is implied
        ///     by the schedule group.
        /// </summary>
        /// <param name="_PScheduleGroup">
        ///     A reference to a schedule group.
        /// </param>
        priority_buffer(ScheduleGroup& _PScheduleGroup)
        {
            initialize_source_and_target(NULL, &_PScheduleGroup);
        }

        /// <summary>
        ///     Creates an priority_buffer within the specified schedule group.  The scheduler is implied
        ///     by the schedule group.
        /// </summary>
        /// <param name="_PScheduleGroup">
        ///     A reference to a schedule group.
        /// </param>
        /// <param name="_Filter">
        ///     A reference to a filter function.
        /// </param>
        priority_buffer(ScheduleGroup& _PScheduleGroup, filter_method const& _Filter)
        {
            initialize_source_and_target(NULL, &_PScheduleGroup);
            register_filter(_Filter);
        }

        /// <summary>
        ///     Cleans up any resources that may have been created by the priority_buffer.
        /// </summary>
        ~priority_buffer()
        {
            // Remove all links
            remove_network_links();
        }

        /// <summary>
        ///     Add an item to the priority_buffer
        /// </summary>
        /// <param name="_Item">
        ///     A reference to the item to add.
        /// </param>
        /// <returns>
        ///     A boolean indicating whether the data was accepted.
        /// </returns>
        bool enqueue(_Type const& _Item)
        {
            return Concurrency::send<_Type>(this, _Item);
        }

        /// <summary>
        ///     Remove an item from the priority_buffer
        /// </summary>
        /// <returns>
        ///     The message payload.
        /// </returns>
        _Type dequeue()
        {
            return receive<_Type>(this);
        }

    protected:

        /// <summary>
        ///     The main propagate() function for ITarget blocks.  Called by a source
        ///     block, generally within an asynchronous task to send messages to its targets.
        /// </summary>
        /// <param name="_PMessage">
        ///     A pointer to the message.
        /// </param>
        /// <param name="_PSource">
        ///     A pointer to the source block offering the message.
        /// </param>
        /// <returns>
        ///     An indication of what the target decided to do with the message.
        /// </returns>
        /// <remarks>
        ///     It is important that calls to propagate do *not* take the same lock on the
        ///     internal structure that is used by Consume and the LWT.  Doing so could
        ///     result in a deadlock with the Consume call.  (in the case of the priority_buffer,
        ///     this lock is the m_internalLock)
        /// </remarks>
        virtual message_status propagate_message(message<_Type> * _PMessage, ISource<_Type> * _PSource)
        {
            message_status _Result = accepted;
            //
            // Accept the message being propagated
            // Note: depending on the source block propagating the message
            // this may not necessarily be the same message (pMessage) first
            // passed into the function.
            //
            _PMessage = _PSource->accept(_PMessage->msg_id(), this);

            if (_PMessage != NULL)
            {
                async_send(_PMessage);
            }
            else
            {
                _Result = missed;
            }

            return _Result;
        }

        /// <summary>
        ///     Synchronously sends a message to this block.  When this function completes the message will
        ///     already have propagated into the block.
        /// </summary>
        /// <param name="_PMessage">
        ///     A pointer to the message.
        /// </param>
        /// <param name="_PSource">
        ///     A pointer to the source block offering the message.
        /// </param>
        /// <returns>
        ///     An indication of what the target decided to do with the message.
        /// </returns>
        virtual message_status send_message(message<_Type> * _PMessage, ISource<_Type> * _PSource)
        {
            _PMessage = _PSource->accept(_PMessage->msg_id(), this);

            if (_PMessage != NULL)
            {
                sync_send(_PMessage);
            }
            else
            {
                return missed;
            }

            return accepted;
        }

        /// <summary>
        ///     Accepts an offered message by the source, transferring ownership to the caller.
        /// </summary>
        /// <param name="_MsgId">
        ///     The runtime object identity of the message.
        /// </param>
        /// <returns>
        ///     A pointer to the message that the caller now has ownership of.
        /// </returns>
        virtual message<_Type> * accept_message(runtime_object_identity _MsgId)
        {
            //
            // Peek at the head message in the message buffer.  If the Ids match
            // dequeue and transfer ownership
            //
            message<_Type> * _Msg = NULL;

            if (_M_messageBuffer.is_head(_MsgId))
            {
                _Msg = _M_messageBuffer.dequeue();
            }

            return _Msg;
        }

        /// <summary>
        ///     Reserves a message previously offered by the source.
        /// </summary>
        /// <param name="_MsgId">
        ///     The runtime object identity of the message.
        /// </param>
        /// <returns>
        ///     A Boolean indicating whether the reservation worked or not.
        /// </returns>
        /// <remarks>
        ///     After 'reserve' is called, either 'consume' or 'release' must be called.
        /// </remarks>
        virtual bool reserve_message(runtime_object_identity _MsgId)
        {
            // Allow reservation if this is the head message
            return _M_messageBuffer.is_head(_MsgId);
        }

        /// <summary>
        ///     Consumes a message that was reserved previously.
        /// </summary>
        /// <param name="_MsgId">
        ///     The runtime object identity of the message.
        /// </param>
        /// <returns>
        ///     A pointer to the message that the caller now has ownership of.
        /// </returns>
        /// <remarks>
        ///     Similar to 'accept', but is always preceded by a call to 'reserve'.
        /// </remarks>
        virtual message<_Type> * consume_message(runtime_object_identity _MsgId)
        {
            // By default, accept the message
            return accept_message(_MsgId);
        }

        /// <summary>
        ///     Releases a previous message reservation.
        /// </summary>
        /// <param name="_MsgId">
        ///     The runtime object identity of the message.
        /// </param>
        virtual void release_message(runtime_object_identity _MsgId)
        {
            // The head message is the one reserved.
            if (!_M_messageBuffer.is_head(_MsgId))
            {
                throw message_not_found();
            }
        }

        /// <summary>
        ///    Resumes propagation after a reservation has been released
        /// </summary>
        virtual void resume_propagation()
        {
            // If there are any messages in the buffer, propagate them out
            if (_M_messageBuffer.count() > 0)
            {
                async_send(NULL);
            }
        }

        /// <summary>
        ///     Notification that a target was linked to this source.
        /// </summary>
        /// <param name="_PTarget">
        ///     A pointer to the newly linked target.
        /// </param>
        virtual void link_target_notification(ITarget<_Type> * _PTarget)
        {
            // If the message queue is blocked due to reservation
            // there is no need to do any message propagation
            if (_M_pReservedFor != NULL)
            {
                return;
            }

            message<_Type> * _Msg = _M_messageBuffer.peek();

            if (_Msg != NULL)
            {
                // Propagate the head message to the new target
                message_status _Status = _PTarget->propagate(_Msg, this);

                if (_Status == accepted)
                {
                    // The target accepted the message, restart propagation.
                    propagate_to_any_targets(NULL);
                }

                // If the status is anything other than accepted, then leave
                // the message queue blocked.
            }
        }

        /// <summary>
        /// Takes the message and propagates it to all the targets of this priority_buffer.
        /// </summary>
        /// <param name="_PMessage">
        ///     A pointer to a new message.
        /// </param>
        virtual void propagate_to_any_targets(message<_Type> * _PMessage)
        {
            // Enqueue pMessage to the internal unbounded buffer queue if it is non-NULL.
            // _PMessage can be NULL if this LWT was the result of a Repropagate call
            // out of a Consume or Release (where no new message is queued up, but
            // everything remaining in the priority_buffer needs to be propagated out)
            if (_PMessage != NULL)
            {
                message<_Type> *pPrevHead = _M_messageBuffer.peek();

                // If a reservation is held make sure to not insert this new
                // message before it.
                if(_M_pReservedFor != NULL)
                {
                    _M_messageBuffer.enqueue(_PMessage, false);
                }
                else
                {
                    _M_messageBuffer.enqueue(_PMessage);
                }

                // If the head message didn't change, we can safely assume that
                // the head message is blocked and waiting on Consume(), Release() or a new
                // link_target()
                if (pPrevHead != NULL && !_M_messageBuffer.is_head(pPrevHead->msg_id()))
                {
                    return;
                }
            }

            // Attempt to propagate messages to all the targets
            _Propagate_priority_order();
        }

    private:

        /// <summary>
        ///     Attempts to propagate out any messages currently in the block.
        /// </summary>
        void _Propagate_priority_order()
        {
            message<_Target_type> * _Msg = _M_messageBuffer.peek();

            // If someone has reserved the _Head message, don't propagate anymore
            if (_M_pReservedFor != NULL)
            {
                return;
            }

            while (_Msg != NULL)
            {
                message_status _Status = declined;

                // Always start from the first target that linked.
                for (target_iterator _Iter = _M_connectedTargets.begin(); *_Iter != NULL; ++_Iter)
                {
                    ITarget<_Target_type> * _PTarget = *_Iter;
                    _Status = _PTarget->propagate(_Msg, this);

                    // Ownership of message changed. Do not propagate this
                    // message to any other target.
                    if (_Status == accepted)
                    {
                        break;
                    }

                    // If the target just propagated to reserved this message, stop
                    // propagating it to others.
                    if (_M_pReservedFor != NULL)
                    {
                        break;
                    }
                }

                // If status is anything other than accepted, then the head message
                // was not propagated out.  Thus, nothing after it in the queue can
                // be propagated out.  Cease propagation.
                if (_Status != accepted)
                {
                    break;
                }

                // Get the next message
                _Msg = _M_messageBuffer.peek();
            }
        }

        /// <summary>
        ///     Priority Queue used to store messages.
        /// </summary>
        PriorityQueue<_Type> _M_messageBuffer;

        //
        // Hide assignment operator and copy constructor.
        //
        priority_buffer const &operator =(priority_buffer const&);  // no assignment operator
        priority_buffer(priority_buffer const &);                   // no copy constructor
    };


    /// <summary>
    ///    A bounded_buffer implementation. Once the capacity is reached it will save the offered message
    ///    id and postpone. Once below capacity again the bounded_buffer will try to reserve and consume
    ///    any of the postponed messages. Preference is given to previously offered messages before new ones.
    ///
    ///    NOTE: this bounded_buffer implementation contains code that is very unique to this particular block. 
    ///          Extreme caution should be taken if code is directly copy and pasted from this class. The bounded_buffer
    ///          implementation uses a critical_section, several interlocked operations, and additional calls to async_send.
    ///          These are needed to not abandon a previously saved message id. Most blocks never have to deal with this problem.
    /// </summary>
    /// <typeparam name="_Type">
    ///     The payload type of messages stored and propagated by the buffer.
    /// </typeparam>
    template<class _Type>
    class bounded_buffer : public propagator_block<multi_link_registry<ITarget<_Type>>, multi_link_registry<ISource<_Type>>>
    {
    public:
        /// <summary>
        ///     Creates an bounded_buffer within the default scheduler, and places it any schedule
        ///     group of the scheduler’s choosing.
        /// </summary>
        bounded_buffer(const size_t capacity)
            : _M_capacity(capacity), _M_currentSize(0)
        {
            initialize_source_and_target();
        }

        /// <summary>
        ///     Creates an bounded_buffer within the default scheduler, and places it any schedule
        ///     group of the scheduler’s choosing.
        /// </summary>
        /// <param name="_Filter">
        ///     A reference to a filter function.
        /// </param>
        bounded_buffer(const size_t capacity, filter_method const& _Filter)
            : _M_capacity(capacity), _M_currentSize(0)
        {
            initialize_source_and_target();
            register_filter(_Filter);
        }

        /// <summary>
        ///     Creates an bounded_buffer within the specified scheduler, and places it any schedule
        ///     group of the scheduler’s choosing.
        /// </summary>
        /// <param name="_PScheduler">
        ///     A reference to a scheduler instance.
        /// </param>
        bounded_buffer(const size_t capacity, Scheduler& _PScheduler)
            : _M_capacity(capacity), _M_currentSize(0)
        {
            initialize_source_and_target(&_PScheduler);
        }

        /// <summary>
        ///     Creates an bounded_buffer within the specified scheduler, and places it any schedule
        ///     group of the scheduler’s choosing.
        /// </summary>
        /// <param name="_PScheduler">
        ///     A reference to a scheduler instance.
        /// </param>
        /// <param name="_Filter">
        ///     A reference to a filter function.
        /// </param>
        bounded_buffer(const size_t capacity, Scheduler& _PScheduler, filter_method const& _Filter) 
            : _M_capacity(capacity), _M_currentSize(0)
        {
            initialize_source_and_target(&_PScheduler);
            register_filter(_Filter);
        }

        /// <summary>
        ///     Creates an bounded_buffer within the specified schedule group.  The scheduler is implied
        ///     by the schedule group.
        /// </summary>
        /// <param name="_PScheduleGroup">
        ///     A reference to a schedule group.
        /// </param>
        bounded_buffer(const size_t capacity, ScheduleGroup& _PScheduleGroup)
            : _M_capacity(capacity), _M_currentSize(0)
        {
            initialize_source_and_target(NULL, &_PScheduleGroup);
        }

        /// <summary>
        ///     Creates an bounded_buffer within the specified schedule group.  The scheduler is implied
        ///     by the schedule group.
        /// </summary>
        /// <param name="_PScheduleGroup">
        ///     A reference to a schedule group.
        /// </param>
        /// <param name="_Filter">
        ///     A reference to a filter function.
        /// </param>
        bounded_buffer(const size_t capacity, ScheduleGroup& _PScheduleGroup, filter_method const& _Filter)
            : _M_capacity(capacity), _M_currentSize(0)
        {
            initialize_source_and_target(NULL, &_PScheduleGroup);
            register_filter(_Filter);
        }

        /// <summary>
        ///     Cleans up any resources that may have been used by the bounded_buffer.
        /// </summary>
        ~bounded_buffer()
        {
            // Remove all links
            remove_network_links();
        }

        /// <summary>
        ///     Add an item to the bounded_buffer.
        /// </summary>
        /// <param name="_Item">
        ///     A reference to the item to add.
        /// </param>
        /// <returns>
        ///     A boolean indicating whether the data was accepted.
        /// </returns>
        bool enqueue(_Type const& _Item)
        {
            return Concurrency::send<_Type>(this, _Item);
        }

        /// <summary>
        ///     Remove an item from the bounded_buffer.
        /// </summary>
        /// <returns>
        ///     The message payload.
        /// </returns>
        _Type dequeue()
        {
            return receive<_Type>(this);
        }

    protected:

        /// <summary>
        ///     The main propagate() function for ITarget blocks.  Called by a source
        ///     block, generally within an asynchronous task to send messages to its targets.
        /// </summary>
        /// <param name="_PMessage">
        ///     A pointer to the message.
        /// </param>
        /// <param name="_PSource">
        ///     A pointer to the source block offering the message.
        /// </param>
        /// <returns>
        ///     An indication of what the target decided to do with the message.
        /// </returns>
        /// <remarks>
        ///     It is important that calls to propagate do *not* take the same lock on the
        ///     internal structure that is used by Consume and the LWT.  Doing so could
        ///     result in a deadlock with the Consume call.  (in the case of the bounded_buffer,
        ///     this lock is the m_internalLock)
        /// </remarks>
        virtual message_status propagate_message(message<_Type> * _PMessage, ISource<_Type> * _PSource)
        {
            message_status _Result = accepted;
            
            // Check current capacity. 
            if((size_t)_InterlockedIncrement(&_M_currentSize) > _M_capacity)
            {
                // Postpone the message, buffer is full.
                _InterlockedDecrement(&_M_currentSize);
                _Result = postponed;

                // Save off the message id from this source to later try
                // and reserve/consume when more space is free.
                {
                    critical_section::scoped_lock scopedLock(_M_savedIdsLock);
                    _M_savedSourceMsgIds[_PSource] = _PMessage->msg_id();
                }

                async_send(NULL);
            }
            else
            {
                //
                // Accept the message being propagated
                // Note: depending on the source block propagating the message
                // this may not necessarily be the same message (pMessage) first
                // passed into the function.
                //
                _PMessage = _PSource->accept(_PMessage->msg_id(), this);

                if (_PMessage != NULL)
                {
                    async_send(_PMessage);
                }
                else
                {
                    // Didn't get a message so need to decrement.
                    _InterlockedDecrement(&_M_currentSize);
                    _Result = missed;
                    async_send(NULL);
                }
            }

            return _Result;
        }

        /// <summary>
        ///     Synchronously sends a message to this block.  When this function completes the message will
        ///     already have propagated into the block.
        /// </summary>
        /// <param name="_PMessage">
        ///     A pointer to the message.
        /// </param>
        /// <param name="_PSource">
        ///     A pointer to the source block offering the message.
        /// </param>
        /// <returns>
        ///     An indication of what the target decided to do with the message.
        /// </returns>
        virtual message_status send_message(message<_Type> * _PMessage, ISource<_Type> * _PSource)
        {
            message_status _Result = accepted;
            
            // Check current capacity. 
            if((size_t)_InterlockedIncrement(&_M_currentSize) > _M_capacity)
            {
                // Postpone the message, buffer is full.
                _InterlockedDecrement(&_M_currentSize);
                _Result = postponed;

                // Save off the message id from this source to later try
                // and reserve/consume when more space is free.
                {
                    critical_section::scoped_lock scopedLock(_M_savedIdsLock);
                    _M_savedSourceMsgIds[_PSource] = _PMessage->msg_id();
                }

                async_send(NULL);
            }
            else
            {
                //
                // Accept the message being propagated
                // Note: depending on the source block propagating the message
                // this may not necessarily be the same message (pMessage) first
                // passed into the function.
                //
                _PMessage = _PSource->accept(_PMessage->msg_id(), this);

                if (_PMessage != NULL)
                {
                    async_send(_PMessage);
                }
                else
                {
                    // Didn't get a message so need to decrement.
                    _InterlockedDecrement(&_M_currentSize);
                    _Result = missed;
                    async_send(NULL);
                }
            }

            return _Result;
        }

        /// <summary>
        ///     Accepts an offered message by the source, transferring ownership to the caller.
        /// </summary>
        /// <param name="_MsgId">
        ///     The runtime object identity of the message.
        /// </param>
        /// <returns>
        ///     A pointer to the message that the caller now has ownership of.
        /// </returns>
        virtual message<_Type> * accept_message(runtime_object_identity _MsgId)
        {
            //
            // Peek at the head message in the message buffer.  If the Ids match
            // dequeue and transfer ownership
            //
            message<_Type> * _Msg = NULL;

            if (_M_messageBuffer.is_head(_MsgId))
            {
                _Msg = _M_messageBuffer.dequeue();

                // Give preference to any previously postponed messages
                // before decrementing current size.
                if(!try_consume_msg())
                {
                    _InterlockedDecrement(&_M_currentSize);
                }
            }

            return _Msg;
        }

        /// <summary>
        ///     Try to reserve and consume a message from list of saved message ids.
        /// </summary>
        /// <returns>
        ///     True if a message was sucessfully consumed, false otherwise.
        /// </returns>
        bool try_consume_msg()
        {
            runtime_object_identity _ReservedId = -1;
            ISource<_Type> * _PSource = NULL;

            // Walk through source links seeing if any saved ids exist.
            bool _ConsumedMsg = true;
            while(_ConsumedMsg)
            {
                source_iterator _Iter = _M_connectedSources.begin();
                {
                    critical_section::scoped_lock scopedLock(_M_savedIdsLock);
                    for (; *_Iter != NULL; ++_Iter)
                    {
                        _PSource = *_Iter;
                        std::map<ISource<_Type> *, runtime_object_identity>::iterator _MapIter;
                        if((_MapIter = _M_savedSourceMsgIds.find(_PSource)) != _M_savedSourceMsgIds.end())
                        {
                            _ReservedId = _MapIter->second;
                            _M_savedSourceMsgIds.erase(_MapIter);
                            break;
                        }
                    }
                }

                // Can't call into source block holding _M_savedIdsLock, that would be a recipe for disaster.
                if(_ReservedId != -1)
                {
                    if(_PSource->reserve(_ReservedId, this))
                    {
                        message<_Type> * _ConsumedMsg = _PSource->consume(_ReservedId, this);
                        async_send(_ConsumedMsg);
                        return true;
                    }
                    // Reserve failed go or link was removed, 
                    // go back and try and find a different msg id.
                    else
                    {
                        continue;
                    }
                }

                // If this point is reached the map of source ids was empty.
                break;
            }

            return false;
        }

        /// <summary>
        ///     Reserves a message previously offered by the source.
        /// </summary>
        /// <param name="_MsgId">
        ///     The runtime object identity of the message.
        /// </param>
        /// <returns>
        ///     A Boolean indicating whether the reservation worked or not.
        /// </returns>
        /// <remarks>
        ///     After 'reserve' is called, either 'consume' or 'release' must be called.
        /// </remarks>
        virtual bool reserve_message(runtime_object_identity _MsgId)
        {
            // Allow reservation if this is the head message
            return _M_messageBuffer.is_head(_MsgId);
        }

        /// <summary>
        ///     Consumes a message that was reserved previously.
        /// </summary>
        /// <param name="_MsgId">
        ///     The runtime object identity of the message.
        /// </param>
        /// <returns>
        ///     A pointer to the message that the caller now has ownership of.
        /// </returns>
        /// <remarks>
        ///     Similar to 'accept', but is always preceded by a call to 'reserve'.
        /// </remarks>
        virtual message<_Type> * consume_message(runtime_object_identity _MsgId)
        {
            // By default, accept the message
            return accept_message(_MsgId);
        }

        /// <summary>
        ///     Releases a previous message reservation.
        /// </summary>
        /// <param name="_MsgId">
        ///     The runtime object identity of the message.
        /// </param>
        virtual void release_message(runtime_object_identity _MsgId)
        {
            // The head message is the one reserved.
            if (!_M_messageBuffer.is_head(_MsgId))
            {
                throw message_not_found();
            }
        }

        /// <summary>
        ///    Resumes propagation after a reservation has been released
        /// </summary>
        virtual void resume_propagation()
        {
            // If there are any messages in the buffer, propagate them out
            if (_M_messageBuffer.count() > 0)
            {
                async_send(NULL);
            }
        }

        /// <summary>
        ///     Notification that a target was linked to this source.
        /// </summary>
        /// <param name="_PTarget">
        ///     A pointer to the newly linked target.
        /// </param>
        virtual void link_target_notification(ITarget<_Type> * _PTarget)
        {
            // If the message queue is blocked due to reservation
            // there is no need to do any message propagation
            if (_M_pReservedFor != NULL)
            {
                return;
            }

            message<_Type> * _Msg = _M_messageBuffer.peek();

            if (_Msg != NULL)
            {
                // Propagate the head message to the new target
                message_status _Status = _PTarget->propagate(_Msg, this);

                if (_Status == accepted)
                {
                    // The target accepted the message, restart propagation.
                    propagate_to_any_targets(NULL);
                }

                // If the status is anything other than accepted, then leave
                // the message queue blocked.
            }
        }

        /// <summary>
        ///     Takes the message and propagates it to all the targets of this bounded_buffer.
        ///     This is called from async_send.
        /// </summary>
        /// <param name="_PMessage">
        ///     A pointer to a new message.
        /// </param>
		virtual void propagate_to_any_targets(message<_Type> * _PMessage)
        {
            // Enqueue pMessage to the internal message buffer if it is non-NULL.
            // pMessage can be NULL if this LWT was the result of a Repropagate call
           // out of a Consume or Release (where no new message is queued up, but
            // everything remaining in the bounded buffer needs to be propagated out)
            if (_PMessage != NULL)
            {
                _M_messageBuffer.enqueue(_PMessage);

                // If the incoming pMessage is not the head message, we can safely assume that
                // the head message is blocked and waiting on Consume(), Release() or a new
                // link_target() and cannot be propagated out.
                if (!_M_messageBuffer.is_head(_PMessage->msg_id()))
                {
                    return;
                }
            }

            _Propagate_priority_order();
            
            {

                // While current size is less than capacity try to consume
                // any previously offered ids.
                bool _ConsumedMsg = true;
                while(_ConsumedMsg)
                {
                    // Assume a message will be found to successfully consume in the
                    // saved ids, if not this will be decremented afterwards.
                    if((size_t)_InterlockedIncrement(&_M_currentSize) > _M_capacity)
                    {
                        break;
                    }

                    _ConsumedMsg = try_consume_msg();
                }

                // Decrement the current size, we broke out of the previous loop
                // because we reached capacity or there were no more messages to consume.
                _InterlockedDecrement(&_M_currentSize);
            }
        }

    private:

        /// <summary>
        ///     Attempts to propagate out any messages currently in the block.
        /// </summary>
        void _Propagate_priority_order()
        {
            message<_Target_type> * _Msg = _M_messageBuffer.peek();

            // If someone has reserved the _Head message, don't propagate anymore
            if (_M_pReservedFor != NULL)
            {
                return;
            }

            while (_Msg != NULL)
            {
                message_status _Status = declined;

                // Always start from the first target that linked.
                for (target_iterator _Iter = _M_connectedTargets.begin(); *_Iter != NULL; ++_Iter)
                {
                    ITarget<_Target_type> * _PTarget = *_Iter;
                    _Status = _PTarget->propagate(_Msg, this);

                    // Ownership of message changed. Do not propagate this
                    // message to any other target.
                    if (_Status == accepted)
                    {
                        break;
                    }

                    // If the target just propagated to reserved this message, stop
                    // propagating it to others.
                    if (_M_pReservedFor != NULL)
                    {
                        break;
                    }
                }

                // If status is anything other than accepted, then the head message
                // was not propagated out.  Thus, nothing after it in the queue can
                // be propagated out.  Cease propagation.
                if (_Status != accepted)
                {
                    break;
                }

                // Get the next message
                _Msg = _M_messageBuffer.peek();
            }
        }

        /// <summary>
        ///     Message buffer used to store messages.
        /// </summary>
        MessageQueue<_Type> _M_messageBuffer;

        /// <summary>
        ///     Maximum number of messages bounded_buffer can hold.
        /// </summary>
        const size_t _M_capacity;

        /// <summary>
        ///     Current number of messages in bounded_buffer.
        /// </summary>
        volatile long _M_currentSize;

        /// <summary>
        ///     Lock used to guard saved message ids map.
        /// </summary>
        critical_section _M_savedIdsLock;

        /// <summary>
        ///     Map of source links to saved message ids.
        /// </summary>
        std::map<ISource<_Type> *, runtime_object_identity> _M_savedSourceMsgIds;

        //
        // Hide assignment operator and copy constructor
        //
        bounded_buffer const &operator =(bounded_buffer const&);  // no assignment operator
        bounded_buffer(bounded_buffer const &);                   // no copy constructor
    };

    /// <summary>
    ///        A simple alternator, offers messages in order to each target
    ///     one at a time. If a consume occurs a message won't be offered to that target again
    ///     until all others are given a chance. This causes messages to be distributed more
    ///     evenly among targets.
    /// </summary>
    /// <typeparam name="_Type">
    ///     The payload type of messages stored and propagated by the buffer.
    /// </typeparam>
    template<class _Type>
    class alternator : public propagator_block<multi_link_registry<ITarget<_Type>>, multi_link_registry<ISource<_Type>>>
    {
    public:
        /// <summary>
        ///     Create an alternator within the default scheduler, and places it any schedule
        ///     group of the scheduler’s choosing.
        /// </summary>
        alternator()
            : _M_indexNextTarget(0)
        {
            initialize_source_and_target();
        }

        /// <summary>
        ///     Creates an alternator within the default scheduler, and places it any schedule
        ///     group of the scheduler’s choosing.
        /// </summary>
        /// <param name="_Filter">
        ///     A reference to a filter function.
        /// </param>
        alternator(filter_method const& _Filter)
            : _M_indexNextTarget(0)
        {
            initialize_source_and_target();
            register_filter(_Filter);
        }

        /// <summary>
        ///     Creates an alternator within the specified scheduler, and places it any schedule
        ///     group of the scheduler’s choosing.
        /// </summary>
        /// <param name="_PScheduler">
        ///     A reference to a scheduler instance.
        /// </param>
        alternator(Scheduler& _PScheduler)
            : _M_indexNextTarget(0)
        {
            initialize_source_and_target(&_PScheduler);
        }

        /// <summary>
        ///     Creates an alternator within the specified scheduler, and places it any schedule
        ///     group of the scheduler’s choosing.
        /// </summary>
        /// <param name="_PScheduler">
        ///     A reference to a scheduler instance.
        /// </param>
        /// <param name="_Filter">
        ///     A reference to a filter function.
        /// </param>
        alternator(Scheduler& _PScheduler, filter_method const& _Filter) 
            : _M_indexNextTarget(0)
        {
            initialize_source_and_target(&_PScheduler);
            register_filter(_Filter);
        }

        /// <summary>
        ///     Creates an alternator within the specified schedule group.  The scheduler is implied
        ///     by the schedule group.
        /// </summary>
        /// <param name="_PScheduleGroup">
        ///     A reference to a schedule group.
        /// </param>
        alternator(ScheduleGroup& _PScheduleGroup)
            : _M_indexNextTarget(0)
        {
            initialize_source_and_target(NULL, &_PScheduleGroup);
        }

        /// <summary>
        ///     Creates an alternator within the specified schedule group.  The scheduler is implied
        ///     by the schedule group.
        /// </summary>
        /// <param name="_PScheduleGroup">
        ///     A reference to a schedule group.
        /// </param>
        /// <param name="_Filter">
        ///     A reference to a filter function.
        /// </param>
        alternator(ScheduleGroup& _PScheduleGroup, filter_method const& _Filter)
            : _M_indexNextTarget(0)
        {
            initialize_source_and_target(NULL, &_PScheduleGroup);
            register_filter(_Filter);
        }

        /// <summary>
        ///     Cleans up any resources that may have been created by the alternator.
        /// </summary>
        ~alternator()
        {
            // Remove all links
            remove_network_links();
        }

    protected:

        /// <summary>
        ///     The main propagate() function for ITarget blocks.  Called by a source
        ///     block, generally within an asynchronous task to send messages to its targets.
        /// </summary>
        /// <param name="_PMessage">
        ///     A pointer to the message
        /// </param>
        /// <param name="_PSource">
        ///     A pointer to the source block offering the message.
        /// </param>
        /// <returns>
        ///     An indication of what the target decided to do with the message.
        /// </returns>
        /// <remarks>
        ///     It is important that calls to propagate do *not* take the same lock on the
        ///     internal structure that is used by Consume and the LWT.  Doing so could
        ///     result in a deadlock with the Consume call.  (in the case of the alternator,
        ///     this lock is the m_internalLock)
        /// </remarks>
        virtual message_status propagate_message(message<_Type> * _PMessage, ISource<_Type> * _PSource)
        {
            message_status _Result = accepted;
            //
            // Accept the message being propagated
            // Note: depending on the source block propagating the message
            // this may not necessarily be the same message (pMessage) first
            // passed into the function.
            //
            _PMessage = _PSource->accept(_PMessage->msg_id(), this);

            if (_PMessage != NULL)
            {
                async_send(_PMessage);
            }
            else
            {
                _Result = missed;
            }

            return _Result;
        }

        /// <summary>
        ///     Synchronously sends a message to this block.  When this function completes the message will
        ///     already have propagated into the block.
        /// </summary>
        /// <param name="_PMessage">
        ///     A pointer to the message.
        /// </param>
        /// <param name="_PSource">
        ///     A pointer to the source block offering the message.
        /// </param>
        /// <returns>
        ///     An indication of what the target decided to do with the message.
        /// </returns>
        virtual message_status send_message(message<_Type> * _PMessage, ISource<_Type> * _PSource)
        {
            _PMessage = _PSource->accept(_PMessage->msg_id(), this);

            if (_PMessage != NULL)
            {
                sync_send(_PMessage);
            }
            else
            {
                return missed;
            }

            return accepted;
        }

        /// <summary>
        ///     Accepts an offered message by the source, transferring ownership to the caller.
        /// </summary>
        /// <param name="_MsgId">
        ///     The runtime object identity of the message.
        /// </param>
        /// <returns>
        ///     A pointer to the message that the caller now has ownership of.
        /// </returns>
        virtual message<_Type> * accept_message(runtime_object_identity _MsgId)
        {
            //
            // Peek at the head message in the message buffer.  If the Ids match
            // dequeue and transfer ownership
            //
            message<_Type> * _Msg = NULL;

            if (_M_messageBuffer.is_head(_MsgId))
            {
                _Msg = _M_messageBuffer.dequeue();
            }

            return _Msg;
        }

        /// <summary>
        ///     Reserves a message previously offered by the source.
        /// </summary>
        /// <param name="_MsgId">
        ///     The runtime object identity of the message.
        /// </param>
        /// <returns>
        ///     A Boolean indicating whether the reservation worked or not.
        /// </returns>
        /// <remarks>
        ///     After 'reserve' is called, either 'consume' or 'release' must be called.
        /// </remarks>
        virtual bool reserve_message(runtime_object_identity _MsgId)
        {
            // Allow reservation if this is the head message
            return _M_messageBuffer.is_head(_MsgId);
        }

        /// <summary>
        ///     Consumes a message that was reserved previously.
        /// </summary>
        /// <param name="_MsgId">
        ///     The runtime object identity of the message.
        /// </param>
        /// <returns>
        ///     A pointer to the message that the caller now has ownership of.
        /// </returns>
        /// <remarks>
        ///     Similar to 'accept', but is always preceded by a call to 'reserve'.
        /// </remarks>
        virtual message<_Type> * consume_message(runtime_object_identity _MsgId)
        {
            // Update so we don't offer to this target again until
            // all others have a chance.
            target_iterator _CurrentIter = _M_connectedTargets.begin();
            for(size_t i = 0;*_CurrentIter != NULL; ++_CurrentIter, ++i) 
            {
                if(*_CurrentIter == _M_pReservedFor)
                {
                    _M_indexNextTarget = i + 1;
                    break;
                }
            }

            // By default, accept the message
            return accept_message(_MsgId);
        }

        /// <summary>
        ///     Releases a previous message reservation.
        /// </summary>
        /// <param name="_MsgId">
        ///     The runtime object identity of the message.
        /// </param>
        virtual void release_message(runtime_object_identity _MsgId)
        {
            // The head message is the one reserved.
            if (!_M_messageBuffer.is_head(_MsgId))
            {
                throw message_not_found();
            }
        }

        /// <summary>
        ///    Resumes propagation after a reservation has been released.
        /// </summary>
        virtual void resume_propagation()
        {
            // If there are any messages in the buffer, propagate them out
            if (_M_messageBuffer.count() > 0)
            {
                async_send(NULL);
            }
        }

        /// <summary>
        ///     Notification that a target was linked to this source.
        /// </summary>
        /// <param name="_PTarget">
        ///     A pointer to the newly linked target.
        /// </param>
        virtual void link_target_notification(ITarget<_Type> * _PTarget)
        {
            // If the message queue is blocked due to reservation
            // there is no need to do any message propagation
            if (_M_pReservedFor != NULL)
            {
                return;
            }

            message<_Type> * _Msg = _M_messageBuffer.peek();

            if (_Msg != NULL)
            {
                // Propagate the head message to the new target
                message_status _Status = _PTarget->propagate(_Msg, this);

                if (_Status == accepted)
                {
                    // The target accepted the message, restart propagation.
                    propagate_to_any_targets(NULL);
                }

                // If the status is anything other than accepted, then leave
                // the message queue blocked.
            }
        }

        /// <summary>
        ///     Takes the message and propagates it to all the targets of this alternator.
        ///     This is called from async_send.
        /// </summary>
        /// <param name="_PMessage">
        ///     A pointer to a new message.
        /// </param>
        virtual void propagate_to_any_targets(message<_Type> * _PMessage)
        {
            // Enqueue pMessage to the internal buffer queue if it is non-NULL.
            // pMessage can be NULL if this LWT was the result of a Repropagate call
            // out of a Consume or Release (where no new message is queued up, but
            // everything remaining in the unbounded buffer needs to be propagated out)
            if (_PMessage != NULL)
            {
                _M_messageBuffer.enqueue(_PMessage);

                // If the incoming pMessage is not the head message, we can safely assume that
                // the head message is blocked and waiting on Consume(), Release() or a new
                // link_target()
                if (!_M_messageBuffer.is_head(_PMessage->msg_id()))
                {
                    return;
                }
            }

            // Attempt to propagate messages to targets in order last left off.
            _Propagate_alternating_order();
        }

        /// <summary>
        ///     Offers messages to targets in alternating order to help distribute messages
        ///     evenly among targets.
        /// </summary>
        void _Propagate_alternating_order()
        {
            message<_Target_type> * _Msg = _M_messageBuffer.peek();

            // If someone has reserved the _Head message, don't propagate anymore
            if (_M_pReservedFor != NULL)
            {
                return;
            }

            //
            // Try to start where left off before, if the link has been removed
            // or this is the first time then start at the beginning.
            //
            target_iterator _CurrentIter = _M_connectedTargets.begin();
            const target_iterator _FirstLinkIter(_CurrentIter);
            for(size_t i = 0;*_CurrentIter != NULL && i < _M_indexNextTarget; ++_CurrentIter, ++i) {}

            while (_Msg != NULL)
            {
                message_status _Status = declined;

                // Loop offering message until end of links is reached.
                target_iterator _StartedIter(_CurrentIter);
                for(;*_CurrentIter != NULL; ++_CurrentIter)
                {
                    _Status = (*_CurrentIter)->propagate(_Msg, this);
                    ++_M_indexNextTarget;

                    // Ownership of message changed. Do not propagate this
                    // message to any other target.
                    if (_Status == accepted)
                    {
                        ++_CurrentIter;
                        break;
                    }

                    // If the target just propagated to reserved this message, stop
                    // propagating it to others
                    if (_M_pReservedFor != NULL)
                    {
                        return;
                    }
                }

                // Message ownership changed go to next messages.
                if (_Status == accepted)
                {
                    continue;
                }

                // Try starting from the beginning until the first link offering was started at.
                _M_indexNextTarget = 0;
                for(_CurrentIter = _FirstLinkIter;*_CurrentIter != NULL; ++_CurrentIter)
                {
                    // I have offered the same message to all links now so stop.
                    if(*_CurrentIter == *_StartedIter)
                    {
                        break;
                    }

                    _Status = (*_CurrentIter)->propagate(_Msg, this);
                    ++_M_indexNextTarget;

                    // Ownership of message changed. Do not propagate this
                    // message to any other target.
                    if (_Status == accepted)
                    {
                        ++_CurrentIter;
                        break;
                    }

                    // If the target just propagated to reserved this message, stop
                    // propagating it to others
                    if (_M_pReservedFor != NULL)
                    {
                        return;
                    }
                }

                // If status is anything other than accepted, then the head message
                // was not propagated out.  Thus, nothing after it in the queue can
                // be propagated out.  Cease propagation.
                if (_Status != accepted)
                {
                    break;
                }

                // Get the next message
                _Msg = _M_messageBuffer.peek();
            }
        }

    private:

        /// <summary>
        ///     Message queue used to store messages.
        /// </summary>
        MessageQueue<_Type> _M_messageBuffer;

        /// <summary>
        ///     Index of next target to call propagate on. Used to alternate and load
        ///     balance message offering.
        /// </summary>
        size_t _M_indexNextTarget;

        //
        // Hide assignment operator and copy constructor.
        //
        alternator const &operator =(alternator const&);  // no assignment operator
        alternator(alternator const &);                   // no copy constructor
    };

    #include <agents.h>
	
    //
    // Sample block that combines join and transform.
    //
    template<class _Input, class _Output, join_type _Jtype = non_greedy>
    class join_transform : public propagator_block<single_link_registry<ITarget<_Output>>, multi_link_registry<ISource<_Input>>>
    {
    public:

        typedef std::tr1::function<_Output(std::vector<_Input> const&)> _Transform_method;

        /// <summary>
        ///     Create an join block within the default scheduler, and places it any schedule
        ///     group of the scheduler’s choosing.
        /// </summary>
        /// <param name="_NumInputs">
        ///     The number of inputs this join will be allowed
        /// </param>
        join_transform(size_t _NumInputs, _Transform_method const& _Func)
            : _M_messageArray(_NumInputs, 0),
              _M_savedMessageIdArray(_NumInputs, -1),
              _M_pFunc(_Func)
        {
            _Initialize(_NumInputs);
        }

        /// <summary>
        ///     Create an join block within the default scheduler, and places it any schedule
        ///     group of the scheduler’s choosing.
        /// </summary>
        /// <param name="_NumInputs">
        ///     The number of inputs this join will be allowed
        /// </param>
        /// <param name="_Filter">
        ///     A filter method placed on this join
        /// </param>
        join_transform(size_t _NumInputs, _Transform_method const& _Func, filter_method const& _Filter)
            : _M_messageArray(_NumInputs, 0),
              _M_savedMessageIdArray(_NumInputs, -1),
              _M_pFunc(_Func)
        {
            _Initialize(_NumInputs);
            register_filter(_Filter);
        }

        /// <summary>
        ///     Create an join block within the specified scheduler, and places it any schedule
        ///     group of the scheduler’s choosing.
        /// </summary>
        /// <param name="_Scheduler">
        ///     The scheduler onto which the task's message propagation will be scheduled.
        /// </param>
        /// <param name="_NumInputs">
        ///     The number of inputs this join will be allowed
        /// </param>
        join_transform(Scheduler& _PScheduler, size_t _NumInputs, _Transform_method const& _Func)
            : _M_messageArray(_NumInputs, 0),
              _M_savedMessageIdArray(_NumInputs, -1),
              _M_pFunc(_Func)
        {
            _Initialize(_NumInputs, &_PScheduler);
        }

        /// <summary>
        ///     Create an join block within the specified scheduler, and places it any schedule
        ///     group of the scheduler’s choosing.
        /// </summary>
        /// <param name="_Scheduler">
        ///     The scheduler onto which the task's message propagation will be scheduled.
        /// </param>
        /// <param name="_NumInputs">
        ///     The number of inputs this join will be allowed
        /// </param>
        /// <param name="_Filter">
        ///     A filter method placed on this join
        /// </param>
        join_transform(Scheduler& _PScheduler, size_t _NumInputs, _Transform_method const& _Func, filter_method const& _Filter)
            : _M_messageArray(_NumInputs, 0),
              _M_savedMessageIdArray(_NumInputs, -1),
              _M_pFunc(_Func)
        {
            _Initialize(_NumInputs, &_PScheduler);
            register_filter(_Filter);
        }

        /// <summary>
        ///     Create an join block within the specified schedule group.  The scheduler is implied
        ///     by the schedule group.
        /// </summary>
        /// <param name="_PScheduleGroup">
        ///     The ScheduleGroup onto which the task's message propagation will be scheduled.
        /// </param>
        /// <param name="_NumInputs">
        ///     The number of inputs this join will be allowed
        /// </param>
        join_transform(ScheduleGroup& _PScheduleGroup, size_t _NumInputs, _Transform_method const& _Func)
            : _M_messageArray(_NumInputs, 0),
              _M_savedMessageIdArray(_NumInputs, -1),
              _M_pFunc(_Func)
        {
            _Initialize(_NumInputs, NULL, &_PScheduleGroup);
        }

        /// <summary>
        ///     Create an join block within the specified schedule group.  The scheduler is implied
        ///     by the schedule group.
        /// </summary>
        /// <param name="_PScheduleGroup">
        ///     The ScheduleGroup onto which the task's message propagation will be scheduled.
        /// </param>
        /// <param name="_NumInputs">
        ///     The number of inputs this join will be allowed
        /// </param>
        /// <param name="_Filter">
        ///     A filter method placed on this join
        /// </param>
        join_transform(ScheduleGroup& _PScheduleGroup, size_t _NumInputs, _Transform_method const& _Func, filter_method const& _Filter)
            : _M_messageArray(_NumInputs, 0),
              _M_savedMessageIdArray(_NumInputs, -1),
              _M_pFunc(_Func)
        {
            _Initialize(_NumInputs, NULL, &_PScheduleGroup);
            register_filter(_Filter);
        }

        /// <summary>
        ///     Destroys a join
        /// </summary>
        ~join_transform()
        {
            // Remove all links that are targets of this join
            remove_network_links();

            delete [] _M_savedIdBuffer;
        }

    protected:
        //
        // propagator_block protected function implementations
        //

        /// <summary>
        ///     The main propagate() function for ITarget blocks.  Called by a source
        ///     block, generally within an asynchronous task to send messages to its targets.
        /// </summary>
        /// <param name="_PMessage">
        ///     The message being propagated
        /// </param>
        /// <param name="_PSource">
        ///     The source doing the propagation
        /// </param>
        /// <returns>
        ///     An indication of what the target decided to do with the message.
        /// </returns>
        /// <remarks>
        ///     It is important that calls to propagate do *not* take the same lock on the
        ///     internal structure that is used by Consume and the LWT.  Doing so could
        ///     result in a deadlock with the Consume call. 
        /// </remarks>
        message_status propagate_message(message<_Input> * _PMessage, ISource<_Input> * _PSource) 
        {
            message_status _Ret_val = accepted;

            //
            // Find the slot index of this source
            //
            size_t _Slot = 0;
            bool _Found = false;
            for (source_iterator _Iter = _M_connectedSources.begin(); *_Iter != NULL; ++_Iter)
            {
                if (*_Iter == _PSource)
                {
                    _Found = true;
                    break;
                }

                _Slot++;
            }

            if (!_Found)
            {
                // If this source was not found in the array, this is not a connected source
                // decline the message
                return declined;
            }

            _ASSERTE(_Slot < _M_messageArray.size());

            bool fIsGreedy = (_Jtype == greedy);

            if (fIsGreedy)
            {
                //
                // Greedy type joins immediately accept the message.
                //
                {
                    critical_section::scoped_lock lockHolder(_M_propagationLock);
                    if (_M_messageArray[_Slot] != NULL)
                    {
                        _M_savedMessageIdArray[_Slot] = _PMessage->msg_id();
                        _Ret_val = postponed;
                    }
                }

                if (_Ret_val != postponed)
                {
                    _M_messageArray[_Slot] = _PSource->accept(_PMessage->msg_id(), this);

                    if (_M_messageArray[_Slot] != NULL)
                    {
                        if (_InterlockedDecrementSizeT(&_M_messagesRemaining) == 0)
                        {
                            // If messages have arrived on all links, start a propagation
                            // of the current message
                            async_send(NULL);
                        }
                    }
                    else
                    {
                        _Ret_val = missed;
                    }
                }
            }
            else
            {
                //
                // Non-greedy type joins save the message ids until they have all arrived
                //

                if (_InterlockedExchange((volatile long *) &_M_savedMessageIdArray[_Slot], _PMessage->msg_id()) == -1)
                {
                    // Decrement the message remaining count if this thread is switching 
                    // the saved id from -1 to a valid value.
                    if (_InterlockedDecrementSizeT(&_M_messagesRemaining) == 0)
                    {
                        async_send(NULL);
                    }
                }

                // Always return postponed.  This message will be consumed
                // in the LWT
                _Ret_val = postponed;
            }

            return _Ret_val;
        }

        /// <summary>
        ///     Accepts an offered message by the source, transferring ownership to the caller.
        /// </summary>
        /// <param name="_MsgId">
        ///     The runtime object identity of the message.
        /// </param>
        /// <returns>
        ///     A pointer to the message that the caller now has ownership of.
        /// </returns>
        virtual message<_Output> * accept_message(runtime_object_identity _MsgId)
        {
            //
            // Peek at the head message in the message buffer.  If the Ids match
            // dequeue and transfer ownership
            //
            message<_Output> * _Msg = NULL;

            if (_M_messageBuffer.is_head(_MsgId))
            {
                _Msg = _M_messageBuffer.dequeue();
            }

            return _Msg;
        }

        /// <summary>
        ///     Reserves a message previously offered by the source.
        /// </summary>
        /// <param name="_MsgId">
        ///     The runtime object identity of the message.
        /// </param>
        /// <returns>
        ///     A bool indicating whether the reservation worked or not
        /// </returns>
        /// <remarks>
        ///     After 'reserve' is called, either 'consume' or 'release' must be called.
        /// </remarks>
        virtual bool reserve_message(runtime_object_identity _MsgId)
        {
            // Allow reservation if this is the head message
            return _M_messageBuffer.is_head(_MsgId);
        }

        /// <summary>
        ///     Consumes a message previously offered by the source and reserved by the target, 
        ///     transferring ownership to the caller.
        /// </summary>
        /// <param name="_MsgId">
        ///     The runtime object identity of the message.
        /// </param>
        /// <returns>
        ///     A pointer to the message that the caller now has ownership of.
        /// </returns>
        /// <remarks>
        ///     Similar to 'accept', but is always preceded by a call to 'reserve'
        /// </remarks>
        virtual message<_Output> * consume_message(runtime_object_identity _MsgId)
        {
            // By default, accept the message
            return accept_message(_MsgId);
        }

        /// <summary>
        ///     Releases a previous message reservation.
        /// </summary>
        /// <param name="_MsgId">
        ///     The runtime object identity of the message.
        /// </param>
        virtual void release_message(runtime_object_identity _MsgId)
        {
            // The head message is the one reserved.
            if (!_M_messageBuffer.is_head(_MsgId))
            {
                throw message_not_found();
            }
        }

        /// <summary>
        ///     Resumes propagation after a reservation has been released
        /// </summary>
        virtual void resume_propagation()
        {
            // If there are any messages in the buffer, propagate them out
            if (_M_messageBuffer.count() > 0)
            {
                async_send(NULL);
            }
        }

        /// <summary>
        ///     Notification that a target was linked to this source.
        /// </summary>
        /// <param name="_PTarget">
        ///     A pointer to the newly linked target.
        /// </param>
        virtual void link_target_notification(ITarget<_Output> *)
        {
            // If the message queue is blocked due to reservation
            // there is no need to do any message propagation
            if (_M_pReservedFor != NULL)
            {
                return;
            }

            _Propagate_priority_order(_M_messageBuffer);
        }

        /// <summary>
        ///     Takes the message and propagates it to all the target of this join.
        ///     This is called from async_send.
        /// </summary>
        /// <param name="_PMessage">
        ///     The message being propagated
        /// </param>
        void propagate_to_any_targets(message<_Output> *) 
        {
            message<_Output> * _Msg = NULL;
            // Create a new message from the input sources
            // If messagesRemaining == 0, we have a new message to create.  Otherwise, this is coming from
            // a consume or release from the target.  In that case we don't want to create a new message.
            if (_M_messagesRemaining == 0)
            {
                // A greedy join can immediately create the message, a non-greedy
                // join must try and consume all the messages it has postponed
                _Msg = _Create_new_message();
            }

            if (_Msg == NULL)
            {
                // Create message failed.  This happens in non_greedy joins when the
                // reserve/consumption of a postponed message failed.
                _Propagate_priority_order(_M_messageBuffer);
                return;
            }

            bool fIsGreedy = (_Jtype == greedy);

            // For a greedy join, reset the number of messages remaining
            // Check to see if multiple messages have been passed in on any of the links,
            // and postponed. If so, try and reserve/consume them now
            if (fIsGreedy)
            {
                // Look at the saved ids and reserve/consume any that have passed in while
                // this join was waiting to complete
                _ASSERTE(_M_messageArray.size() == _M_savedMessageIdArray.size());

                for (size_t i = 0; i < _M_messageArray.size(); i++)
                {
                    for(;;)
                    {
                        runtime_object_identity _Saved_id;
                        // Grab the current saved id value.  This value could be changing from based on any
                        // calls of source->propagate(this).  If the message id is different than what is snapped
                        // here, that means, the reserve below must fail.  This is because reserve is trying
                        // to get the same source lock the propagate(this) call must be holding.
                        {
                            critical_section::scoped_lock lockHolder(_M_propagationLock);

                            _ASSERTE(_M_messageArray[i] != NULL);

                            _Saved_id = _M_savedMessageIdArray[i];

                            if (_Saved_id == -1)
                            {
                                _M_messageArray[i] = NULL;
                                break;
                            }
                            else
                            {
                                _M_savedMessageIdArray[i] = -1;
                            }
                        }

                        if (_Saved_id != -1)
                        {
                            source_iterator _Iter = _M_connectedSources.begin();
                            
                            ISource<_Input> * _PSource = _Iter[i];
                            if ((_PSource != NULL) && _PSource->reserve(_Saved_id, this))
                            {
                                _M_messageArray[i] = _PSource->consume(_Saved_id, this);
                                _InterlockedDecrementSizeT(&_M_messagesRemaining);
                                break;
                            }
                        }
                    }
                }

                // If messages have all been received, async_send again, this will start the
                // LWT up to create a new message
                if (_M_messagesRemaining == 0)
                {
                    async_send(NULL);
                }
            }
            
            // Add the new message to the outbound queue
            _M_messageBuffer.enqueue(_Msg);

            if (!_M_messageBuffer.is_head(_Msg->msg_id()))
            {
                // another message is at the head of the outbound message queue and blocked
                // simply return
                return;
            }

            _Propagate_priority_order(_M_messageBuffer);
        }

    private:

        //
        //  Private Methods
        //

        /// <summary>
        ///     Propagate messages in priority order
        /// </summary>
        /// <param name="_MessageBuffer">
        ///     Reference to a message queue with messages to be propagated
        /// </param>
        void _Propagate_priority_order(MessageQueue<_Output> & _MessageBuffer)
        {
            message<_Output> * _Msg = _MessageBuffer.peek();

            // If someone has reserved the _Head message, don't propagate anymore
            if (_M_pReservedFor != NULL)
            {
                return;
            }

            while (_Msg != NULL)
            {
                message_status _Status = declined;

                // Always start from the first target that linked
                for (target_iterator _Iter = _M_connectedTargets.begin(); *_Iter != NULL; ++_Iter)
                {
                    ITarget<_Output> * _PTarget = *_Iter;
                    _Status = _PTarget->propagate(_Msg, this);

                    // Ownership of message changed. Do not propagate this
                    // message to any other target.
                    if (_Status == accepted)
                    {
                        break;
                    }

                    // If the target just propagated to reserved this message, stop
                    // propagating it to others
                    if (_M_pReservedFor != NULL)
                    {
                        break;
                    }
                }

                // If status is anything other than accepted, then the head message
                // was not propagated out.  Thus, nothing after it in the queue can
                // be propagated out.  Cease propagation.
                if (_Status != accepted)
                {
                    break;
                }

                // Get the next message
                _Msg = _MessageBuffer.peek();
            }
        }

        /// <summary>
        ///     Create a new message from the data output
        /// </summary>
        /// <returns>
        ///     The created message (NULL if creation failed)
        /// </returns>
        message<_Output> * __cdecl _Create_new_message()
        {
            bool fIsNonGreedy = (_Jtype == non_greedy);

            // If this is a non-greedy join, check each source and try to consume their message
            if (fIsNonGreedy)
            {

                // The iterator _Iter below will ensure that it is safe to touch
                // non-NULL source pointers. Take a snapshot.
                std::vector<ISource<_Input> *> _Sources;
                source_iterator _Iter = _M_connectedSources.begin();

                while (*_Iter != NULL)
                {
                    ISource<_Input> * _PSource = *_Iter;

                    if (_PSource == NULL)
                    {
                        break;
                    }

                    _Sources.push_back(_PSource);
                    ++_Iter;
                }

                if (_Sources.size() != _M_messageArray.size())
                {
                    // Some of the sources were unlinked. The join is broken
                    return NULL;
                }

                // First, try and reserve all the messages.  If a reservation fails,
                // then release any reservations that had been made.
                for (size_t i = 0; i < _M_savedMessageIdArray.size(); i++)
                {
                    // Snap the current saved id into a buffer.  This value can be changing behind the scenes from
                    // other source->propagate(msg, this) calls, but if so, that just means the reserve below will
                    // fail.
                    _InterlockedIncrementSizeT(&_M_messagesRemaining);
                    _M_savedIdBuffer[i] = _InterlockedExchange((volatile long *) &_M_savedMessageIdArray[i], -1);

                    _ASSERTE(_M_savedIdBuffer[i] != -1);

                    if (!_Sources[i]->reserve(_M_savedIdBuffer[i], this))
                    {
                        // A reservation failed, release all reservations made up until
                        // this block, and wait for another message to arrive on this link
                        for (size_t j = 0; j < i; j++)
                        {
                            _Sources[j]->release(_M_savedIdBuffer[j], this);
                            if (_InterlockedCompareExchange((volatile long *) &_M_savedMessageIdArray[j], _M_savedIdBuffer[j], -1) == -1)
                            {
                                if (_InterlockedDecrementSizeT(&_M_messagesRemaining) == 0)
                                {
                                    async_send(NULL);
                                }
                            }
                        }

                        // Return NULL to indicate that the create failed
                        return NULL;
                    }  
                }

                // Since everything has been reserved, consume all the messages.
                // This is guaranteed to return true.
                for (size_t i = 0; i < _M_messageArray.size(); i++)
                {
                    _M_messageArray[i] = _Sources[i]->consume(_M_savedIdBuffer[i], this);
                    _M_savedIdBuffer[i] = -1;
                }
            }

            if (!fIsNonGreedy)
            {
                // Reinitialize how many messages are being waited for.
                // This is safe because all messages have been received, thus no new async_sends for
                // greedy joins can be called.
                _M_messagesRemaining = _M_messageArray.size();
            }

            std::vector<_Input> _OutputVector;
            for (size_t i = 0; i < _M_messageArray.size(); i++)
            {
                _ASSERTE(_M_messageArray[i] != NULL);
                _OutputVector.push_back(_M_messageArray[i]->payload);

                delete _M_messageArray[i];
                if (fIsNonGreedy)
                {
                    _M_messageArray[i] = NULL;
                }
            }

            _Output _Out = _M_pFunc(_OutputVector);

            return (new message<_Output>(_Out));
        }

        /// <summary>
        ///     Initialize the join block
        /// </summary>
        /// <param name="_NumInputs">
        ///     The number of inputs
        /// </param>
        void _Initialize(size_t _NumInputs, Scheduler * _PScheduler = NULL, ScheduleGroup * _PScheduleGroup = NULL)
        {
            initialize_source_and_target(_PScheduler, _PScheduleGroup);

            _M_connectedSources.set_bound(_NumInputs);
            _M_messagesRemaining = _NumInputs;

            bool fIsNonGreedy = (_Jtype == non_greedy);

            if (fIsNonGreedy)
            {
                // Non greedy joins need a buffer to snap off saved message ids to.
                _M_savedIdBuffer = new runtime_object_identity[_NumInputs];
                memset(_M_savedIdBuffer, -1, sizeof(runtime_object_identity) * _NumInputs);
            }
            else
            {
                _M_savedIdBuffer = NULL;
            }
        }

        // The current number of messages remaining
        volatile size_t _M_messagesRemaining;

        // An array containing the accepted messages of this join.
        std::vector<message<_Input>*> _M_messageArray;

        // An array containing the msg ids of messages propagated to the array
        // For greedy joins, this contains a log of other messages passed to this
        // join after the first has been accepted
        // For non-greedy joins, this contains the message id of any message 
        // passed to it.
        std::vector<runtime_object_identity> _M_savedMessageIdArray;

        // The transformer method called by this block
        _Transform_method _M_pFunc;

        // Buffer for snapping saved ids in non-greedy joins
        runtime_object_identity * _M_savedIdBuffer;

        // A lock for modifying the buffer or the connected blocks
        ::Concurrency::critical_section _M_propagationLock;

        // Queue to hold output messages
        MessageQueue<_Output> _M_messageBuffer;
    };

    //
    // Message block that invokes a transform method when it receives message on any of the input links.
    // A typical example is recal engine for a cell in an Excel spreadsheet.
    // (Remember that a normal join block is triggered only when it receives messages on all its input links).
    //
    template<class _Input, class _Output>
    class recalculate : public propagator_block<single_link_registry<ITarget<_Output>>, multi_link_registry<ISource<_Input>>>
    {
    public:
        typedef std::tr1::function<_Output(std::vector<_Input> const&)> _Recalculate_method;

        /// <summary>
        ///     Create an recalculate block within the default scheduler, and places it any schedule
        ///     group of the scheduler’s choosing.
        /// </summary>
        /// <param name="_NumInputs">
        ///     The number of inputs 
        /// </param>
        recalculate(size_t _NumInputs, _Recalculate_method const& _Func)
            : _M_messageArray(_NumInputs, 0), _M_savedMessageIdArray(_NumInputs, -1),
              _M_pFunc(_Func)
        {
            _Initialize(_NumInputs);
        }

        /// <summary>
        ///     Create an recalculate block within the default scheduler, and places it any schedule
        ///     group of the scheduler’s choosing.
        /// </summary>
        /// <param name="_NumInputs">
        ///     The number of inputs 
        /// </param>
        /// <param name="_Filter">
        ///     A filter method placed on this join
        /// </param>
        recalculate(size_t _NumInputs, _Recalculate_method const& _Func, filter_method const& _Filter)
            : _M_messageArray(_NumInputs, 0), _M_savedMessageIdArray(_NumInputs, -1),
              _M_pFunc(_Func)
        {
            _Initialize(_NumInputs);
            register_filter(_Filter);
        }

        /// <summary>
        ///     Create an recalculate block within the specified scheduler, and places it any schedule
        ///     group of the scheduler’s choosing.
        /// </summary>
        /// <param name="_Scheduler">
        ///     The scheduler onto which the task's message propagation will be scheduled.
        /// </param>
        /// <param name="_NumInputs">
        ///     The number of inputs 
        /// </param>
        recalculate(size_t _NumInputs, Scheduler& _PScheduler, _Recalculate_method const& _Func)
            : _M_messageArray(_NumInputs, 0), _M_savedMessageIdArray(_NumInputs, -1),
              _M_pFunc(_Func)
        {
            _Initialize(_NumInputs, &_PScheduler);
        }

        /// <summary>
        ///     Create an recalculate block within the specified scheduler, and places it any schedule
        ///     group of the scheduler’s choosing.
        /// </summary>
        /// <param name="_Scheduler">
        ///     The scheduler onto which the task's message propagation will be scheduled.
        /// </param>
        /// <param name="_NumInputs">
        ///     The number of inputs 
        /// </param>
        /// <param name="_Filter">
        ///     A filter method placed on this join
        /// </param>
        recalculate(size_t _NumInputs, Scheduler& _PScheduler, _Recalculate_method const& _Func, filter_method const& _Filter)
            : _M_messageArray(_NumInputs, 0), _M_savedMessageIdArray(_NumInputs, -1),
              _M_pFunc(_Func)
        {
            _Initialize(_NumInputs, &_PScheduler);
            register_filter(_Filter);
        }

        /// <summary>
        ///     Create an recalculate block within the specified schedule group.  The scheduler is implied
        ///     by the schedule group.
        /// </summary>
        /// <param name="_PScheduleGroup">
        ///     The ScheduleGroup onto which the task's message propagation will be scheduled.
        /// </param>
        /// <param name="_NumInputs">
        ///     The number of inputs 
        /// </param>
        recalculate(size_t _NumInputs, ScheduleGroup& _PScheduleGroup, _Recalculate_method const& _Func)
            : _M_messageArray(_NumInputs, 0), _M_savedMessageIdArray(_NumInputs, -1),
              _M_pFunc(_Func)
        {
            _Initialize(_NumInputs, NULL, &_PScheduleGroup);
        }

        /// <summary>
        ///     Create an recalculate block within the specified schedule group.  The scheduler is implied
        ///     by the schedule group.
        /// </summary>
        /// <param name="_PScheduleGroup">
        ///     The ScheduleGroup onto which the task's message propagation will be scheduled.
        /// </param>
        /// <param name="_NumInputs">
        ///     The number of inputs 
        /// </param>
        /// <param name="_Filter">
        ///     A filter method placed on this join
        /// </param>
        recalculate(size_t _NumInputs, ScheduleGroup& _PScheduleGroup, _Recalculate_method const& _Func, filter_method const& _Filter)
            : _M_messageArray(_NumInputs, 0), _M_savedMessageIdArray(_NumInputs, -1),
              _M_pFunc(_Func)
        {
            _Initialize(_NumInputs, NULL, &_PScheduleGroup);
            register_filter(_Filter);
        }

        /// <summary>
        ///     Destroys a join
        /// </summary>
        ~recalculate()
        {
            // Remove all links that are targets of this join
            remove_network_links();

            delete [] _M_savedIdBuffer;
        }

    protected:
        //
        // propagator_block protected function implementations
        //

        /// <summary>
        ///     The main propagate() function for ITarget blocks.  Called by a source
        ///     block, generally within an asynchronous task to send messages to its targets.
        /// </summary>
        /// <param name="_PMessage">
        ///     The message being propagated
        /// </param>
        /// <param name="_PSource">
        ///     The source doing the propagation
        /// </param>
        /// <returns>
        ///     An indication of what the target decided to do with the message.
        /// </returns>
        /// <remarks>
        ///     It is important that calls to propagate do *not* take the same lock on the
        ///     internal structure that is used by Consume and the LWT.  Doing so could
        ///     result in a deadlock with the Consume call. 
        /// </remarks>
        message_status propagate_message(message<_Input> * _PMessage, ISource<_Input> * _PSource) 
        {
            //
            // Find the slot index of this source
            //
            size_t _Slot = 0;
            bool _Found = false;
            for (source_iterator _Iter = _M_connectedSources.begin(); *_Iter != NULL; ++_Iter)
            {
                if (*_Iter == _PSource)
                {
                    _Found = true;
                    break;
                }

                _Slot++;
            }

            if (!_Found)
            {
                // If this source was not found in the array, this is not a connected source
                // decline the message
                return declined;
            }

            //
            // Save the message id
            //
            if (_InterlockedExchange((volatile long *) &_M_savedMessageIdArray[_Slot], _PMessage->msg_id()) == -1)
            {
                // If it is not seen by Create_message attempt a recalculate
                async_send(NULL);
            }

            // Always return postponed.  This message will be consumed
            // in the LWT
            return postponed;
        }

        /// <summary>
        ///     Accepts an offered message by the source, transferring ownership to the caller.
        /// </summary>
        /// <param name="_MsgId">
        ///     The runtime object identity of the message.
        /// </param>
        /// <returns>
        ///     A pointer to the message that the caller now has ownership of.
        /// </returns>
        virtual message<_Output> * accept_message(runtime_object_identity _MsgId)
        {
            //
            // Peek at the head message in the message buffer.  If the Ids match
            // dequeue and transfer ownership
            //
            message<_Output> * _Msg = NULL;

            if (_M_messageBuffer.is_head(_MsgId))
            {
                _Msg = _M_messageBuffer.dequeue();
            }

            return _Msg;
        }

        /// <summary>
        ///     Reserves a message previously offered by the source.
        /// </summary>
        /// <param name="_MsgId">
        ///     The runtime object identity of the message.
        /// </param>
        /// <returns>
        ///     A bool indicating whether the reservation worked or not
        /// </returns>
        /// <remarks>
        ///     After 'reserve' is called, either 'consume' or 'release' must be called.
        /// </remarks>
        virtual bool reserve_message(runtime_object_identity _MsgId)
        {
            // Allow reservation if this is the head message
            return _M_messageBuffer.is_head(_MsgId);
        }

        /// <summary>
        ///     Consumes a message previously offered by the source and reserved by the target, 
        ///     transferring ownership to the caller.
        /// </summary>
        /// <param name="_MsgId">
        ///     The runtime object identity of the message.
        /// </param>
        /// <returns>
        ///     A pointer to the message that the caller now has ownership of.
        /// </returns>
        /// <remarks>
        ///     Similar to 'accept', but is always preceded by a call to 'reserve'
        /// </remarks>
        virtual message<_Output> * consume_message(runtime_object_identity _MsgId)
        {
            // By default, accept the message
            return accept_message(_MsgId);
        }

        /// <summary>
        ///     Releases a previous message reservation.
        /// </summary>
        /// <param name="_MsgId">
        ///     The runtime object identity of the message.
        /// </param>
        virtual void release_message(runtime_object_identity _MsgId)
        {
            // The head message is the one reserved.
            if (!_M_messageBuffer.is_head(_MsgId))
            {
                throw message_not_found();
            }
        }

        /// <summary>
        ///     Resumes propagation after a reservation has been released
        /// </summary>
        virtual void resume_propagation()
        {
            // If there are any messages in the buffer, propagate them out
            if (_M_messageBuffer.count() > 0)
            {
                async_send(NULL);
            }
        }

        /// <summary>
        ///     Notification that a target was linked to this source.
        /// </summary>
        /// <param name="_PTarget">
        ///     A pointer to the newly linked target.
        /// </param>
        virtual void link_target_notification(ITarget<_Output> *)
        {
            // If the message queue is blocked due to reservation
            // there is no need to do any message propagation
            if (_M_pReservedFor != NULL)
            {
                return;
            }

            _Propagate_priority_order(_M_messageBuffer);
        }

        /// <summary>
        ///     Takes the message and propagates it to all the target of this join.
        ///     This is called from async_send.
        /// </summary>
        /// <param name="_PMessage">
        ///     The message being propagated
        /// </param>
        void propagate_to_any_targets(message<_Output> *) 
        {
            // Attempt to create a new message
            message<_Output> * _Msg = _Create_new_message();

            if (_Msg != NULL)
            {
                // Add the new message to the outbound queue
                _M_messageBuffer.enqueue(_Msg);

                if (!_M_messageBuffer.is_head(_Msg->msg_id()))
                {
                    // another message is at the head of the outbound message queue and blocked
                    // simply return
                    return;
                }
            }

            _Propagate_priority_order(_M_messageBuffer);
        }

    private:

        //
        //  Private Methods
        //

        /// <summary>
        ///     Propagate messages in priority order
        /// </summary>
        /// <param name="_MessageBuffer">
        ///     Reference to a message queue with messages to be propagated
        /// </param>
        void _Propagate_priority_order(MessageQueue<_Output> & _MessageBuffer)
        {
            message<_Output> * _Msg = _MessageBuffer.peek();

            // If someone has reserved the _Head message, don't propagate anymore
            if (_M_pReservedFor != NULL)
            {
                return;
            }

            while (_Msg != NULL)
            {
                message_status _Status = declined;

                // Always start from the first target that linked
                for (target_iterator _Iter = _M_connectedTargets.begin(); *_Iter != NULL; ++_Iter)
                {
                    ITarget<_Output> * _PTarget = *_Iter;
                    _Status = _PTarget->propagate(_Msg, this);

                    // Ownership of message changed. Do not propagate this
                    // message to any other target.
                    if (_Status == accepted)
                    {
                        break;
                    }

                    // If the target just propagated to reserved this message, stop
                    // propagating it to others
                    if (_M_pReservedFor != NULL)
                    {
                        break;
                    }
                }

                // If status is anything other than accepted, then the head message
                // was not propagated out.  Thus, nothing after it in the queue can
                // be propagated out.  Cease propagation.
                if (_Status != accepted)
                {
                    break;
                }

                // Get the next message
                _Msg = _MessageBuffer.peek();
            }
        }

        /// <summary>
        ///     Create a new message from the data output
        /// </summary>
        /// <returns>
        ///     The created message (NULL if creation failed)
        /// </returns>
        message<_Output> * __cdecl _Create_new_message()
        {
            // If this is a non-greedy join, check each source and try to consume their message
            size_t _NumInputs = _M_savedMessageIdArray.size();

            // The iterator _Iter below will ensure that it is safe to touch
            // non-NULL source pointers. Take a snapshot.
            std::vector<ISource<_Input> *> _Sources;
            source_iterator _Iter = _M_connectedSources.begin();

            while (*_Iter != NULL)
            {
                ISource<_Input> * _PSource = *_Iter;

                if (_PSource == NULL)
                {
                    break;
                }

                _Sources.push_back(_PSource);
                ++_Iter;
            }

            if (_Sources.size() != _NumInputs)
            {
                // Some of the sources were unlinked. The join is broken
                return NULL;
            }

            // First, try and reserve all the messages.  If a reservation fails,
            // then release any reservations that had been made.
            for (size_t i = 0; i < _M_savedMessageIdArray.size(); i++)
            {
                // Swap the id to -1 indicating that we have used that value for a recalculate
                _M_savedIdBuffer[i] = _InterlockedExchange((volatile long *) &_M_savedMessageIdArray[i], -1);

                // If the id is -1, either we have never received a message on that link or the previous message is stored
                // in the message array. If it is the former we abort. 
                // If the id is not -1, we attempt to reserve the message. On failure we abort.
                if (((_M_savedIdBuffer[i] == -1) && (_M_messageArray[i] == NULL))
                    || ((_M_savedIdBuffer[i] != -1) && !_Sources[i]->reserve(_M_savedIdBuffer[i], this)))
                {
                    // Abort. Release all reservations made up until this block, 
                    // and wait for another message to arrive.
                    for (size_t j = 0; j < i; j++)
                    {
                        if (_M_savedIdBuffer[j] != -1)
                        {
                            _Sources[j]->release(_M_savedIdBuffer[j], this);
                            _InterlockedCompareExchange((volatile long *) &_M_savedMessageIdArray[j], _M_savedIdBuffer[j], -1);
                        }
                    }

                    // Return NULL to indicate that the create failed
                    return NULL;
                }  
            }

            // Since everything has been reserved, consume all the messages.
            // This is guaranteed to return true.
            size_t _NewMessages = 0;
            for (size_t i = 0; i < _NumInputs; i++)
            {
                if (_M_savedIdBuffer[i] != -1)
                {
                    // Delete previous message since we have a new one
                    if (_M_messageArray[i] != NULL)
                    {
                        delete _M_messageArray[i];
                    }
                    _M_messageArray[i] = _Sources[i]->consume(_M_savedIdBuffer[i], this);
                    _M_savedIdBuffer[i] = -1;
                    _NewMessages++;
                }
            }

            if (_NewMessages == 0)
            {
                // There is no need to recal if we did not consume a new message
                return NULL;
            }

            std::vector<_Input> _OutputVector;
            for (size_t i = 0; i < _NumInputs; i++)
            {
                _ASSERTE(_M_messageArray[i] != NULL);
                _OutputVector.push_back(_M_messageArray[i]->payload);
            }

            _Output _Out = _M_pFunc(_OutputVector);
            return (new message<_Output>(_Out));
        }

        /// <summary>
        ///     Initialize the recalculate block
        /// </summary>
        /// <param name="_NumInputs">
        ///     The number of inputs
        /// </param>
        void _Initialize(size_t _NumInputs, Scheduler * _PScheduler = NULL, ScheduleGroup * _PScheduleGroup = NULL)
        {
            initialize_source_and_target(_PScheduler, _PScheduleGroup);

            _M_connectedSources.set_bound(_NumInputs);

            // Non greedy joins need a buffer to snap off saved message ids to.
            _M_savedIdBuffer = new runtime_object_identity[_NumInputs];
            memset(_M_savedIdBuffer, -1, sizeof(runtime_object_identity) * _NumInputs);
        }

        // An array containing the accepted messages of this join.
        std::vector<message<_Input>*> _M_messageArray;

        // An array containing the msg ids of messages propagated to the array
        // For non-greedy joins, this contains the message id of any message 
        // passed to it.
        std::vector<runtime_object_identity> _M_savedMessageIdArray;

        // Buffer for snapping saved ids in non-greedy joins
        runtime_object_identity * _M_savedIdBuffer;

        // The transformer method called by this block
        _Recalculate_method _M_pFunc;

        // Queue to hold output messages
        MessageQueue<_Output> _M_messageBuffer;
    };

    //
    // Container class to hold a join_transform block and keep unbounded buffers in front of each input.
    //
    template<class _Input, class _Output>
    class buffered_join
    {
        typedef std::tr1::function<_Output(std::vector<_Input> const&)> _Transform_method;

    public:
        buffered_join(int _NumInputs, _Transform_method const& _Func): m_currentInput(0), m_numInputs(_NumInputs)
        {
            m_buffers = new unbounded_buffer<_Input>*[_NumInputs];
            m_join = new join_transform<_Input,_Output,greedy>(_NumInputs, _Func);

            for(int i = 0; i < _NumInputs; i++)
            {
                m_buffers[i] = new unbounded_buffer<_Input>;
                m_buffers[i]->link_target(m_join);
            }
        }

        ~buffered_join()
        {
            for(int i = 0; i < m_numInputs; i++)
                delete m_buffers[i];
            delete [] m_buffers;
            delete m_join;
        }

        // Add input takes _PSource and connects it to the next input on this block
        void add_input(ISource<_Input> * _PSource)
        {
            _PSource->link_target(m_buffers[m_currentInput]);
            m_currentInput++;
        }

        // link_target links this container block to _PTarget
        void link_target(ITarget<_Output> * _PTarget)
        {
            m_join->link_target(_PTarget);
        }
    private:

        int m_currentInput;
        int m_numInputs;
        unbounded_buffer<_Input> ** m_buffers;
        join_transform<_Input,_Output,greedy> * m_join;
    };
} // namespace Concurrency
