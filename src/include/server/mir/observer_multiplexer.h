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

#include <vector>
#include <algorithm>
#include <atomic>
#include <thread>
#include <unordered_map>
#include <condition_variable>
#include <mutex>
#include <shared_mutex>

namespace
{
template<typename T>
class AtomicWeakPtr
{
public:
    AtomicWeakPtr(std::weak_ptr<T> const& target)
        : state{std::make_shared<SharedState>(target)}
    {
    }

    static thread_local bool thread_owns;

    std::shared_ptr<T> lock()
    {
        if (!state)
        {
            return nullptr;
        }
        ++state->refcnt;
        auto locked_target = state->target.load()->lock();
        state->holder = locked_target;

        if (!locked_target)
        {
            --state->refcnt;
            return locked_target;
        }

        return std::shared_ptr<T>{
            state->holder.get(),
            [shared_state = state](T*)
            {
                auto previous_count = shared_state->refcnt--;
                if (previous_count == 2)
                {
                    // We were holding the last live reference, so unref the target shared_ptr.
                    shared_state->holder.reset();
                }
                if (previous_count == 1)
                {
                    // We were waiting to die; signal the waiting thread
                    shared_state->deleted.notify_all();
                }
            }};
    }

    void kill()
    {
        auto previous_target = state->target.exchange(new std::weak_ptr<T>{});
        delete previous_target;

        int new_count = --state->refcnt;
        if (new_count == 0 || ((new_count == 1 && thread_owns)))
        {
            // No one else holds us, we're done.
            return;
        }
        else
        {
            std::unique_lock<std::mutex> lock{state->deleted_mutex};
            state->deleted.wait(lock, [this]() { return state->refcnt == 0; });
        }
    }
private:
    struct SharedState
    {
        SharedState(std::weak_ptr<T> const& target)
            : target{new std::weak_ptr<T>{target}},
              refcnt{1}
        {
        }
        ~SharedState()
        {
            delete target;
        }

        std::atomic<std::weak_ptr<T>*> target;
        std::shared_ptr<T> holder;

        std::atomic<int> refcnt;

        std::mutex deleted_mutex;
        std::condition_variable deleted;
    };

    std::shared_ptr<SharedState> state;
};

template<typename T>
thread_local bool AtomicWeakPtr<T>::thread_owns = false;

}

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
    std::shared_timed_mutex observer_mutex;
    std::vector<AtomicWeakPtr<Observer>> observers;

    std::mutex addition_list_mutex;
    std::vector<std::weak_ptr<Observer>> addition_list;

    void process_addition_list();
    void process_removals();
};

template<class Observer>
void ObserverMultiplexer<Observer>::register_interest(std::weak_ptr<Observer> const& observer)
{
    std::lock_guard<std::mutex> lock{addition_list_mutex};
    addition_list.push_back(observer);
}

template<class Observer>
void ObserverMultiplexer<Observer>::unregister_interest(Observer const& observer)
{
    {
        std::lock_guard<std::mutex> lock{addition_list_mutex};
        addition_list.erase(
            std::remove_if(
                addition_list.begin(),
                addition_list.end(),
                [&observer](auto candidate)
                {
                    // Remove any deleted
                    if (candidate.lock().get() == &observer)
                        std::cout << "Hello" <<std::endl;

                    return candidate.lock().get() == &observer;
                }),
            addition_list.end());
    }
    std::shared_lock<decltype(observer_mutex)> lock{observer_mutex};
    for (auto& candidate_observer : observers)
    {
        std::cout << "Hello!" << std::endl;
        if (candidate_observer.lock().get() == &observer)
        {
            std::cout<< "Imma kill you " << std::endl;
            candidate_observer.kill();
        }
    }
}

template<class Observer>
void ObserverMultiplexer<Observer>::process_addition_list()
{
    std::lock_guard<std::mutex> adder_lock{addition_list_mutex};
    std::unique_lock<std::shared_timed_mutex> observer_lock{observer_mutex};
    for (auto const& addition : addition_list)
    {
        observers.push_back(addition);
    }
    addition_list.clear();
}

template<class Observer>
void ObserverMultiplexer<Observer>::process_removals()
{
    std::unique_lock<std::shared_timed_mutex> observer_lock{observer_mutex};
    observers.erase(
        std::remove_if(
            observers.begin(),
            observers.end(),
            [](auto candidate)
            {
                // Remove any deleted
                return candidate.lock() == nullptr;
            }),
        observers.end());
}

template<class Observer>
template<typename Callable>
void ObserverMultiplexer<Observer>::for_each_observer(Callable&& f)
{
    process_addition_list();
    process_removals();

    {
        std::shared_lock<std::shared_timed_mutex> lock{observer_mutex};
        for (auto& maybe_observer: observers)
        {
            if (auto observer = maybe_observer.lock())
            {
                maybe_observer.thread_owns = true;
                f(*observer);
                maybe_observer.thread_owns = false;
            }
        }
    }
}
}


#endif //MIR_OBSERVER_MULTIPLEXER_H_
