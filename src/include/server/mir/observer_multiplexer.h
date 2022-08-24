/*
 * Copyright © 2016 Canonical Ltd.
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

namespace mir
{
template<class Observer>
class ObserverMultiplexer : public ObserverRegistrar<Observer>, public Observer
{
public:
    void register_interest(std::weak_ptr<Observer> const& observer) override;
    void register_interest(
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
                if (state->pending_reset || !state->live_lock.owns_lock())
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
                    if (state->pending_reset && state->threads_in_use.empty() && state->live_lock.owns_lock())
                    {
                        // If we are currently resetting and there are no longer any observations in progress
                        state->live_lock.unlock();
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
                if (state->live_lock.owns_lock())
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
                        state->live_lock.unlock();
                    }
                    else
                    {
                        // Other thread(s) are making observations, so ask them to unlock live_lock when they're done
                        state->pending_reset = true;
                        state.drop();
                        // Wait for live_mutex to become available, which indicates all observatins have finished (we
                        // deliberately do not hold on to live_mutex)
                        std::lock_guard{live_mutex};
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

        /// Locked (by State::live_lock) until this observer is reset
        std::mutex live_mutex;

        struct State
        {
            State(std::mutex& live_mutex)
                : live_lock{live_mutex}
            {
            }

            /// All threads that are currently making observations. If a thread makes an observation from within an
            /// observation, it appears multiple times. Number of times an ID appears does matter, but order does not.
            std::vector<std::thread::id> threads_in_use;
            /// Locked until this observer is reset
            std::unique_lock<std::mutex> live_lock;
            /// Set to true when this observer is reset, but there are other in-progress observations. When the final
            /// observation completes live_lock will be unlocked. No new observations are started when pending_reset is
            /// true.
            bool pending_reset{false};
        };

        Synchronised<State> synchronised_state{live_mutex};
    };

    PosixRWMutex observer_mutex;
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
void ObserverMultiplexer<Observer>::unregister_interest(Observer const& observer)
{
    std::lock_guard lock{observer_mutex};
    observers.erase(
        std::remove_if(
            observers.begin(),
            observers.end(),
            [&observer](auto& candidate)
            {
                // This will wait for any (other) thread to finish with the candidate observer, then reset it
                // (preventing future notifications from being sent) if it is the same as the unregistered observer.
                return candidate->maybe_reset(&observer);
            }),
        observers.end());
}

template<class Observer>
auto ObserverMultiplexer<Observer>::empty() -> bool
{
    std::lock_guard lock{observer_mutex};
    return observers.empty();
}

template<class Observer>
template<typename MemberFn, typename... Args>
void ObserverMultiplexer<Observer>::for_each_observer(MemberFn f, Args&&... args)
{
    static_assert(
        std::is_member_function_pointer<MemberFn>::value,
        "f must be of type (Observer::*)(Args...), a pointer to an Observer member function.");
    decltype(observers) local_observers;
    {
        std::lock_guard lock{observer_mutex};
        local_observers = observers;
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
    decltype(observers) local_observers;
    {
        std::lock_guard lock{observer_mutex};
        local_observers = observers;
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
