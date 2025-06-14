/*
 * Copyright (c) 2025 Jacob Lifshay <programmerjake@gmail.com>
 *
 * This file is part of CasparCG (www.casparcg.com).
 *
 * CasparCG is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CasparCG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

// use boost's types to support thread interruption
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <queue>
#include <string>

#ifdef CASPARCG_DEBUG_CHANNEL
#include <sstream>

#include "common/log.h"
#endif

namespace caspar {
template <typename T>
class Sender;

template <typename T>
class Receiver;

struct channel_capacity_is_hint_t final
{
};

inline constexpr channel_capacity_is_hint_t channel_capacity_is_hint{};

template <typename T>
std::pair<Sender<T>, Receiver<T>> channel(std::size_t capacity, std::string debug_name = {});

template <typename T>
std::pair<Sender<T>, Receiver<T>>
channel(std::size_t capacity, channel_capacity_is_hint_t, std::string debug_name = {});

template <typename T>
class Receiver final
{
    template <typename>
    friend class Sender;

    template <typename T2>
    friend std::pair<Sender<T2>, Receiver<T2>> channel(std::size_t capacity, std::string debug_name);

    template <typename T2>
    friend std::pair<Sender<T2>, Receiver<T2>>
    channel(std::size_t capacity, channel_capacity_is_hint_t, std::string debug_name);

  public:
    Receiver() noexcept
        : state_()
    {
    }
    Receiver(const Receiver&)                = delete;
    Receiver(Receiver&&) noexcept            = default;
    Receiver& operator=(const Receiver&)     = delete;
    Receiver& operator=(Receiver&&) noexcept = default;

    bool closed() const
    {
        if (!state_)
            return true;
        auto lock = boost::unique_lock(state_->lock_);
        return state_->queue_.empty() && state_->sender_destroyed_;
    }
    bool empty() const
    {
        if (!state_)
            return true;
        auto lock = boost::unique_lock(state_->lock_);
        return state_->queue_.empty();
    }
    std::size_t capacity() const
    {
        if (!state_)
            return 0;
        return state_->capacity_;
    }

    struct TryReceiveResult final
    {
        std::optional<T> value;
        bool             closed;
    };
    TryReceiveResult try_receive()
    {
        if (!state_)
            return {std::nullopt, true};
        auto lock = boost::unique_lock(state_->lock_);
        return state_->try_receive_locked();
    }
    /// not named `receive` since that would imply throwing an exception when closed
    std::optional<T> try_receive_blocking()
    {
        if (!state_)
            return std::nullopt;
        auto lock = boost::unique_lock(state_->lock_);

        TryReceiveResult try_receive_result{std::nullopt, true};
        state_->cond_.wait(lock, [&]() {
            try_receive_result = state_->try_receive_locked();
            return try_receive_result.value || try_receive_result.closed;
        });
        return std::move(try_receive_result.value);
    }

  private:
    struct TrySendResult final
    {
        std::optional<T> unsent;
        bool             closed;
    };
    struct State final
    {
        // use boost's types to support thread interruption
        boost::mutex              lock_;
        boost::condition_variable cond_;
        std::queue<T>             queue_;
        const std::size_t         capacity_;
        const bool                capacity_is_hint_;
#ifdef CASPARCG_DEBUG_CHANNEL
        const std::string debug_name_;
#endif
        bool sender_destroyed_   = false;
        bool receiver_destroyed_ = false;

        TryReceiveResult try_receive_locked()
        {
            if (queue_.empty()) {
#ifdef CASPARCG_DEBUG_CHANNEL
                CASPAR_LOG(trace) << "channel state: " << debug_name_
                                  << ": try_receive_locked: empty, sender_destroyed_ == " << sender_destroyed_;
#endif
                return {std::nullopt, sender_destroyed_};
            }
            TryReceiveResult retval{std::move(queue_.front()), false};
            queue_.pop();
            cond_.notify_all();
#ifdef CASPARCG_DEBUG_CHANNEL
            CASPAR_LOG(trace) << "channel state: " << debug_name_ << ": try_receive_locked: received value";
#endif
            return retval;
        }

        TrySendResult try_send_locked(T&& v)
        {
            if ((queue_.size() >= capacity_ && !capacity_is_hint_) || receiver_destroyed_) {
#ifdef CASPARCG_DEBUG_CHANNEL
                CASPAR_LOG(trace) << "channel state: " << debug_name_
                                  << ": try_send_locked: full, receiver_destroyed_ == " << receiver_destroyed_;
#endif
                return {std::move(v), receiver_destroyed_};
            }
            queue_.push(std::move(v));
            cond_.notify_all();
#ifdef CASPARCG_DEBUG_CHANNEL
            CASPAR_LOG(trace) << "channel state: " << debug_name_ << ": try_send_locked: sent value";
#endif
            return {std::nullopt, false};
        }

#ifdef CASPARCG_DEBUG_CHANNEL
        static std::string make_default_debug_name(const void* p)
        {
            std::ostringstream os;
            os << p;
            return std::move(os).str();
        }
#endif

        explicit State(std::size_t capacity, bool capacity_is_hint, std::string debug_name)
            : capacity_(capacity)
            , capacity_is_hint_(capacity_is_hint)
#ifdef CASPARCG_DEBUG_CHANNEL
            , debug_name_(debug_name.empty() ? make_default_debug_name(this) : std::move(debug_name))
#endif
        {
        }
        template <bool is_sender>
        struct Deleter final
        {
            void operator()(State* state) noexcept
            {
                if (!state)
                    return;
                {
                    auto lock = boost::unique_lock(state->lock_);
                    if (is_sender) {
#ifdef CASPARCG_DEBUG_CHANNEL
                        CASPAR_LOG(trace) << "channel state: " << state->debug_name_ << ": destroy sender";
#endif
                        state->sender_destroyed_ = true;
                    } else {
#ifdef CASPARCG_DEBUG_CHANNEL
                        CASPAR_LOG(trace) << "channel state: " << state->debug_name_ << ": destroy receiver";
#endif
                        state->receiver_destroyed_ = true;
                    }
                    if (!state->sender_destroyed_ || !state->receiver_destroyed_) {
                        state->cond_.notify_all();
                        return;
                    }
                }
                delete state;
            }
        };
    };
    using Deleter = typename State::template Deleter<false>;
    explicit Receiver(std::unique_ptr<State, Deleter> state) noexcept
        : state_(std::move(state))
    {
    }
    std::unique_ptr<State, Deleter> state_;
};

template <typename T>
class Sender final
{
    template <typename T2>
    friend std::pair<Sender<T2>, Receiver<T2>> channel(std::size_t capacity, std::string debug_name);

    template <typename T2>
    friend std::pair<Sender<T2>, Receiver<T2>>
    channel(std::size_t capacity, channel_capacity_is_hint_t, std::string debug_name);

  public:
    Sender() noexcept
        : state_()
    {
    }
    Sender(const Sender&)                = delete;
    Sender(Sender&&) noexcept            = default;
    Sender& operator=(const Sender&)     = delete;
    Sender& operator=(Sender&&) noexcept = default;

    using TrySendResult = typename Receiver<T>::TrySendResult;

    bool closed() const
    {
        if (!state_)
            return true;
        auto lock = boost::unique_lock(state_->lock_);
        return state_->receiver_destroyed_;
    }
    bool full() const
    {
        if (!state_)
            return true;
        auto lock = boost::unique_lock(state_->lock_);
        return state_->receiver_destroyed_ || state_->queue_.size() >= state_->capacity_;
    }
    std::size_t capacity() const
    {
        if (!state_)
            return 0;
        return state_->capacity_;
    }
    TrySendResult try_send(T v)
    {
        if (!state_)
            return {std::move(v), true};
        auto lock = boost::unique_lock(state_->lock_);
        return state_->try_send_locked(std::move(v));
    }
    /// not named `send` since that would imply throwing an exception when closed.
    /// returns the passed-in value if closed, otherwise returns nullopt
    std::optional<T> try_send_blocking(T v)
    {
        if (!state_)
            return std::move(v);
        auto lock = boost::unique_lock(state_->lock_);

        TrySendResult try_send_result{std::move(v), false};
        state_->cond_.wait(lock, [&]() {
            try_send_result = state_->try_send_locked(std::move(*try_send_result.unsent));
            return try_send_result.closed || !try_send_result.unsent;
        });
        return std::move(try_send_result.unsent);
    }

  private:
    using State   = typename Receiver<T>::State;
    using Deleter = typename State::template Deleter<true>;
    explicit Sender(std::unique_ptr<State, Deleter> state) noexcept
        : state_(std::move(state))
    {
    }
    std::unique_ptr<State, Deleter> state_;
};

template <typename T>
std::pair<Sender<T>, Receiver<T>> channel(std::size_t capacity, std::string debug_name)
{
    using State = typename Receiver<T>::State;
    auto state  = new State(capacity, false, std::move(debug_name));
    return {Sender<T>(std::unique_ptr<State, typename Sender<T>::Deleter>(state)),
            Receiver<T>(std::unique_ptr<State, typename Receiver<T>::Deleter>(state))};
}

template <typename T>
std::pair<Sender<T>, Receiver<T>> channel(std::size_t capacity, channel_capacity_is_hint_t, std::string debug_name)
{
    using State = typename Receiver<T>::State;
    auto state  = new State(capacity, true, std::move(debug_name));
    return {Sender<T>(std::unique_ptr<State, typename Sender<T>::Deleter>(state)),
            Receiver<T>(std::unique_ptr<State, typename Receiver<T>::Deleter>(state))};
}
} // namespace caspar