/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "default_input_manager.h"
#include "android/input_reader_dispatchable.h"

#include "mir/input/platform.h"
#include "mir/input/legacy_input_dispatchable.h"
#include "mir/dispatch/action_queue.h"
#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/dispatch/threaded_dispatcher.h"

#include "mir/main_loop.h"
#include "mir/thread_name.h"
#include "mir/unwind_helpers.h"
#include "mir/terminate_with_current_exception.h"

#include <condition_variable>
#include <mutex>
#include <future>

namespace mi = mir::input;
namespace mia = mi::android;

mi::DefaultInputManager::DefaultInputManager(std::shared_ptr<dispatch::MultiplexingDispatchable> const& multiplexer,
                                             std::shared_ptr<LegacyInputDispatchable>  const& legacy_dispatchable)
    : multiplexer{multiplexer}, legacy_dispatchable{legacy_dispatchable}, queue{std::make_shared<mir::dispatch::ActionQueue>()}, state{State::stopped}
{
}

mi::DefaultInputManager::~DefaultInputManager()
{
    stop();
}

void mi::DefaultInputManager::add_platform(std::shared_ptr<Platform> const& platform)
{
    if (state == State::started)
    {
        queue->enqueue([this, platform]()
                       {
                           platforms.push_back(platform);
                           platform->start();
                           multiplexer->add_watch(platform->dispatchable());
                       });
    }
    else
    {
        queue->enqueue([this, platform]()
                       {
                           platforms.push_back(platform);
                       });
    }

}

void mi::DefaultInputManager::start()
{
    auto expected = State::stopped;

    if (!state.compare_exchange_strong(expected, State::starting))
        return;

    auto reset_to_stopped_on_failure = on_unwind([this]{state = State::stopped;});

    multiplexer->add_watch(queue);
    auto unregister_queue = on_unwind([this]{multiplexer->remove_watch(queue);});

    multiplexer->add_watch(legacy_dispatchable);
    auto unregister_legacy_dispatchable = on_unwind([this]{multiplexer->remove_watch(legacy_dispatchable);});

    legacy_dispatchable->start();

    auto const started_promise = std::make_shared<std::promise<void>>();
    auto const weak_started_promise = std::weak_ptr<std::promise<void>>(started_promise);
    auto started_future = started_promise->get_future();

    queue->enqueue([this,weak_started_promise]()
                   {
                        start_platforms();
                        // TODO: Udev monitoring is still not separated yet - an initial scan is necessary to open
                        // devices, this will be triggered through the first call to dispatch->InputReader->loopOnce.
                        legacy_dispatchable->dispatch(dispatch::FdEvent::readable);
                        auto const started_promise =
                            std::shared_ptr<std::promise<void>>(weak_started_promise);
                        started_promise->set_value();
                   });

    input_thread = std::make_unique<dispatch::ThreadedDispatcher>(
        "Mir/Input Reader",
        multiplexer,
        [this,weak_started_promise]()
        {
            stop_platforms();
            multiplexer->remove_watch(queue);
            multiplexer->remove_watch(legacy_dispatchable);
            state = State::stopped;
            if (auto started_promise = weak_started_promise.lock())
                started_promise->set_exception(std::current_exception());
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

    auto reset_to_started_on_failure = on_unwind([this]{state = State::started;});

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
            queue->enqueue([this](){ start_platforms(); });
        });

    multiplexer->remove_watch(queue);
    auto register_queue = on_unwind([this]{multiplexer->add_watch(queue);});

    multiplexer->remove_watch(legacy_dispatchable);

    auto register_legacy_dispatchable= on_unwind([this]{multiplexer->add_watch(legacy_dispatchable);});

    input_thread.reset();

    state = State::stopped;
}

void mi::DefaultInputManager::start_platforms()
{
    for (auto const& platform : platforms)
    {
        platform->start();
        multiplexer->add_watch(platform->dispatchable());
    }
}

void mi::DefaultInputManager::stop_platforms()
{
    for (auto const& platform : platforms)
    {
        multiplexer->remove_watch(platform->dispatchable());
        platform->stop();
    }
}
