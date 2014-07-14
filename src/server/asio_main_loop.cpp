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

namespace
{

void synchronous_server_action(
    mir::ServerActionQueue& queue,
    boost::optional<std::thread::id> queue_thread_id,
    mir::ServerAction const& action)
{
    if (!queue_thread_id || *queue_thread_id == std::this_thread::get_id())
    {
        try { action(); } catch(...) {}
    }
    else
    {
        std::mutex done_mutex;
        bool done{false};
        std::condition_variable done_condition;

        queue.enqueue(&queue, [&]
            {
                std::lock_guard<std::mutex> lock(done_mutex);

                try { action(); } catch(...) {}

                done = true;
                done_condition.notify_one();
            });

        std::unique_lock<std::mutex> lock(done_mutex);
        done_condition.wait(lock, [&] { return done; });
    }
}

struct MirClockTimerTraits
{
    // TODO the clock used by the main loop is a global setting, this is a restriction
    // of boost::asio only allowing static methods inside the taits type.
    struct TimerServiceClockStorage
    {
    public:
        void set_clock(std::shared_ptr<mir::time::Clock const> const& clock)
        {
            std::lock_guard<std::mutex> lock(timer_service_mutex);
            auto stored_clock = timer_service_clock.lock();
            if (stored_clock && stored_clock != clock)
                BOOST_THROW_EXCEPTION(std::logic_error("A clock is already in use as time source for mir::AsioMainLoop"));
            timer_service_clock = clock;
        }
        mir::time::Timestamp now()
        {
            std::lock_guard<std::mutex> lock(timer_service_mutex);
            auto clock = timer_service_clock.lock();
            if (!clock)
                BOOST_THROW_EXCEPTION(std::logic_error("No clock available to create time stamp"));
            return clock->sample();
        }
    private:
        std::mutex timer_service_mutex;
        std::weak_ptr<mir::time::Clock const> timer_service_clock;
    };

    static TimerServiceClockStorage clock_storage;

    static void set_clock(std::shared_ptr<mir::time::Clock const> const& clock)
    {
        clock_storage.set_clock(clock);
    }

    // time_traits interface required by boost::asio::deadline_timer{_service}
    typedef mir::time::Timestamp time_type;
    typedef std::chrono::milliseconds duration_type;


    static time_type now()
    {
        return clock_storage.now();
    }

    static time_type add(const time_type& t, const duration_type& d)
    {
        return t + d;
    }

    static duration_type subtract(const time_type& t1, const time_type& t2)
    {
        return std::chrono::duration_cast<duration_type>(t1 - t2);
    }

    static bool less_than(const time_type& t1, const time_type& t2)
    {
        return t1 < t2;
    }

    static boost::posix_time::time_duration to_posix_duration(
        const duration_type& d)
    {
        return boost::posix_time::millisec(d.count());
    }
};

MirClockTimerTraits::TimerServiceClockStorage MirClockTimerTraits::clock_storage;

typedef boost::asio::basic_deadline_timer<mir::time::Timestamp, MirClockTimerTraits> deadline_timer;
}

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
              void const* owner,
              std::function<void(int)> const& handler)
        : handler{handler}, owner{owner}
    {
        for (auto fd : fds)
        {
            auto raw = new boost::asio::posix::stream_descriptor{io, fd};
            auto s = std::unique_ptr<boost::asio::posix::stream_descriptor>(raw);
            stream_descriptors.push_back(std::move(s));
        }
    }

    ~FDHandler()
    {
        for (auto & desc : stream_descriptors)
        {
            desc->release(); // release native handle which is not owned by main loop
        }
    }

    bool is_owned_by(void const* possible_owner) const
    {
        return owner == possible_owner;
    }

    static void async_wait(std::shared_ptr<FDHandler> const& fd_handler, ServerActionQueue& queue)
    {
        for (auto const& s : fd_handler->stream_descriptors)
            read_some(s.get(), fd_handler, queue);
    }

private:
    static void read_some(boost::asio::posix::stream_descriptor* s, std::weak_ptr<FDHandler> const& possible_fd_handler, ServerActionQueue& queue)
    {
        s->async_read_some(
            boost::asio::null_buffers(),
            [possible_fd_handler,s,&queue](boost::system::error_code err, size_t /*bytes*/)
            {
                if (err)
                    return;

                // The actual execution of the fd handler is moved to the action queue.This allows us to synchronously
                // unregister FDHandlers even when they are about to be executed. We do this because of the way Asio
                // copies and executes pending completion handlers.
                // In worst case during the call to unregister the FDHandler, it may still be executed, but not after
                // the unregister call returned.
                queue.enqueue(
                    s,
                    [possible_fd_handler, s, &queue]()
                    {
                        auto fd_handler = possible_fd_handler.lock();
                        if (!fd_handler)
                            return;

                        fd_handler->handler(s->native_handle());
                        fd_handler.reset();

                        if (possible_fd_handler.lock())
                            read_some(s, possible_fd_handler, queue);
                    });
            });
    }

    std::vector<std::unique_ptr<boost::asio::posix::stream_descriptor>> stream_descriptors;
    std::function<void(int)> handler;
    void const* owner;
};

/*
 * We need to define the constructor and destructor in the .cpp file,
 * so that we can use unique_ptr to hold SignalHandler. Otherwise, users
 * of AsioMainLoop end up creating constructors and destructors that
 * don't have complete type information for SignalHandler and fail
 * to compile.
 */
mir::AsioMainLoop::AsioMainLoop(std::shared_ptr<time::Clock> const& clock)
    : work{io}, clock(clock)
{
    MirClockTimerTraits::set_clock(clock);
}

mir::AsioMainLoop::~AsioMainLoop() noexcept(true)
{
}

void mir::AsioMainLoop::run()
{
    main_loop_thread = std::this_thread::get_id();
    io.run();
}

void mir::AsioMainLoop::stop()
{
    io.stop();
    main_loop_thread.reset();
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
    void const* owner,
    std::function<void(int)> const& handler)
{
    assert(handler);

    auto fd_handler = std::make_shared<FDHandler>(io, fds, owner, handler);

    FDHandler::async_wait(fd_handler, *this);

    std::lock_guard<std::mutex> lock(fd_handlers_mutex);
    fd_handlers.push_back(std::move(fd_handler));
}

void mir::AsioMainLoop::unregister_fd_handler(void const* owner)
{
    // synchronous_server_action makes sure that with the
    // completion of the method unregister_fd_handler the
    // handler will no longer be called.
    // There is a chance for a fd handler callback to happen before
    // the completion of this method.
    synchronous_server_action(
        *this,
        main_loop_thread,
        [this,owner]()
        {
            std::lock_guard<std::mutex> lock(fd_handlers_mutex);
            auto end_of_valid = remove_if(
                begin(fd_handlers),
                end(fd_handlers),
                [owner](std::shared_ptr<FDHandler> const& item)
                {
                    return item->is_owned_by(owner);
                });
            fd_handlers.erase(end_of_valid, end(fd_handlers));
        });
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

    AlarmImpl(boost::asio::io_service& io,
              std::function<void(void)> callback);

    ~AlarmImpl() noexcept override;

    bool cancel() override;
    State state() const override;

    bool reschedule_in(std::chrono::milliseconds delay) override;
    bool reschedule_for(mir::time::Timestamp time_point) override;
private:
    void stop();
    bool cancel_unlocked();
    void update_timer();
    struct InternalState
    {
        explicit InternalState(std::function<void(void)> callback)
            : callback{callback}, state{pending}
        {
        }

        std::mutex m;
        int callbacks_running = 0;
        std::condition_variable callback_done;
        std::function<void(void)> const callback;
        State state;
    };

    ::deadline_timer timer;
    std::shared_ptr<InternalState> const data;
    std::function<void(boost::system::error_code const& ec)> const handler;

    friend auto make_handler(std::weak_ptr<InternalState> possible_data)
    -> std::function<void(boost::system::error_code const& ec)>;
};

auto make_handler(std::weak_ptr<AlarmImpl::InternalState> possible_data)
-> std::function<void(boost::system::error_code const& ec)>
{
    // Awkwardly, we can't stop the async_wait handler from being called
    // on a destroyed AlarmImpl. This means we need to wedge a weak_ptr
    // into the async_wait callback.
    return [possible_data](boost::system::error_code const& ec)
    {
        if (!ec)
        {
            if (auto data = possible_data.lock())
            {
                std::unique_lock<decltype(data->m)> lock(data->m);
                if (data->state == mir::time::Alarm::pending)
                {
                    data->state = mir::time::Alarm::triggered;
                    ++data->callbacks_running;
                    lock.unlock();
                    data->callback();
                    lock.lock();
                    --data->callbacks_running;
                    data->callback_done.notify_all();
                }
            }
        }
    };
}

AlarmImpl::AlarmImpl(boost::asio::io_service& io,
                     std::chrono::milliseconds delay,
                     std::function<void ()> callback)
    : AlarmImpl(io, callback)
{
    reschedule_in(delay);
}

AlarmImpl::AlarmImpl(boost::asio::io_service& io,
                     mir::time::Timestamp time_point,
                     std::function<void ()> callback)
    : AlarmImpl(io, callback)
{
    reschedule_for(time_point);
}

AlarmImpl::AlarmImpl(boost::asio::io_service& io,
                     std::function<void(void)> callback)
    : timer{io},
      data{std::make_shared<InternalState>(callback)},
      handler{make_handler(data)}
{
    data->state = triggered;
}

AlarmImpl::~AlarmImpl() noexcept
{
    AlarmImpl::stop();
}

void AlarmImpl::stop()
{
    std::unique_lock<decltype(data->m)> lock(data->m);

    while (data->callbacks_running > 0)
        data->callback_done.wait(lock);

    cancel_unlocked();
}

bool AlarmImpl::cancel()
{
    std::lock_guard<decltype(data->m)> lock(data->m);
    return cancel_unlocked();
}

bool AlarmImpl::cancel_unlocked()
{
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
    bool cancelling = timer.expires_from_now(delay);
    update_timer();
    return cancelling;
}

bool AlarmImpl::reschedule_for(mir::time::Timestamp time_point)
{
    bool cancelling = timer.expires_at(time_point);
    update_timer();
    return cancelling;
}

void AlarmImpl::update_timer()
{
    timer.async_wait(handler);
    std::lock_guard<decltype(data->m)> lock(data->m);
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

std::unique_ptr<mir::time::Alarm> mir::AsioMainLoop::create_alarm(std::function<void()> callback)
{
    return std::unique_ptr<mir::time::Alarm>{new AlarmImpl{io, callback}};
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
