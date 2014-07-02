/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#ifndef MIR_SCENE_OBSERVER_LIST_H_
#define MIR_SCENE_OBSERVER_LIST_H_

#include <algorithm>
#include <mutex>

namespace mir
{
namespace scene
{

/**
 * ObserverList allows:
 * 1. Removal of an observer guarantees the observer will not receive
 *    any more callbacks.
 * 2. Add/remove can be called from within the callback context
 * 3. for_each can be called from within the callback context
 * 4. for_each is re-entrant safe, so a callback can originate another for_each call
 */

template<typename ElementType>
class ObserverList
{
public:

    ObserverList()
        : dispatching{false}, snapshot_needs_update{false}
    {}

    void for_each(
        std::function<void(ElementType const&)> const& callback)
    {
        std::unique_lock<decltype(mutex)> lock(mutex);

        //Needed for reentrancy
        bool const was_already_dispatching = dispatching;
        if (!was_already_dispatching)
        {
            update_snapshot();
            dispatching = true;
        }

        for (auto const& observer : observers_snapshot)
        {
            //Don't check original list unnecessarily - only if it's been updated
            bool const notify = !snapshot_needs_update || orig_contains(observer);
            if (notify)
            {
                lock.unlock();
                callback(observer);
                lock.lock();
            }
        }

        if (!was_already_dispatching)
            dispatching = false;
    }

    void add(ElementType const& observer)
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        observers.push_back(observer);
        try_update_snapshot();
    }

    void remove(ElementType const& observer)
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        observers.erase(std::remove(observers.begin(),observers.end(), observer), observers.end());

        //If in middle of dispatching, the snapshot will be updated on the next add/remove/dispatch call
        //The dispatcher guarantees the removed observer will not be called anymore.
        try_update_snapshot();
    }

private:
    bool orig_contains(ElementType const& item)
    {
        auto const& end = observers.end();
        return std::find(observers.begin(), end, item) != end;
    }

    void update_snapshot()
    {
        if (snapshot_needs_update)
        {
            observers_snapshot = observers;
            snapshot_needs_update = false;
        }
    }

    void try_update_snapshot()
    {
        snapshot_needs_update = true;
        if (dispatching)
            return;
        else
            update_snapshot();
    }

    std::mutex mutex;
    bool dispatching;
    bool snapshot_needs_update;
    std::vector<ElementType> observers;
    std::vector<ElementType> observers_snapshot;
};

}
}

#endif
