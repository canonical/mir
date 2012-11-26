/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored By: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_FRONTEND_APPLICATION_MODEL_H_
#define MIR_FRONTEND_APPLICATION_MODEL_H_

#include "mir/frontend/session_container.h"
#include "mir/thread/all.h"

#include <memory>
#include <set>
#include <map>
#include <vector>

namespace mir
{

namespace frontend
{

class Session;

class TheSessionContainerImplementation : public SessionContainer
{
public:
    explicit TheSessionContainerImplementation();
    virtual ~TheSessionContainerImplementation() {}
    
    void insert_session(std::shared_ptr<Session> const& session);
    void remove_session(std::shared_ptr<Session> const& session);
    
    void lock();
    void unlock();

    class LockingIterator : public SessionContainer::LockingIterator
    {
    public:
        void advance();
        bool is_valid() const;
        void reset();
        const std::shared_ptr<Session> operator*();
        virtual ~LockingIterator();
    protected:
        friend class TheSessionContainerImplementation;
      
        LockingIterator(TheSessionContainerImplementation *model,
                        size_t index);

      
    private:
      TheSessionContainerImplementation *model;
      size_t it;
	
    };

    std::shared_ptr<SessionContainer::LockingIterator> iterator();
    

protected:
    TheSessionContainerImplementation(const TheSessionContainerImplementation&) = delete;
    TheSessionContainerImplementation& operator=(const TheSessionContainerImplementation&) = delete;

private:
    std::vector<std::shared_ptr<Session>> apps;
    std::mutex guard;
};

}
}

#endif // MIR_FRONTEND_APPLICATION_MODEL_H_
