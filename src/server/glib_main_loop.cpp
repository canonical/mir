/*
 * Copyright © 2014-2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 *              Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#include "mir/glib_main_loop.h"
#include "mir/lockable_callback_wrapper.h"
#include "mir/basic_callback.h"

#include <stdexcept>
#include <algorithm>
#include <condition_variable>

#include <boost/throw_exception.hpp>

namespace
{

class AlarmImpl : public mir::time::Alarm
{
public:
    AlarmImpl(
        GMainContext* main_context,
        std::shared_ptr<mir::time::Clock> const& clock,
        std::unique_ptr<mir::LockableCallback>&& callback,
        std::function<void()> const& exception_handler)
        : main_context{g_main_context_ref(main_context)},
          clock{clock},
          state_{State::cancelled},
          exception_handler{exception_handler},
          wrapped_callback{std::make_shared<mir::LockableCallbackWrapper>(
              std::move(callback), [this] { state_ = State::triggered; })}
    {
    }

    ~AlarmImpl() override
    {
        gsource.ensure_no_further_dispatch();
        g_main_context_unref(main_context);
    }

    bool cancel() override
    {
        std::lock_guard<std::mutex> lock{alarm_mutex};

        gsource.ensure_no_further_dispatch();
        if (state_ ==  State::pending)
        {
            gsource = mir::detail::GSourceHandle{};
            state_ = State::cancelled;
        }
        return state_ == State::cancelled;
    }

    State state() const override
    {
        std::lock_guard<std::mutex> lock{alarm_mutex};
        return state_;
    }

    bool reschedule_in(std::chrono::milliseconds delay) override
    {
        return reschedule_for(clock->now() + delay);
    }

    bool reschedule_for(mir::time::Timestamp time_point) override
    {
        std::lock_guard<std::mutex> lock{alarm_mutex};

        auto old_state = state_;
        state_ = State::pending;
        gsource = mir::detail::add_timer_gsource(
            main_context,
            clock,
            wrapped_callback,
            exception_handler,
            time_point);

        return old_state == State::pending;
    }

private:
    mutable std::mutex alarm_mutex;
    GMainContext* main_context;
    std::shared_ptr<mir::time::Clock> const clock;
    State state_;
    std::function<void()> exception_handler;
    std::shared_ptr<mir::LockableCallback> wrapped_callback;
    mir::detail::GSourceHandle gsource;
};

}

mir::detail::GMainContextHandle::GMainContextHandle()
    : main_context{g_main_context_new()}
{
    if (!main_context)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create GMainContext"));
}

mir::detail::GMainContextHandle::~GMainContextHandle()
{
    if (main_context)
        g_main_context_unref(main_context);
}

mir::detail::GMainContextHandle::operator GMainContext*() const
{
    return main_context;
}


mir::GLibMainLoop::GLibMainLoop(
    std::shared_ptr<time::Clock> const& clock)
    : clock{clock},
      running{false},
      fd_sources{main_context},
      signal_sources{fd_sources},
      before_iteration_hook{[]{}}
{
}

void mir::GLibMainLoop::run()
{
    main_loop_exception = nullptr;
    running = true;

    while (running)
    {
        before_iteration_hook();
        g_main_context_iteration(main_context, TRUE);
    }

    if (main_loop_exception)
        std::rethrow_exception(main_loop_exception);
}

void mir::GLibMainLoop::stop()
{
    detail::add_idle_gsource(main_context, G_PRIORITY_HIGH,
        [this]
        {
            {
                std::lock_guard<std::mutex> lock{run_on_halt_mutex};
                running = false;
            }
            // We know any other thread sees running == false here, so don't need
            // to lock run_on_halt_queue.
            for (auto& action : run_on_halt_queue)
            {
                try { action(); }
                catch (...) { handle_exception(std::current_exception()); }
            }
            run_on_halt_queue.clear();

            g_main_context_wakeup(main_context);
        });
}

void mir::GLibMainLoop::register_signal_handler(
    std::initializer_list<int> sigs,
    std::function<void(int)> const& handler)
{
    auto const handler_with_exception_handling =
        [this, handler] (int sig)
        {
            try { handler(sig); }
            catch (...) { handle_exception(std::current_exception()); }
        };

    signal_sources.add(sigs, handler_with_exception_handling);
}

void mir::GLibMainLoop::register_signal_handler(
    std::initializer_list<int> sigs,
    mir::UniqueModulePtr<std::function<void(int)>> handler)
{
    std::shared_ptr<std::function<void(int)>> const shared_handler{std::move(handler)};

    auto const handler_with_exception_handling =
        [this, shared_handler] (int sig)
        {
            try { (*shared_handler)(sig); }
            catch (...) { handle_exception(std::current_exception()); }
        };

    signal_sources.add(sigs, handler_with_exception_handling);
}

void mir::GLibMainLoop::register_fd_handler(
    std::initializer_list<int> fds,
    void const* owner,
    std::function<void(int)> const& handler)
{
    auto const handler_with_exception_handling =
        [this, handler] (int fd)
        {
            try { handler(fd); }
            catch (...) { handle_exception(std::current_exception()); }
        };

    for (auto fd : fds)
        fd_sources.add(fd, owner, handler_with_exception_handling);
}

void mir::GLibMainLoop::register_fd_handler(
    std::initializer_list<int> fds,
    void const* owner,
    mir::UniqueModulePtr<std::function<void(int)>> handler)
{
    std::shared_ptr<std::function<void(int)>> const shared_handler{std::move(handler)};

    auto const handler_with_exception_handling =
        [this, shared_handler] (int fd)
        {
            try { (*shared_handler)(fd); }
            catch (...) { handle_exception(std::current_exception()); }
        };

    for (auto fd : fds)
        fd_sources.add(fd, owner, handler_with_exception_handling);
}

void mir::GLibMainLoop::unregister_fd_handler(
    void const* owner)
{
    fd_sources.remove_all_owned_by(owner);
}

void mir::GLibMainLoop::enqueue(void const* owner, ServerAction const& action)
{
    auto const action_with_exception_handling =
        [this, action]
        {
            try { action(); }
            catch (...) { handle_exception(std::current_exception()); }
        };

    detail::add_server_action_gsource(main_context, owner,
        action_with_exception_handling,
        [this] (void const* owner)
        {
            return should_process_actions_for(owner);
        });
}


void mir::GLibMainLoop::enqueue_with_guaranteed_execution (mir::ServerAction const& action)
{
    auto const action_with_exception_handling =
        [this]
        {
            try
            {
                mir::ServerAction action;
                {
                    std::lock_guard<std::mutex> lock{run_on_halt_mutex};
                    action = run_on_halt_queue.front();
                    run_on_halt_queue.pop_front();
                }
                action();
            }
            catch (...)
            {
                handle_exception(std::current_exception());
            }
        };

    {
        std::lock_guard<std::mutex> lock{run_on_halt_mutex};

        if (!running)
        {
            action();
            return;
        }
        else
        {
            run_on_halt_queue.push_back(action);
        }
    }

    detail::add_idle_gsource(
        main_context,
        G_PRIORITY_DEFAULT,
        action_with_exception_handling);
}

void mir::GLibMainLoop::pause_processing_for(void const* owner)
{
    std::lock_guard<std::mutex> lock{do_not_process_mutex};

    auto const iter = std::find(do_not_process.begin(), do_not_process.end(), owner);
    if (iter == do_not_process.end())
        do_not_process.push_back(owner);
}

void mir::GLibMainLoop::resume_processing_for(void const* owner)
{
    std::lock_guard<std::mutex> lock{do_not_process_mutex};

    auto const new_end = std::remove(do_not_process.begin(), do_not_process.end(), owner);
    do_not_process.erase(new_end, do_not_process.end());

    // Wake up the context to reprocess all sources
    g_main_context_wakeup(main_context);
}

bool mir::GLibMainLoop::should_process_actions_for(void const* owner)
{
    std::lock_guard<std::mutex> lock{do_not_process_mutex};

    auto const iter = std::find(do_not_process.begin(), do_not_process.end(), owner);
    return iter == do_not_process.end();
}

std::unique_ptr<mir::time::Alarm> mir::GLibMainLoop::create_alarm(
    std::function<void()> const& callback)
{
    return create_alarm(std::make_unique<BasicCallback>(callback));
}

std::unique_ptr<mir::time::Alarm> mir::GLibMainLoop::create_alarm(
    std::unique_ptr<LockableCallback> callback)
{
    auto const exception_hander =
        [this]
        {
            handle_exception(std::current_exception());
        };

    return std::make_unique<AlarmImpl>(
        main_context, clock, std::move(callback), exception_hander);
}

void mir::GLibMainLoop::reprocess_all_sources()
{
    std::condition_variable reprocessed_cv;
    std::mutex reprocessed_mutex;
    bool reprocessed = false;

    // Schedule setting the before_iteration_hook as an
    // idle source to ensure there is no concurrent access
    // to it.
    detail::add_idle_gsource(main_context, G_PRIORITY_HIGH,
        [&]
        {
            // GMainContexts process sources in order of decreasing priority.
            // Since all of our sources have priority higher than
            // G_PRIORITY_LOW, by adding a G_PRIORITY_LOW source, we can be
            // sure that when this source is processed all other sources will
            // have been processed before it. We add the source in the
            // before_iteration_hook to avoid premature notifications.
            before_iteration_hook =
                [&]
                {
                    detail::add_idle_gsource(main_context, G_PRIORITY_LOW,
                        [&]
                        {
                            std::lock_guard<std::mutex> lock{reprocessed_mutex};
                            reprocessed = true;
                            reprocessed_cv.notify_all();
                        });

                    before_iteration_hook = []{};
                };

            // Wake up the main loop to ensure that we eventually leave
            // g_main_context_iteration() and reprocess all sources after
            // having called the newly set before_iteration_hook.
            g_main_context_wakeup(main_context);
        });

    std::unique_lock<std::mutex> reprocessed_lock{reprocessed_mutex};
    reprocessed_cv.wait(reprocessed_lock, [&] { return reprocessed == true; });
}

void mir::GLibMainLoop::handle_exception(std::exception_ptr const& e)
{
    main_loop_exception = e;
    stop();
}

void mir::GLibMainLoop::spawn(std::function<void()>&& work)
{
    auto const action_with_exception_handling =
        [this, action = std::move(work)]
        {
            try { action(); }
            catch (...) { handle_exception(std::current_exception()); }
        };

    detail::add_server_action_gsource(
        main_context,
        nullptr,
        action_with_exception_handling,
        [](auto) { return true; });
}
