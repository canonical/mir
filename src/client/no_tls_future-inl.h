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
class PromiseState
{
public:
    template<class Rep, class Period>
    std::future_status wait_for(std::chrono::duration<Rep, Period> const& timeout_duration) const
    {
        std::unique_lock<std::mutex> lk(mutex);
        if (cv.wait_for(lk, timeout_duration, [this]{ return set || broken; }))
            return std::future_status::ready;
        return std::future_status::timeout;
    }

    void set_value(T const& val)
    {
        std::lock_guard<std::mutex> lk(mutex);
        set = true;
        if (continuation)
        {
            value = val;
            continuation(std::move(value));
        }
        else
        {
            value = val;
        }
        cv.notify_all();
    }

    void set_value(T&& val)
    {
        std::lock_guard<std::mutex> lk(mutex);
        set = true;
        if (continuation)
        {
            continuation(std::move(val));
        }
        else
        {
            value = val;
        }
        cv.notify_all();
    }

    T get_value()
    {
        std::unique_lock<std::mutex> lk(mutex);
        cv.wait(lk, [this]{ return set || broken; });
        if (broken)
        {
            //clang has problems with std::future_error::what() on vivid+overlay
            BOOST_THROW_EXCEPTION(std::runtime_error("broken_promise"));
        }
        return value;
    }

    void break_promise()
    {
        std::lock_guard<std::mutex> lk(mutex);
        if (!set)
        {
            broken = true;
            if (exception_continuation)
            {
                exception_continuation(std::make_exception_ptr(std::runtime_error("broken_promise")));
            }
        }
        cv.notify_all();
    }

    PromiseState() = default;
    PromiseState(PromiseState const&) = delete;
    PromiseState(PromiseState &&) = delete;
    PromiseState& operator=(PromiseState const&) = delete;
    PromiseState& operator=(PromiseState &&) = delete;

private:
    std::mutex mutable mutex;
    std::condition_variable mutable cv;
    bool set{false};
    bool broken{false};
    T value;
    std::function<void(typename std::add_rvalue_reference<T>::type)> continuation;
    std::function<void(std::exception_ptr const&)> exception_continuation;

    friend class NoTLSFuture<T>;
    void set_continuation(std::function<void(typename std::add_rvalue_reference<T>::type)> const& continuation)
    {
        std::lock_guard<std::mutex> lk{mutex};
        if (set)
        {
            continuation(std::move(value));
        }
        this->continuation = continuation;
    }

    friend class NoTLSFutureBase<T>;
    void set_exception_continuation(std::function<void(std::exception_ptr const&)> const& exception_continuation)
    {
        std::lock_guard<std::mutex> lk{mutex};
        if (broken)
        {
            exception_continuation(std::make_exception_ptr(std::runtime_error("broken_promise")));
        }
        this->exception_continuation = exception_continuation;
    }
};

template<>
class PromiseState<void>
{
public:
    template<class Rep, class Period>
    std::future_status wait_for(std::chrono::duration<Rep, Period> const& timeout_duration) const
    {
        std::unique_lock<std::mutex> lk(mutex);
        if (cv.wait_for(lk, timeout_duration, [this]{ return set || broken; }))
            return std::future_status::ready;
        return std::future_status::timeout;
    }

    void set_value()
    {
        std::lock_guard<std::mutex> lk(mutex);
        set = true;
        if (continuation)
        {
            continuation();
        }
        cv.notify_all();
    }

    void get_value()
    {
        std::unique_lock<std::mutex> lk(mutex);
        cv.wait(lk, [this]{ return set || broken; });
        if (broken)
        {
            //clang has problems with std::future_error::what() on vivid+overlay
            BOOST_THROW_EXCEPTION(std::runtime_error("broken_promise"));
        }
    }

    void break_promise()
    {
        std::lock_guard<std::mutex> lk(mutex);
        if (!set)
        {
            broken = true;
            if (exception_continuation)
            {
                exception_continuation(std::make_exception_ptr(std::runtime_error("broken_promise")));
            }
        }
        cv.notify_all();
    }

    PromiseState() = default;
    PromiseState(PromiseState const&) = delete;
    PromiseState(PromiseState &&) = delete;
    PromiseState& operator=(PromiseState const&) = delete;
    PromiseState& operator=(PromiseState &&) = delete;

private:
    std::mutex mutable mutex;
    std::condition_variable mutable cv;
    bool set{false};
    bool broken{false};
    std::function<void()> continuation;
    std::function<void(std::exception_ptr const&)> exception_continuation;

    friend class NoTLSFuture<void>;
    void set_continuation(std::function<void(void)> const& continuation)
    {
        std::lock_guard<std::mutex> lk{mutex};
        if (set)
        {
            continuation();
        }
        this->continuation = continuation;
    }

    friend class NoTLSFutureBase<void>;
    void set_exception_continuation(std::function<void(std::exception_ptr const&)> const& exception_continuation)
    {
        std::lock_guard<std::mutex> lk{mutex};
        if (broken)
        {
            exception_continuation(std::make_exception_ptr(std::runtime_error("broken_promise")));
        }
        this->exception_continuation = exception_continuation;
    }
};


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

    void or_else(std::function<void(std::exception_ptr const&)> const& handler)
    {
        state->set_exception_continuation(handler);
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
        auto value = NoTLSFutureBase<T>::state->get_value();
        NoTLSFutureBase<T>::state = nullptr;
        return value;
    }

    void and_then(std::function<void(typename std::add_rvalue_reference<T>::type)> const& continuation)
    {
        NoTLSFutureBase<T>::state->set_continuation(continuation);
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

    void and_then(std::function<void()> const& continuation)
    {
        state->set_continuation(continuation);
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

}
}
#endif
