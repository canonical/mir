/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/asio_main_loop.h"

#include "boost/date_time/posix_time/conversion.hpp"

#include <cassert>
#include <mutex>
#include <condition_variable>

class mir::AsioMainLoop::SignalHandler
{
public:
    SignalHandler(boost::asio::io_service& io,
                  std::initializer_list<int> signals,
                  std::function<void(int)> const& handler)
        : signal_set{io},
          handler{handler}
    {
        for (auto sig : signals)
            signal_set.add(sig);
    }

    void async_wait()
    {
        signal_set.async_wait(
            std::bind(&SignalHandler::handle, this,
                      std::placeholders::_1, std::placeholders::_2));
    }

private:
    void handle(boost::system::error_code err, int sig)
    {
        if (!err)
        {
            handler(sig);
            signal_set.async_wait(
                std::bind(&SignalHandler::handle, this,
                          std::placeholders::_1, std::placeholders::_2));
        }
    }

    boost::asio::signal_set signal_set;
    std::function<void(int)> handler;
};

class mir::AsioMainLoop::FDHandler
{
public:
    FDHandler(boost::asio::io_service& io,
              std::initializer_list<int> fds,
              std::function<void(int)> const& handler)
        : handler{handler}
    {
        for (auto fd : fds)
        {
            auto raw = new boost::asio::posix::stream_descriptor{io, fd};
            auto s = std::unique_ptr<boost::asio::posix::stream_descriptor>(raw);
            stream_descriptors.push_back(std::move(s));
        }
    }

    void async_wait()
    {
        for (auto const& s : stream_descriptors)
        {
            s->async_read_some(
                boost::asio::null_buffers(),
                std::bind(&FDHandler::handle, this,
                          std::placeholders::_1, std::placeholders::_2, s.get()));
        }
    }

private:
    void handle(boost::system::error_code err, size_t /*bytes*/,
                boost::asio::posix::stream_descriptor* s)
    {
        if (!err)
        {
            handler(s->native_handle());
            s->async_read_some(
                boost::asio::null_buffers(),
                std::bind(&FDHandler::handle, this,
                          std::placeholders::_1, std::placeholders::_2, s));
        }
    }

    std::vector<std::unique_ptr<boost::asio::posix::stream_descriptor>> stream_descriptors;
    std::function<void(int)> handler;
};

/*
 * We need to define an empty constructor and destructor in the .cpp file,
 * so that we can use unique_ptr to hold SignalHandler. Otherwise, users
 * of AsioMainLoop end up creating default constructors and destructors
 * that don't have complete type information for SignalHandler and fail
 * to compile.
 */
mir::AsioMainLoop::AsioMainLoop()
    : work{io}
{
}

mir::AsioMainLoop::~AsioMainLoop() noexcept(true)
{
}

void mir::AsioMainLoop::run()
{
    io.run();
}

void mir::AsioMainLoop::stop()
{
    io.stop();
}

void mir::AsioMainLoop::register_signal_handler(
    std::initializer_list<int> signals,
    std::function<void(int)> const& handler)
{
    assert(handler);

    auto sig_handler = std::unique_ptr<SignalHandler>{
        new SignalHandler{io, signals, handler}};

    sig_handler->async_wait();

    signal_handlers.push_back(std::move(sig_handler));
}

void mir::AsioMainLoop::register_fd_handler(
    std::initializer_list<int> fds,
    std::function<void(int)> const& handler)
{
    assert(handler);

    auto fd_handler = std::unique_ptr<FDHandler>{
        new FDHandler{io, fds, handler}};

    fd_handler->async_wait();

    fd_handlers.push_back(std::move(fd_handler));
}

namespace
{
class AlarmImpl : public mir::time::Alarm
{
public:
    AlarmImpl(boost::asio::io_service& io,
              std::chrono::milliseconds delay,
              std::function<void(void)> callback);

    AlarmImpl(boost::asio::io_service& io,
              mir::time::Timestamp time_point,
              std::function<void(void)> callback);

    ~AlarmImpl() noexcept override;

    bool cancel() override;
    State state() const override;

    bool reschedule_in(std::chrono::milliseconds delay) override;
    bool reschedule_for(mir::time::Timestamp time_point) override;
private:
    void update_timer();
    struct InternalState
    {
        explicit InternalState(std::function<void(void)> callback)
            : callback{callback}
        {
        }

        mutable std::mutex m;
        std::function<void(void)> callback;
        State state;
    };

    boost::asio::deadline_timer timer;
    std::shared_ptr<InternalState> data;
};

AlarmImpl::AlarmImpl(boost::asio::io_service& io,
                     std::chrono::milliseconds delay,
                     std::function<void ()> callback)
    : timer{io},
      data{std::make_shared<InternalState>(callback)}
{
    reschedule_in(delay);
}

AlarmImpl::AlarmImpl(boost::asio::io_service& io,
                     mir::time::Timestamp time_point,
                     std::function<void ()> callback)
    : timer{io},
      data{std::make_shared<InternalState>(callback)}
{
    reschedule_for(time_point);
}

AlarmImpl::~AlarmImpl() noexcept
{
    AlarmImpl::cancel();
}

bool AlarmImpl::cancel()
{
    std::lock_guard<decltype(data->m)> lock(data->m);
    if (data->state == triggered)
        return false;

    data->state = cancelled;
    timer.cancel();
    return true;
}

mir::time::Alarm::State AlarmImpl::state() const
{
    std::lock_guard<decltype(data->m)> lock(data->m);

    return data->state;
}

bool AlarmImpl::reschedule_in(std::chrono::milliseconds delay)
{
    bool cancelling = timer.expires_from_now(boost::posix_time::milliseconds{delay.count()});
    update_timer();
    return cancelling;
}

bool AlarmImpl::reschedule_for(mir::time::Timestamp time_point)
{
    bool cancelling =
        timer.expires_at(boost::posix_time::from_time_t(
                std::chrono::high_resolution_clock::to_time_t(time_point)
                ));
    update_timer();
    return cancelling;
}

void AlarmImpl::update_timer()
{
    std::lock_guard<decltype(data->m)> lock(data->m);
    // Awkwardly, we can't stop the async_wait handler from being called
    // on a destroyed AlarmImpl. This means we need to wedge a shared_ptr
    // into the async_wait callback.
    std::weak_ptr<InternalState> possible_data = data;
    timer.async_wait([possible_data](boost::system::error_code const& ec)
    {
        auto data = possible_data.lock();
        if (!data)
            return;

        std::unique_lock<decltype(data->m)> lock(data->m);
        if (!ec && data->state == pending)
        {
            data->state = triggered;
            lock.unlock();
            data->callback();
        }
    });
    data->state = pending;
}
}

std::unique_ptr<mir::time::Alarm> mir::AsioMainLoop::notify_in(std::chrono::milliseconds delay,
                                                               std::function<void()> callback)
{
    return std::unique_ptr<mir::time::Alarm>{new AlarmImpl{io, delay, callback}};
}

std::unique_ptr<mir::time::Alarm> mir::AsioMainLoop::notify_at(mir::time::Timestamp time_point,
                                                               std::function<void()> callback)
{
    return std::unique_ptr<mir::time::Alarm>{new AlarmImpl{io, time_point, callback}};

}
void mir::AsioMainLoop::enqueue(void const* owner, ServerAction const& action)
{
    {
        std::lock_guard<std::mutex> lock{server_actions_mutex};
        server_actions.push_back({owner, action});
    }

    io.post([this] { process_server_actions(); });
}

void mir::AsioMainLoop::pause_processing_for(void const* owner)
{
    std::lock_guard<std::mutex> lock{server_actions_mutex};
    do_not_process.insert(owner);
}

void mir::AsioMainLoop::resume_processing_for(void const* owner)
{
    {
        std::lock_guard<std::mutex> lock{server_actions_mutex};
        do_not_process.erase(owner);
    }

    io.post([this] { process_server_actions(); });
}

void mir::AsioMainLoop::process_server_actions()
{
    std::unique_lock<std::mutex> lock{server_actions_mutex};

    size_t i = 0;

    while (i < server_actions.size())
    {
        /* 
         * It's safe to use references to elements, since std::deque<>
         * guarantees that references remain valid after appends, which is
         * the only operation that can be performed on server_actions outside
         * this function (in AsioMainLoop::post()).
         */
        auto const& owner = server_actions[i].first;
        auto const& action = server_actions[i].second;

        if (do_not_process.find(owner) == do_not_process.end())
        {
            lock.unlock();
            action();
            lock.lock();
            /*
             * This erase is always ok, since outside this function
             * we only append to server_actions, i.e., our index i
             * is guaranteed to remain valid and correct.
             */
            server_actions.erase(server_actions.begin() + i);
        }
        else
        {
            ++i;
        }
    }
}
