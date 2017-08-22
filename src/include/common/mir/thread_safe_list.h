/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
                Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_THREAD_SAFE_LIST_H_
#define MIR_THREAD_SAFE_LIST_H_

#include "mir/recursive_read_write_mutex.h"

#include <atomic>
#include <functional>

namespace mir
{

/*
 * Requirements for type 'Element'
 *  - for_each():
 *    - copy-assignable
 *  - add():
 *    - copy-assignable
 *    - conversion to bool: indicates whether this is a valid element
 *  - remove(), remove_all():
 *    - copy-assignable
 *    - Element{}: default construction should create an invalid element
 *    - bool operator==: equality of elements
 *    - bool operator!=: inequality of elements
 *  - clear():
 *    - copy-assignable
 *    - Element{}: value initialization should create an invalid element
 */

template<class Element>
class ThreadSafeList
{
public:
    void add(Element const& element);
    void remove(Element const& element);
    unsigned int remove_all(Element const& element);
    void clear();
    void for_each(std::function<void(Element const& element)> const& f);

private:
    struct ListItem
    {
        ListItem() {}
        RecursiveReadWriteMutex mutex;
        Element element{};
        std::atomic<ListItem*> next{nullptr};

        ~ListItem() { delete next.load(); }
    } head;
};

template<class Element>
void ThreadSafeList<Element>::for_each(
    std::function<void(Element const& element)> const& f)
{
    ListItem* current_item = &head;

    while (current_item)
    {
        RecursiveReadLock lock{current_item->mutex};

        // We need to take a copy in case we recursively remove during call
        if (auto const copy_of_element = current_item->element) f(copy_of_element);

        current_item = current_item->next;
    }
}

template<class Element>
void ThreadSafeList<Element>::add(Element const& element)
{
    ListItem* current_item = &head;

    do
    {
        // Note: we release the read lock to avoid two threads calling add at
        // the same time mutually blocking the other's upgrade to write lock.
        {
            RecursiveReadLock lock{current_item->mutex};
            if (current_item->element) continue;
        }

        RecursiveWriteLock lock{current_item->mutex};

        if (!current_item->element)
        {
            current_item->element = element;
            return;
        }
    }
    while (current_item->next && (current_item = current_item->next));

    // No empty Items so append a new one
    auto new_item = new ListItem;
    new_item->element = element;

    for (ListItem* expected{nullptr};
        !current_item->next.compare_exchange_weak(expected, new_item);
        expected = nullptr)
    {
        if (expected) current_item = expected;
    }
}

template<class Element>
void ThreadSafeList<Element>::remove(Element const& element)
{
    ListItem* current_item = &head;

    do
    {
        {
            RecursiveReadLock lock{current_item->mutex};
            if (current_item->element != element) continue;
        }

        RecursiveWriteLock lock{current_item->mutex};

        if (current_item->element == element)
        {
            current_item->element = Element{};
            return;
        }
    }
    while ((current_item = current_item->next));
}

template<class Element>
unsigned int ThreadSafeList<Element>::remove_all(Element const& element)
{
    ListItem* current_item = &head;
    auto removed = 0u;

    do
    {
        {
            RecursiveReadLock lock{current_item->mutex};
            if (current_item->element != element) continue;
        }

        RecursiveWriteLock lock{current_item->mutex};

        if (current_item->element == element)
        {
            current_item->element = Element{};
            ++removed;
        }
    }
    while ((current_item = current_item->next));

    return removed;
}

template<class Element>
void ThreadSafeList<Element>::clear()
{
    ListItem* current_item = &head;

    do
    {
        RecursiveWriteLock lock{current_item->mutex};
        current_item->element = Element{};
    }
    while ((current_item = current_item->next));
}

}

#endif /* MIR_THREAD_SAFE_LIST_H_ */
