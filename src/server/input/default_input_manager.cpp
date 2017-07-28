/*
 * Copyright © 2015-2016 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "default_input_manager.h"

#include "mir/input/platform.h"
#include "mir/dispatch/action_queue.h"
#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/dispatch/threaded_dispatcher.h"

#include "mir/main_loop.h"
#include "mir/thread_name.h"
#include "mir/unwind_helpers.h"
#include "mir/terminate_with_current_exception.h"

#include <future>

namespace mi = mir::input;

mi::DefaultInputManager::DefaultInputManager(
    std::shared_ptr<dispatch::MultiplexingDispatchable> const& multiplexer,
    std::shared_ptr<Platform> const& platform) :
    platform{platform},
    multiplexer{multiplexer},
    queue{std::make_shared<mir::dispatch::ActionQueue>()},
    state{State::stopped}
{
}

mi::DefaultInputManager::~DefaultInputManager()
{
    stop();
}

void mi::DefaultInputManager::start()
{
    auto expected = State::stopped;

    if (!state.compare_exchange_strong(expected, State::starting))
        return;

    auto reset_to_stopped_on_failure = on_unwind([this]
        {
            auto expected = State::starting;
            state.compare_exchange_strong(expected, State::stopped);
        });

    multiplexer->add_watch(queue);
    auto unregister_queue = on_unwind([this]
        {
            // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=62258
            // After using rethrow_exception() (and catching the exception),
            // all subsequent calls to uncaught_exception() return `true'.
            if (state == State::started) return;

            multiplexer->remove_watch(queue);
        });


    auto started_promise = std::make_shared<std::promise<void>>();
    auto started_future = started_promise->get_future();

    /*
     * We need the starting-lambda to own started_promise so that it is guaranteed that
     * started_future gets signalled; either by ->set_value in the success path or
     * by the destruction of started_promise generating a broken_promise exception.
     */
    queue->enqueue([this,promise = std::move(started_promise)]()
                   {
                        start_platforms();
                        promise->set_value();
                   });

    input_thread = std::make_unique<dispatch::ThreadedDispatcher>(
        "Mir/Input Reader",
        multiplexer,
        [this]()
        {
            stop_platforms();
            multiplexer->remove_watch(queue);
            state = State::stopped;
            mir::terminate_with_current_exception();
        });

    started_future.wait();

    expected = State::starting;
    state.compare_exchange_strong(expected, State::started);
}

void mi::DefaultInputManager::stop()
{
    auto expected = State::started;

    if (!state.compare_exchange_strong(expected, State::stopping))
        return;

    auto reset_to_started_on_failure = on_unwind([this]
        {
            auto expected = State::stopping;
            state.compare_exchange_strong(expected, State::started);
        });

    auto const stop_promise = std::make_shared<std::promise<void>>();

    queue->enqueue([stop_promise,this]()
                   {
                       stop_platforms();
                       stop_promise->set_value();
                   });

    stop_promise->get_future().wait();

    auto restore_platforms = on_unwind(
        [this]
        {
            // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=62258
            // After using rethrow_exception() (and catching the exception),
            // all subsequent calls to uncaught_exception() return `true'.
            if (state == State::stopped) return;

            queue->enqueue([this](){ start_platforms(); });
        });

    multiplexer->remove_watch(queue);
    auto register_queue = on_unwind([this]
        {
            // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=62258
            // After using rethrow_exception() (and catching the exception),
            // all subsequent calls to uncaught_exception() return `true'.
            if (state == State::stopped) return;

            multiplexer->add_watch(queue);
        });

    input_thread.reset();

    state = State::stopped;
}

void mi::DefaultInputManager::start_platforms()
{
    platform->start();
    multiplexer->add_watch(platform->dispatchable());
}

void mi::DefaultInputManager::stop_platforms()
{
    multiplexer->remove_watch(platform->dispatchable());
    platform->stop();
}

void mi::DefaultInputManager::pause_for_config()
{
    queue->enqueue([this](){platform->pause_for_config();});
}

void mi::DefaultInputManager::continue_after_config()
{
    queue->enqueue([this](){platform->continue_after_config();});
}
