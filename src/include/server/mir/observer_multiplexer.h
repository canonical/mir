/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_OBSERVER_MULTIPLEXER_H_
#define MIR_OBSERVER_MULTIPLEXER_H_

#include "mir/observer_registrar.h"
#include "mir/raii.h"
#include "mir/posix_rw_mutex.h"

#include <vector>
#include <algorithm>
#include <mutex>
#include <thread>
#include <shared_mutex>
#include <unordered_map>

namespace mir
{
template<class Observer>
class ObserverMultiplexer : public ObserverRegistrar<Observer>, public Observer
{
public:
    void register_interest(std::weak_ptr<Observer> const& observer) override;
    void unregister_interest(Observer const& observer) override;

protected:
    ObserverMultiplexer() = default;

    /*
     * In the magical future compile-time reflection will allow us to remove this and
     * automatically implement Observer.
     *
     * For now, provide an iterator for subclasses to use when implementing Observer.
     */
    template<typename Callable>
    void for_each_observer(Callable&& f);
private:
    struct Observable
    {
        Observable(std::weak_ptr<Observer> const& target)
            : target{target}
        {
        }

        Observable(Observable&& from)
            : target{std::move(from.target)}
        {
        }
        Observable& operator=(Observable&& from)
        {
            target = std::move(from.target);
            return *this;
        }

        std::recursive_mutex target_mutex;
        std::weak_ptr<Observer> target;
    };

    PosixRWMutex observer_mutex{PosixRWMutex::Type::PreferWriterNonRecursive};
    std::vector<Observable> observers;

    std::mutex recurse_mutex;
    std::unordered_map<std::thread::id, int> recursing_threads;
    bool is_recursive_call();

    std::mutex addition_list_mutex;
    std::vector<std::weak_ptr<Observer>> addition_list;
};

template<class Observer>
bool ObserverMultiplexer<Observer>::is_recursive_call()
{
    std::lock_guard<decltype(recurse_mutex)> lock{recurse_mutex};
    return recursing_threads.count(std::this_thread::get_id()) > 0;
}


template<class Observer>
void ObserverMultiplexer<Observer>::register_interest(std::weak_ptr<Observer> const& observer)
{
    if (is_recursive_call())
    {
        std::lock_guard<std::mutex> lock{addition_list_mutex};
        addition_list.push_back(observer);
    }
    else
    {
        std::unique_lock<decltype(observer_mutex)> lock{observer_mutex};
        observers.push_back(observer);
    }
}

template<class Observer>
void ObserverMultiplexer<Observer>::unregister_interest(Observer const& observer)
{
    {
        // First remove all matches from the pending additions list
        std::lock_guard<std::mutex> lock{addition_list_mutex};
        addition_list.erase(
            std::remove_if(
                addition_list.begin(),
                addition_list.end(),
                [&observer](auto const& candidate)
                {
                    return candidate.lock().get() == &observer;
                }),
            addition_list.end());
    }

    if (is_recursive_call())
    {
        // We know that the observers vector is already locked.
        for (auto& candidate_observer : observers)
        {
            std::lock_guard<decltype(candidate_observer.target_mutex)> lock{candidate_observer.target_mutex};
            if (candidate_observer.target.lock().get() == &observer)
            {
                candidate_observer.target = std::weak_ptr<Observer>{};
            }
        }
    }
    else
    {
        std::unique_lock<decltype(observer_mutex)> lock{observer_mutex};
        observers.erase(
            std::remove_if(
                observers.begin(),
                observers.end(),
                [&observer](auto& candidate)
                {
                    std::lock_guard<decltype(candidate.target_mutex)> lock{candidate.target_mutex};
                    auto resolved_candidate = candidate.target.lock().get();
                    return (resolved_candidate == nullptr) || (resolved_candidate == &observer);
                }),
            observers.end());
    }
}

template<class Observer>
template<typename Callable>
void ObserverMultiplexer<Observer>::for_each_observer(Callable&& f)
{
    if (!is_recursive_call())
    {
        std::unique_lock<decltype(observer_mutex)> lock{observer_mutex};
        std::lock_guard<std::mutex> adder_lock{addition_list_mutex};
        for (auto const& addition : addition_list)
        {
            observers.push_back(addition);
        }
        addition_list.clear();
    }

    {
        auto recurse_guard = mir::raii::paired_calls(
            [this]()
            {
                std::lock_guard<decltype(recurse_mutex)> lock{recurse_mutex};
                // If this thread is not already in the map, operator[] will default-initialise to 0.
                ++recursing_threads[std::this_thread::get_id()];
            },
            [this]()
            {
                std::lock_guard<decltype(recurse_mutex)> lock{recurse_mutex};
                if(--recursing_threads[std::this_thread::get_id()] == 0)
                {
                    recursing_threads.erase(std::this_thread::get_id());
                }
            });

        std::shared_lock<decltype(observer_mutex)> lock{observer_mutex};
        for (auto& maybe_observer: observers)
        {
            std::lock_guard<decltype(maybe_observer.target_mutex)> lock{maybe_observer.target_mutex};
            if (auto observer = maybe_observer.target.lock())
            {
                f(*observer);
            }
        }
    }
}
}


#endif //MIR_OBSERVER_MULTIPLEXER_H_
