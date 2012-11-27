/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef FRONTEND_SESSION_CONTAINER_H_
#define FRONTEND_SESSION_CONTAINER_H_

#include "mir/thread/all.h"
#include <vector>
#include <memory>

namespace mir
{
namespace frontend
{
class Session;

class SessionContainer
{
public:
    SessionContainer();
    ~SessionContainer();

    virtual void insert_session(std::shared_ptr<Session> const& session);
    virtual void remove_session(std::shared_ptr<Session> const& session);

    void lock();
    void unlock();

    class LockingIterator
    {
    public:
        void advance();
        bool is_valid() const;
        void reset();
        const std::shared_ptr<Session> operator*();
        ~LockingIterator();
    protected:
        LockingIterator(SessionContainer* container,
                        size_t index);
        friend class SessionContainer;
        LockingIterator() = delete;
        LockingIterator(LockingIterator const&LockingIterator) = delete;
    private:
      SessionContainer* container;
      size_t it;
    };

    std::shared_ptr<SessionContainer::LockingIterator> iterator();


protected:
    SessionContainer(const SessionContainer&) = delete;
    SessionContainer& operator=(const SessionContainer&) = delete;

private:
    std::vector<std::shared_ptr<Session>> apps;
    std::mutex guard;
};

}
}


#endif // FRONTEND_SESSION_CONTAINER_H_
