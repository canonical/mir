/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_BASIC_OBSERVERS_H_
#define MIR_BASIC_OBSERVERS_H_

#include "mir/recursive_read_write_mutex.h"

#include <atomic>
#include <memory>

namespace mir
{
template<class Observer>
class BasicObservers
{
protected:
    void add(std::shared_ptr<Observer> const& observer);
    void remove(std::shared_ptr<Observer> const& observer);
    void for_each(std::function<void(std::shared_ptr<Observer> const& observer)> const& f);

private:
    struct ListItem
    {
        ListItem() {}
        RecursiveReadWriteMutex mutex;
        std::shared_ptr<Observer> observer;
        std::atomic<ListItem*> next{nullptr};

        ~ListItem() { delete next.load(); }
    } head;
};

template<class Observer>
void BasicObservers<Observer>::for_each(
    std::function<void(std::shared_ptr<Observer> const& observer)> const& f)
{
    ListItem* current_item = &head;

    while (current_item)
    {
        RecursiveReadLock lock{current_item->mutex};

        // We need to take a copy in case we recursively remove during call
        if (auto const copy_of_observer = current_item->observer) f(copy_of_observer);

        current_item = current_item->next;
    }
}

template<class Observer>
void BasicObservers<Observer>::add(std::shared_ptr<Observer> const& observer)
{
    ListItem* current_item = &head;

    do
    {
        // Note: we release the read lock to avoid two threads calling add at
        // the same time mutually blocking the other's upgrade to write lock.
        {
            RecursiveReadLock lock{current_item->mutex};
            if (current_item->observer) continue;
        }

        RecursiveWriteLock lock{current_item->mutex};

        if (!current_item->observer)
        {
            current_item->observer = observer;
            return;
        }
    }
    while (current_item->next && (current_item = current_item->next));

    // No empty Items so append a new one
    auto new_item = new ListItem;
    new_item->observer = observer;

    for (ListItem* expected{nullptr};
        !current_item->next.compare_exchange_weak(expected, new_item);
        expected = nullptr)
    {
        if (expected) current_item = expected;
    }
}

template<class Observer>
void BasicObservers<Observer>::remove(std::shared_ptr<Observer> const& observer)
{
    ListItem* current_item = &head;

    do
    {
        {
            RecursiveReadLock lock{current_item->mutex};
            if (current_item->observer != observer) continue;
        }

        RecursiveWriteLock lock{current_item->mutex};

        if (current_item->observer == observer)
        {
            current_item->observer.reset();
            return;
        }
    }
    while ((current_item = current_item->next));
}
}

#endif /* MIR_BASIC_OBSERVERS_H_ */
