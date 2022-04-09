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
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_OBSERVER_MULTIPLEXER_H_
#define MIR_OBSERVER_MULTIPLEXER_H_

#include "mir/observer_registrar.h"
#include "mir/posix_rw_mutex.h"
#include "mir/executor.h"

#include <vector>
#include <algorithm>
#include <mutex>

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

        /// A handle that excludes attempts to unregister or destroy the Observer
        class LockedObserver
        {
        public:
            LockedObserver(
                std::shared_ptr<Observer>&& observer,
                std::unique_lock<std::recursive_mutex>&& lock)
                : lock{std::move(lock)},
                  observer{std::move(observer)}
            {
            }

            Observer& operator*() const
            {
                return *observer;
            }

            operator bool() const
            {
                return static_cast<bool>(observer);
            }

        private:
            std::unique_lock<std::recursive_mutex> const lock;
            std::shared_ptr<Observer> const observer;
        };

        LockedObserver lock()
        {
            std::unique_lock lock{expired_mutex};
            auto live_observer = observer.lock();
            if (live_observer && !expired)
            {
                return LockedObserver{std::move(live_observer), std::move(lock)};
            }
            else
            {
                return LockedObserver{nullptr, {}};
            }
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

        /// Called when the given observer is unregistered. Returns true if this observer is now invalid (either because
        /// this held the unregistered observer or this->observer has expired)
        auto maybe_reset(Observer const* const unregistered_observer) -> bool
        {
            auto const self = observer.lock().get();
            if (self == unregistered_observer)
            {
                std::lock_guard<std::recursive_mutex> run_lock{expired_mutex};
                expired = true;
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

        /// mutex-guarded boolean to signal when this WeakObserver has been unregistered,
        /// is not currently dispatching an observation and will not dispatch any future
        /// observations.
        std::recursive_mutex expired_mutex;
        bool expired{false};
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
    std::lock_guard<decltype(observer_mutex)> lock{observer_mutex};

    observers.emplace_back(std::make_shared<WeakObserver>(observer, executor));
}

template<class Observer>
void ObserverMultiplexer<Observer>::unregister_interest(Observer const& observer)
{
    std::lock_guard<decltype(observer_mutex)> lock{observer_mutex};
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
    std::lock_guard<decltype(observer_mutex)> lock{observer_mutex};
    return observers.empty();
}

template<class Observer>
template<typename MemberFn, typename... Args>
void ObserverMultiplexer<Observer>::for_each_observer(MemberFn f, Args&&... args)
{
    static_assert(
        std::is_member_function_pointer<MemberFn>::value,
        "f must be of type (Observer::*)(Args...), a pointer to an Observer member function.");
    auto const invokable_mem_fn = std::mem_fn(f);
    decltype(observers) local_observers;
    {
        std::lock_guard<decltype(observer_mutex)> lock{observer_mutex};
        local_observers = observers;
    }
    for (auto& weak_observer: local_observers)
    {
        weak_observer->spawn(
            [invokable_mem_fn, weak_observer = std::move(weak_observer), args...]() mutable
            {
                if (auto observer = weak_observer->lock())
                {
                    invokable_mem_fn(observer, std::forward<Args>(args)...);
                }
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
    auto const invokable_mem_fn = std::mem_fn(f);
    decltype(observers) local_observers;
    {
        std::lock_guard<decltype(observer_mutex)> lock{observer_mutex};
        local_observers = observers;
    }
    for (auto& weak_observer: local_observers)
    {
        weak_observer->spawn_if_eq(target_observer,
            [invokable_mem_fn, weak_observer = std::move(weak_observer), args...]() mutable
            {
                if (auto observer = weak_observer->lock())
                {
                    invokable_mem_fn(observer, std::forward<Args>(args)...);
                }
            });
    }
}
}

#endif // MIR_OBSERVER_MULTIPLEXER_H_
