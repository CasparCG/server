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

namespace caspar {
template <typename T>
class Sender;

template <typename T>
class Receiver;

template <typename T>
std::pair<Sender<T>, Receiver<T>> channel(std::size_t capacity);

template <typename T>
class Receiver final
{
    template <typename>
    friend class Sender;

    template <typename T2>
    friend std::pair<Sender<T2>, Receiver<T2>> channel(std::size_t capacity);

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
        bool                      sender_destroyed_   = false;
        bool                      receiver_destroyed_ = false;

        TryReceiveResult try_receive_locked()
        {
            if (queue_.empty())
                return {std::nullopt, sender_destroyed_};
            TryReceiveResult retval{std::move(queue_.front()), false};
            queue_.pop();
            cond_.notify_all();
            return retval;
        }

        TrySendResult try_send_locked(T&& v)
        {
            if (queue_.size() >= capacity_ || receiver_destroyed_)
                return {std::move(v), receiver_destroyed_};
            queue_.push(std::move(v));
            cond_.notify_all();
            return {std::nullopt, false};
        }

        explicit State(std::size_t capacity)
            : capacity_(capacity)
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
                        state->sender_destroyed_ = true;
                    } else {
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
    friend std::pair<Sender<T2>, Receiver<T2>> channel(std::size_t capacity);

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
std::pair<Sender<T>, Receiver<T>> channel(std::size_t capacity)
{
    using State = typename Receiver<T>::State;
    auto state  = new State(capacity);
    return {Sender<T>(std::unique_ptr<State, typename Sender<T>::Deleter>(state)),
            Receiver<T>(std::unique_ptr<State, typename Receiver<T>::Deleter>(state))};
}
} // namespace caspar