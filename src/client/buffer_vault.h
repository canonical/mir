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

#ifndef MIR_CLIENT_BUFFER_VAULT_H_
#define MIR_CLIENT_BUFFER_VAULT_H_

#include "mir/geometry/size.h"
#include "mir_toolkit/common.h"
#include "mir_toolkit/mir_native_buffer.h"
#include <memory>
#include <future>
#include <deque>
#include <map>

namespace mir
{
namespace protobuf { class Buffer; }
namespace client
{
class ServerBufferRequests
{
public:
    virtual void allocate_buffer(geometry::Size size, MirPixelFormat format, int usage) = 0;
    virtual void free_buffer(int buffer_id) = 0;
    virtual void submit_buffer(ClientBuffer&) = 0;
    virtual ~ServerBufferRequests() = default;
protected:
    ServerBufferRequests() = default;
    ServerBufferRequests(ServerBufferRequests const&) = delete;
    ServerBufferRequests& operator=(ServerBufferRequests const&) = delete;
};

class ClientBufferFactory;
class ClientBuffer;

//Hybris and libc can have TLS collisions if using std::promise
//https://github.com/libhybris/libhybris/issues/212
//

template<typename T>
class State
{
public:
    void set_value(T val)
    {
        std::lock_guard<std::mutex> lk(mutex);
        set = true;
        value = std::move(val);
        cv.notify_all();
    }
    T get_value()
    {
        std::unique_lock<std::mutex> lk(mutex);
        cv.wait(lk, [this]{ return set || broken; });
        if (broken)
            throw std::future_error(std::future_errc::broken_promise);
        return value; 
    }
    void break_promise()
    {
        std::lock_guard<std::mutex> lk(mutex);
        if (!set)
            broken = true;
        cv.notify_all();
    }
private:
    std::mutex mutex;
    std::condition_variable cv;
    bool set{false};
    bool broken{false};
    T value;
};

template<typename T>
struct NoTLSFuture
{
    NoTLSFuture() :
        state(nullptr) 
    {
    }

    NoTLSFuture(std::shared_ptr<State<T>> const& state) :
        state(state)
    {
    }
    ~NoTLSFuture() = default;
    NoTLSFuture(NoTLSFuture&& other) :
        state(other.state)
    {
    }
    NoTLSFuture& operator=(NoTLSFuture&& other)
    {
        state = other.state;
        return *this;
    }

    NoTLSFuture(NoTLSFuture const&) = delete;
    NoTLSFuture& operator=(NoTLSFuture const&) = delete;

    T get()
    {
        return state->get_value();
    }

    bool valid() const
    {
        return state != nullptr;
    }
private:
    std::shared_ptr<State<T>> state;
};

template<typename T>
class NoTLSPromise
{
public:
    NoTLSPromise():
        state(std::make_shared<State<T>>())
    {
    }
    ~NoTLSPromise()
    {
        if (state && !state.unique())
            state->break_promise();
    }

    NoTLSPromise(NoTLSPromise&& other) :
        state(other.state)
    {
        other.state = nullptr;
    }
    NoTLSPromise& operator=(NoTLSPromise&& other)
    {
        state = other.state;
        other.state = nullptr;
    }

    NoTLSPromise(NoTLSPromise const&) = delete;
    NoTLSPromise operator=(NoTLSPromise const&) = delete;

    void set_value(T value)
    {
        state->set_value(value);
    }
    NoTLSFuture<T> get_future()
    {
        return NoTLSFuture<T>(state);
    }

private:
    std::shared_ptr<State<T>> state; 
};

class BufferVault
{
public:
    BufferVault(
        std::shared_ptr<ClientBufferFactory> const&,
        std::shared_ptr<ServerBufferRequests> const&,
        geometry::Size size, MirPixelFormat format, int usage,
        unsigned int initial_nbuffers);
    ~BufferVault();

    NoTLSFuture<std::shared_ptr<ClientBuffer>> withdraw();
    void deposit(std::shared_ptr<ClientBuffer> const& buffer);
    void wire_transfer_inbound(protobuf::Buffer const&);
    void wire_transfer_outbound(std::shared_ptr<ClientBuffer> const& buffer);
    //TODO: class will handle allocation, transition, and destruction around resize notification

private:
    std::shared_ptr<ClientBufferFactory> const factory;
    std::shared_ptr<ServerBufferRequests> const server_requests;
    MirPixelFormat const format;

    enum class Owner;
    struct BufferEntry
    {
        std::shared_ptr<ClientBuffer> buffer;
        Owner owner;
    };

    std::mutex mutex;
    std::map<int, BufferEntry> buffers;
    std::deque<NoTLSPromise<std::shared_ptr<ClientBuffer>>> promises;
};
}
}
#endif /* MIR_CLIENT_BUFFER_VAULT_H_ */
