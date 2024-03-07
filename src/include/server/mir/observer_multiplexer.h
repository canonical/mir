/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_OBSERVER_MULTIPLEXER_H_
#define MIR_OBSERVER_MULTIPLEXER_H_

#include "mir/observer_registrar.h"
#include "mir/posix_rw_mutex.h"
#include "mir/executor.h"
#include "mir/raii.h"
#include "mir/synchronised.h"

#include <vector>
#include <algorithm>
#include <mutex>
#include <thread>
#include <condition_variable>

namespace mir
{
/**
 * A threadsafe mechanism for keeping track of a set of observers and distributing notifications to them.
 *
 * Each time an observer is added, a WeakObserver is created for it. This WeakObserver may outlive the observer, and may
 * continue to exist after the observer has been removed and observations have ended. Each WeakObserver may delegate
 * observations to a different Executor, and each keeps track of when observations should stop being sent due to the
 * observer being removed.
 *
 * A WeakObserver may dispatch multiple observations at the same time, either on different threads or (in the case of
 * a blocking executor) if an observation is sent from within another observation.
 *
 * When an observer is removed a WeakObserver is marked as reset and removed from the observers list.
 * ObserverMultiplexer::unregister_interest() does not return until the related WeakObserver has been reset. This
 * happens once all in-flight observations have either completed, or are on threads that have removed the observer.
 */
template<class Observer>
class ObserverMultiplexer : public ObserverRegistrar<Observer>, public Observer
{
public:
    void register_interest(std::weak_ptr<Observer> const& observer) override;
    void register_interest(
        std::weak_ptr<Observer> const& observer,
        Executor& executor) override;
    void register_early_observer(
        std::weak_ptr<Observer> const& observer,
        Executor& executor) override;
    void unregister_interest(Observer const& observer) override;

    /// Returns true if there are no observers
    auto empty() -> bool;

protected:
    /**
     * \param [in] default_executor Executor that will be used as the execution environment
     *                                  for any observer that does not specify its own.
     * \note \p default_executor must outlive any observer.
     */
    explicit ObserverMultiplexer(Executor& default_executor)
        : default_executor{default_executor}
    {
    }

    /**
     *  Invoke a member function of Observer on each registered observer.
     *
     * \tparam MemberFn Must be (Observer::*)(Args...)
     * \tparam Args     Parameter pack of arguments of Observer member function.
     * \param f         Pointer to Observer member function to invoke.
     * \param args  Arguments for member function invocation.
     */
    template<typename MemberFn, typename... Args>
    void for_each_observer(MemberFn f, Args&&... args);

    /**
     *  Invoke a member function of a specific Observer (if and only if it is registered).
     *
     * \tparam MemberFn Must be (Observer::*)(Args...)
     * \tparam Args     Parameter pack of arguments of Observer member function.
     * \param observer  Reference to the observer the function should be invoked for.
     * \param f         Pointer to Observer member function to invoke.
     * \param args      Arguments for member function invocation.
     */
    template<typename MemberFn, typename... Args>
    void for_single_observer(Observer const& observer, MemberFn f, Args&&... args);
private:
    Executor& default_executor;

    class WeakObserver
    {
    public:
        explicit WeakObserver(std::weak_ptr<Observer> observer, Executor& executor)
            : executor{&executor},
              observer{observer}
        {
        }

        void spawn(std::function<void()>&& work)
        {
            // Executor only guaranteed to be alive as long as observer
            if (auto const live_observer = observer.lock())
            {
                executor->spawn(std::move(work));
            }
        }

        void spawn_if_eq(Observer const& candidate_observer, std::function<void()>&& work)
        {
            // Executor is only guaranteed to be live as long as observer
            auto const live_observer = observer.lock();
            if (live_observer.get() == &candidate_observer)
            {
                executor->spawn(std::move(work));
            }
        }

        template<typename MemberFn, typename... Args>
        void invoke(MemberFn f, Args&&... args)
        {
            auto const live_observer = observer.lock();
            if (!live_observer)
            {
                return;
            }
            {
                auto const state = synchronised_state.lock();
                // If this observer is being reset or has been reset no new observations should be made
                if (state->status != Status::active)
                {
                    return;
                }
                state->threads_in_use.push_back(std::this_thread::get_id());
            }
            raii::PairedCalls cleanup{[](){}, [this]()
                {
                    // This will run after the observation is made, even if the observer throws
                    auto const state = synchronised_state.lock();
                    // Remove only a single copy of this thread's ID from the vector (to match the single copy we
                    // pushed above)
                    auto const it = std::find(
                        begin(state->threads_in_use),
                        end(state->threads_in_use),
                        std::this_thread::get_id());
                    if (it != end(state->threads_in_use))
                    {
                        state->threads_in_use.erase(it);
                    }
                    if (state->status == Status::reset_pending && state->threads_in_use.empty())
                    {
                        state->status = Status::reset_complete;
                        reset_cv.notify_all();
                    }
                }};
            auto const invokable_mem_fn = std::mem_fn(f);
            invokable_mem_fn(live_observer.get(), std::forward<Args>(args)...);
        }

        /// Called when the given observer is unregistered. Returns true if the observer held by `this` is now reset
        /// (either because `this`s observer was unregistered_observer or `this`s observer has expired)
        auto maybe_reset(Observer const* const unregistered_observer) -> bool
        {
            auto const self = observer.lock().get();
            if (self == unregistered_observer)
            {
                // `this` holds the unregistered observer
                auto state = synchronised_state.lock();
                if (state->status != Status::reset_complete)
                {
                    // Remove *all* instances of the current thread from threads_in_use. This means even if several
                    // observations are made recursively, the observer can still be removed from within an observation.
                    state->threads_in_use.erase(
                        std::remove(
                            begin(state->threads_in_use),
                            end(state->threads_in_use),
                            std::this_thread::get_id()),
                        end(state->threads_in_use));
                    if (state->threads_in_use.empty())
                    {
                        // If no other threads are making observations we can safely mark this observer as reset
                        state->status = Status::reset_complete;
                        reset_cv.notify_all();
                    }
                    else
                    {
                        // Other thread(s) are making observations, so set state to reset_pending. This asks them to set
                        // state to reset and notify the condition variable when they are done.
                        state->status = Status::reset_pending;
                        // Wait for the reset to complete
                        state.wait(reset_cv, [&]()
                            {
                                return state->status == Status::reset_complete;
                            });
                    }
                }
                return true;
            }
            else
            {
                // return true if our observer has expired
                return self == nullptr;
            }
        }
    private:
        /// Only guaranteed to be alive while the observer is live. All observations should be run
        /// through this executor.
        Executor* executor;

        std::weak_ptr<Observer> const observer;

        enum class Status
        {
            /// Can receive observations.
            active,
            /// Used when a reset has been requested, but observations are still in progress. New observations will not
            /// be sent. Reset will complete when all observations have ended.
            reset_pending,
            /// No observations are in progress and no new observations will be sent.
            reset_complete,
        };

        struct State
        {
            /// Starts as active. Changes to reset_pending (if needed) and then finally to reset_complete. Never goes
            /// backwards.
            Status status{Status::active};

            /// All threads that are currently making observations. If a thread makes an observation from within an
            /// observation, it appears multiple times. Number of times an ID appears does matter, but order does not.
            std::vector<std::thread::id> threads_in_use;
        };

        Synchronised<State> synchronised_state;

        /// Notified when State::status changes to reset_complete
        std::condition_variable reset_cv;
    };

    PosixRWMutex observer_mutex;
    std::vector<std::shared_ptr<WeakObserver>> early_observers;
    std::vector<std::shared_ptr<WeakObserver>> observers;
};

template<class Observer>
void ObserverMultiplexer<Observer>::register_interest(std::weak_ptr<Observer> const& observer)
{
    register_interest(observer, default_executor);
}

template<class Observer>
void ObserverMultiplexer<Observer>::register_interest(
    std::weak_ptr<Observer> const& observer,
    Executor& executor)
{
    std::lock_guard lock{observer_mutex};

    observers.emplace_back(std::make_shared<WeakObserver>(observer, executor));
}

template<class Observer>
void ObserverMultiplexer<Observer>::register_early_observer(
    std::weak_ptr<Observer> const& observer,
    Executor& executor)
{
    std::lock_guard lock{observer_mutex};

    early_observers.emplace_back(std::make_shared<WeakObserver>(observer, executor));
}

template<class Observer>
void ObserverMultiplexer<Observer>::unregister_interest(Observer const& observer)
{
    std::lock_guard lock{observer_mutex};
    auto remove_observer{
        [&observer](auto& candidate)
        {
            // This will wait for any (other) thread to finish with the candidate observer, then reset it
            // (preventing future notifications from being sent) if it is the same as the unregistered observer.
            return candidate->maybe_reset(&observer);
        }};
    std::erase_if(early_observers, remove_observer);
    std::erase_if(observers, remove_observer);
}

template<class Observer>
auto ObserverMultiplexer<Observer>::empty() -> bool
{
    std::lock_guard lock{observer_mutex};
    return early_observers.empty() && observers.empty();
}

template<class Observer>
template<typename MemberFn, typename... Args>
void ObserverMultiplexer<Observer>::for_each_observer(MemberFn f, Args&&... args)
{
    static_assert(
        std::is_member_function_pointer<MemberFn>::value,
        "f must be of type (Observer::*)(Args...), a pointer to an Observer member function.");
    decltype(observers) local_early_observers;
    decltype(observers) local_observers;
    {
        std::lock_guard lock{observer_mutex};
        local_early_observers = early_observers;
        local_observers = observers;
    }
    for (auto& weak_observer: local_early_observers)
    {
        weak_observer->spawn(
            [f, weak_observer = std::move(weak_observer), args...]() mutable
            {
                weak_observer->invoke(f, std::forward<Args>(args)...);
            });
    }
    for (auto& weak_observer: local_observers)
    {
        weak_observer->spawn(
            [f, weak_observer = std::move(weak_observer), args...]() mutable
            {
                weak_observer->invoke(f, std::forward<Args>(args)...);
            });
    }
}

template<class Observer>
template<typename MemberFn, typename... Args>
void ObserverMultiplexer<Observer>::for_single_observer(Observer const& target_observer, MemberFn f, Args&&... args)
{
    static_assert(
        std::is_member_function_pointer<MemberFn>::value,
        "f must be of type (Observer::*)(Args...), a pointer to an Observer member function.");
    decltype(observers) local_early_observers;
    decltype(observers) local_observers;
    {
        std::lock_guard lock{observer_mutex};
        local_early_observers = early_observers;
        local_observers = observers;
    }
    for (auto& weak_observer: local_early_observers)
    {
        weak_observer->spawn_if_eq(target_observer,
            [f, weak_observer = std::move(weak_observer), args...]() mutable
            {
                weak_observer->invoke(f, std::forward<Args>(args)...);
            });
    }
    for (auto& weak_observer: local_observers)
    {
        weak_observer->spawn_if_eq(target_observer,
            [f, weak_observer = std::move(weak_observer), args...]() mutable
            {
                weak_observer->invoke(f, std::forward<Args>(args)...);
            });
    }
}
}

#endif // MIR_OBSERVER_MULTIPLEXER_H_