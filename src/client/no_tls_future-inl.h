/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

//Hybris and libc can have TLS collisions in the thread using OpenGL
//between std::promise and the GL dispatch.
//https://github.com/libhybris/libhybris/issues/212
//If this bug is resolved, switch to std::future and rm this file

#ifndef MIR_CLIENT_NO_TLS_FUTURE_INL_H_
#define MIR_CLIENT_NO_TLS_FUTURE_INL_H_

#include <memory>
#include <mutex>
#include <condition_variable>
#include <future>
#include <boost/throw_exception.hpp>

namespace mir
{
namespace client
{
template<typename T>
class NoTLSFuture;

template<typename T>
class NoTLSFutureBase;

template<typename T>
class PromiseStateBase
{
public:
    void wait() const
    {
        std::unique_lock<std::mutex> lk(mutex);
        cv.wait(lk, [this]{ return set || broken; });
    }

    template<class Rep, class Period>
    std::future_status wait_for(std::chrono::duration<Rep, Period> const& timeout_duration) const
    {
        std::unique_lock<std::mutex> lk(mutex);
        if (cv.wait_for(lk, timeout_duration, [this]{ return set || broken; }))
            return std::future_status::ready;
        return std::future_status::timeout;
    }

    void break_promise()
    {
        bool run_continuation{false};
        {
            std::lock_guard<std::mutex> lk(mutex);
            if (!set)
            {
                broken = true;
                if (continuation)
                {
                    run_continuation = true;
                }
            }
        }
        if (run_continuation)
        {
            continuation();
        }
        cv.notify_all();
    }

    PromiseStateBase() = default;
    PromiseStateBase(PromiseStateBase const&) = delete;
    PromiseStateBase(PromiseStateBase&&) = delete;
    PromiseStateBase& operator=(PromiseStateBase const&) = delete;
    PromiseStateBase& operator=(PromiseStateBase &&) = delete;

protected:
    class WriteLock
    {
    public:
        WriteLock(PromiseStateBase& parent)
            : parent{parent},
              lock{parent.mutex}
        {
            if (parent.set)
            {
                BOOST_THROW_EXCEPTION(std::runtime_error{"promise_already_satisfied"});
            }
            call_continuation = static_cast<bool>(parent.continuation);
        }
        WriteLock(WriteLock&&) = default;
        WriteLock& operator=(WriteLock&&) = default;

        ~WriteLock() noexcept(false)
        {
            parent.set = true;
            lock.unlock();
            if (call_continuation)
            {
                parent.continuation();
            }
            parent.cv.notify_all();
        }
    private:
        PromiseStateBase& parent;
        std::unique_lock<std::mutex> lock;
        bool call_continuation;
    };
    class ReadLock
    {
    public:
        ReadLock(PromiseStateBase& parent)
            : lock{parent.mutex}
        {
            parent.cv.wait(lock, [&parent]{ return parent.set || parent.broken; });
            if (parent.broken)
            {
                //clang has problems with std::future_error::what() on vivid+overlay
                BOOST_THROW_EXCEPTION(std::runtime_error("broken_promise"));
            }
        }
    private:
        std::unique_lock<std::mutex> lock;
    };

    WriteLock ensure_write_context()
    {
        return WriteLock(*this);
    }
    ReadLock ensure_read_context()
    {
        return ReadLock(*this);
    }

private:
    std::mutex mutable mutex;
    std::condition_variable mutable cv;
    bool set{false};
    bool broken{false};

    class OneShotContinuation
    {
    public:
        OneShotContinuation() = default;

        OneShotContinuation(std::function<void()>&& continuation)
            : continuation{std::move(continuation)}
        {
        }
        OneShotContinuation& operator=(std::function<void()>&& continuation)
        {
            this->continuation = std::move(continuation);
            return *this;
        }

        void operator()()
        {
            continuation();
            continuation = nullptr;
        }
        operator bool() const
        {
            return static_cast<bool>(continuation);
        }
    private:
        std::function<void()> continuation;
    };
    OneShotContinuation continuation;

    friend class NoTLSFutureBase<T>;
    void set_continuation(std::function<void()>&& continuation)
    {
        std::unique_lock<std::mutex> lk{mutex};
        if (set)
        {
            lk.unlock();
            continuation();
        }
        else
        {
            this->continuation = std::move(continuation);
        }
    }
};

template<typename T>
class PromiseState : public PromiseStateBase<T>
{
public:
    void set_value(T const& val)
    {
        auto lock = PromiseStateBase<T>::ensure_write_context();
        value = val;
    }

    void set_value(T&& val)
    {
        auto lock = PromiseStateBase<T>::ensure_write_context();
        value = std::move(val);
    }

    T get_value()
    {
        auto lock = PromiseStateBase<T>::ensure_read_context();
        return value;
    }

private:
    T value;
};

template<>
class PromiseState<void> : public PromiseStateBase<void>
{
public:
    void set_value()
    {
        ensure_write_context();
    }

    void get_value()
    {
        ensure_read_context();
    }
};

template<typename T>
class NoTLSPromise;

template<typename T>
class NoTLSFutureBase
{
public:
    NoTLSFutureBase() :
        state(nullptr)
    {
    }

    NoTLSFutureBase(std::shared_ptr<PromiseState<T>> const& state) :
        state(state)
    {
    }

    ~NoTLSFutureBase()
    {
        if (state)
        {
            state->wait();
        }
    }

    NoTLSFutureBase(NoTLSFutureBase&& other) :
        state(std::move(other.state))
    {
    }

    NoTLSFutureBase& operator=(NoTLSFutureBase&& other)
    {
        state = std::move(other.state);
        return *this;
    }

    NoTLSFutureBase(NoTLSFutureBase const&) = delete;
    NoTLSFutureBase& operator=(NoTLSFutureBase const&) = delete;

    void validate_state() const
    {
        if (!valid())
            throw std::logic_error("state was not valid");
    }

    template<class Rep, class Period>
    std::future_status wait_for(std::chrono::duration<Rep, Period> const& timeout_duration) const
    {
        validate_state();
        return state->wait_for(timeout_duration);
    }

    bool valid() const
    {
        return state != nullptr;
    }


    template<typename Func>
    NoTLSFuture<typename std::result_of_t<Func(NoTLSFuture<T>&&)>> then(Func&& completion)
    {
        NoTLSPromise<typename std::result_of_t<Func(NoTLSFuture<T>&&)>> promise;
        auto transformed_future = promise.get_future();

        state->set_continuation(make_continuation_for(std::move(promise), std::move(completion)));

        state = nullptr;

        return transformed_future;
    }

private:
    template<typename Func, typename Result>
    std::function<void()> make_continuation_for(NoTLSPromise<Result>&& resultant, Func&& continuation);

    template<typename Func>
    std::function<void()> make_continuation_for(NoTLSPromise<void>&& resultant, Func&& continuation);

protected:
    std::shared_ptr<PromiseState<T>> state;
};

template<typename T>
class NoTLSFuture : public NoTLSFutureBase<T>
{
public:
    using NoTLSFutureBase<T>::NoTLSFutureBase;

    T get()
    {
        NoTLSFutureBase<T>::validate_state();
        T value{NoTLSFutureBase<T>::state->get_value()};
        NoTLSFutureBase<T>::state = nullptr;
        return value;
    }
};

template<>
class NoTLSFuture<void> : public NoTLSFutureBase<void>
{
public:
    using NoTLSFutureBase<void>::NoTLSFutureBase;

    void get()
    {
        validate_state();
        state->get_value();
        state = nullptr;
    }
};


template<typename T>
class NoTLSPromiseBase
{
public:
    NoTLSPromiseBase():
        state(std::make_shared<PromiseState<T>>())
    {
    }

    ~NoTLSPromiseBase()
    {
        if (state && !state.unique())
            state->break_promise();
    }

    NoTLSPromiseBase(NoTLSPromiseBase&& other) :
        state(std::move(other.state))
    {
    }

    NoTLSPromiseBase& operator=(NoTLSPromiseBase&& other)
    {
        state = std::move(other.state);
    }

    NoTLSPromiseBase(NoTLSPromiseBase const&) = delete;
    NoTLSPromiseBase operator=(NoTLSPromiseBase const&) = delete;

    NoTLSFuture<T> get_future()
    {
        if (future_retrieved)
        {
            //clang has problems with std::future_error::what() on vivid+overlay
            BOOST_THROW_EXCEPTION(std::runtime_error{"future_already_retrieved"});
        }
        future_retrieved = true;
        return NoTLSFuture<T>(state);
    }

protected:
    std::shared_ptr<PromiseState<T>> state;
private:
    bool future_retrieved{false};
};

template<typename T>
class NoTLSPromise : public NoTLSPromiseBase<T>
{
public:
    void set_value(T const& value)
    {
        NoTLSPromiseBase<T>::state->set_value(value);
    }

    void set_value(T&& value)
    {
        NoTLSPromiseBase<T>::state->set_value(std::move(value));
    }
};

template<>
class NoTLSPromise<void> : public NoTLSPromiseBase<void>
{
public:
    void set_value()
    {
        state->set_value();
    }
};

template<typename T>
template<typename Func, typename Result>
std::function<void()> NoTLSFutureBase<T>::make_continuation_for(
    NoTLSPromise<Result>&& resultant,
    Func&& continuation)
{
    // Ideally we'd be returning a MoveConstructible Callable and could avoid taking
    // a shared_ptr to resultant, but it's not clear that std::packaged_task doesn't use TLS,
    // which would somewhat defeat the purpose, and I don't feel like writing a functor class.
    return
        [promise = std::make_shared<NoTLSPromise<Result>>(std::move(resultant)),
         completion = std::move(continuation),
         state = NoTLSFutureBase<T>::state]()
        {
            promise->set_value(completion(NoTLSFuture<T>(std::move(state))));
        };
}

template<typename T>
template<typename Func>
std::function<void()> NoTLSFutureBase<T>::make_continuation_for(
    NoTLSPromise<void>&& resultant,
    Func&& continuation)
{
    return
        [promise = std::make_shared<NoTLSPromise<void>>(std::move(resultant)),
         completion = std::move(continuation),
         state = NoTLSFutureBase<T>::state]()
        {
            completion(NoTLSFuture<T>(std::move(state)));
            promise->set_value();
        };
}

}
}
#endif
