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
            : target{new std::weak_ptr<Observer>{target}}
        {
        }
        ~Observable()
        {
            delete target.load();
        }

        Observable(Observable&& from)
            : target{from.target.exchange(nullptr)}
        {
        }
        /*
         * This is not threadsafe, but that's fine. This is only ever called
         * while an exclusive lock is held on observer_mutex.
         */
        Observable& operator=(Observable&& from)
        {
            delete target.load();
            target = from.target.load();
            from.target = new std::weak_ptr<Observer>{};
            return *this;
        }

        void reset()
        {
            delete target.exchange(new std::weak_ptr<Observer>{});
        }

        std::shared_ptr<Observer> lock()
        {
            return target.load()->lock();
        }

        std::atomic<std::weak_ptr<Observer>*> target;
    };

    PosixRWMutex observer_mutex;
    std::vector<Observable> observers;

    std::mutex addition_list_mutex;
    std::vector<std::weak_ptr<Observer>> addition_list;
};

template<class Observer>
void ObserverMultiplexer<Observer>::register_interest(std::weak_ptr<Observer> const& observer)
{
    if (auto l = std::unique_lock<decltype(observer_mutex)>{observer_mutex, std::try_to_lock})
    {
        observers.push_back(observer);
    }
    else
    {
        std::lock_guard<std::mutex> lock{addition_list_mutex};
        addition_list.push_back(observer);
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

    if (auto l = std::unique_lock<decltype(observer_mutex)>{observer_mutex, std::try_to_lock})
    {
        observers.erase(
            std::remove_if(
                observers.begin(),
                observers.end(),
                [&observer](auto& candidate)
                {
                    auto resolved_candidate = candidate.lock().get();
                    return (resolved_candidate == nullptr) || (resolved_candidate == &observer);
                }),
            observers.end());
    }
    else
    {
        std::shared_lock<decltype(observer_mutex)> lock{observer_mutex};
        for (auto& candidate_observer : observers)
        {
            if (candidate_observer.lock().get() == &observer)
            {
                candidate_observer.reset();
            }
        }
    }
}

template<class Observer>
template<typename Callable>
void ObserverMultiplexer<Observer>::for_each_observer(Callable&& f)
{
    if (auto observer_lock = std::unique_lock<decltype(observer_mutex)>{observer_mutex, std::try_to_lock})
    {
        std::lock_guard<std::mutex> adder_lock{addition_list_mutex};
        for (auto const& addition : addition_list)
        {
            observers.push_back(addition);
        }
        addition_list.clear();
    }

    {
        std::shared_lock<decltype(observer_mutex)> lock{observer_mutex};
        for (auto& maybe_observer: observers)
        {
            if (auto observer = maybe_observer.lock())
            {
                f(*observer);
            }
        }
    }
}
}


#endif //MIR_OBSERVER_MULTIPLEXER_H_
