/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
#include <boost/exception/diagnostic_information.hpp>

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
        cv.wait(lk, [this]{ return set || captured_exception; });
    }

    template<class Rep, class Period>
    std::future_status wait_for(std::chrono::duration<Rep, Period> const& timeout_duration) const
    {
        std::unique_lock<std::mutex> lk(mutex);
        if (cv.wait_for(lk, timeout_duration, [this]{ return set || captured_exception; }))
            return std::future_status::ready;
        return std::future_status::timeout;
    }

    void break_promise()
    {
        {
            std::lock_guard<std::mutex> lk(mutex);
            if (set || captured_exception)
            {
                return;
            }

            captured_exception = std::make_exception_ptr(
                boost::enable_error_info(std::runtime_error("broken_promise"))
                    << boost::throw_function(__PRETTY_FUNCTION__)
                    << boost::throw_file(__FILE__)
                    << boost::throw_line(__LINE__));
        }
        continuation();
        cv.notify_all();
    }

    void set_exception(std::exception_ptr const& exception)
    {
        {
            std::lock_guard<std::mutex> lock{mutex};
            if (set || captured_exception)
            {
                BOOST_THROW_EXCEPTION(std::runtime_error{"promise_already_satisfied"});
            }
            captured_exception = exception;
        }
        continuation();
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
        }
        WriteLock(WriteLock&&) = default;
        WriteLock& operator=(WriteLock&&) = default;

        ~WriteLock()
        {
            parent.set = true;
            lock.unlock();
            parent.continuation();
            parent.cv.notify_all();
        }
    private:
        PromiseStateBase& parent;
        std::unique_lock<std::mutex> lock;
    };

    class ReadLock
    {
    public:
        ReadLock(PromiseStateBase& parent)
            : lock{parent.mutex}
        {
            parent.cv.wait(lock, [&parent]{ return parent.set || parent.captured_exception; });
            if (parent.captured_exception)
            {
                std::rethrow_exception(parent.captured_exception);
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
    std::exception_ptr captured_exception;

    class OneShotContinuation
    {
    public:
        OneShotContinuation()
            : OneShotContinuation([](){})
        {
        }

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
            continuation = [](){};
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
        return std::move(value);
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

    /**
     * Detach the asynchronous operation from this future.
     *
     * This decouples the asynchronous operation represented by the NoTLSFuture from the
     * NoTLSFuture handle itself, similar to std::thread::detach().
     *
     * After calling detach() this future is no longer in a valid state; valid() will return
     * false, and it is no longer possible to wait for or to extract the value of the future.
     *
     * This does not change the state of the underlying asynchronous operation. If this
     * future is the result of a continuation, that continuation will still be run.
     */
    void detach()
    {
        state = nullptr;
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

    void set_exception(std::exception_ptr const& exception)
    {
        state->set_exception(exception);
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
            try
            {
                promise->set_value(completion(NoTLSFuture<T>(std::move(state))));
            }
            catch (...)
            {
                promise->set_exception(std::current_exception());
            }
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
            try
            {
                completion(NoTLSFuture<T>(std::move(state)));
                promise->set_value();
            }
            catch (...)
            {
                promise->set_exception(std::current_exception());
            }
        };
}

}
}
#endif
