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

#ifndef MIR_FRONTEND_SERVICES_SESSION_CONTAINER_H_
#define MIR_FRONTEND_SERVICES_SESSION_CONTAINER_H_

#include <memory>

namespace mir
{
namespace frontend
{
class ApplicationSession;

namespace mf = mir::frontend;

class ApplicationSessionContainer
{
 public:
    virtual ~ApplicationSessionContainer() {}

    virtual void insert_session(std::shared_ptr<ApplicationSession> session) = 0;
    virtual void remove_session(std::shared_ptr<ApplicationSession> session) = 0;

    virtual void lock() = 0;
    virtual void unlock() = 0;

    class LockingIterator
    {
    public:
      virtual void advance() = 0;
      virtual bool is_valid() const = 0;
      virtual void reset() = 0;
      virtual const std::shared_ptr<ApplicationSession> operator*() = 0;
      virtual ~LockingIterator() {};
    protected:
      //      friend class ApplicationSessionContainer;

      LockingIterator() = default;
    };

    virtual std::shared_ptr<ApplicationSessionContainer::LockingIterator> iterator() = 0;


 protected:
    ApplicationSessionContainer() = default;
    ApplicationSessionContainer(const ApplicationSessionContainer&) = delete;
    ApplicationSessionContainer& operator=(const ApplicationSessionContainer&) = delete;
};

}
}


#endif // MIR_FRONTEND_SERVICES_SESSION_CONTAINER_H_
