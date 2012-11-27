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

#include "mir/frontend/the_session_container_implementation.h"
#include "mir/frontend/session.h"

#include <memory>
#include <cassert>
#include <algorithm>


namespace mf = mir::frontend;

mf::TheSessionContainerImplementation::TheSessionContainerImplementation()
{

}

void mf::TheSessionContainerImplementation::insert_session(std::shared_ptr<mf::Session> const& session)
{
    std::unique_lock<std::mutex> lk(guard);
    auto name = session->get_name();

    apps.push_back(session);
}

void mf::TheSessionContainerImplementation::remove_session(std::shared_ptr<mf::Session> const& session)
{
    std::unique_lock<std::mutex> lk(guard);

    auto it = std::find(apps.begin(), apps.end(), session);
    apps.erase(it);
}

void mf::TheSessionContainerImplementation::lock()
{
    guard.lock();
}

void mf::TheSessionContainerImplementation::unlock()
{
    guard.unlock();
}

mf::TheSessionContainerImplementation::LockingIterator::LockingIterator(TheSessionContainerImplementation *model, size_t index) : container(model),
                                                                                                              it(index)
{
  
}

void mf::TheSessionContainerImplementation::LockingIterator::advance()
{
    it += 1;
}

void mf::TheSessionContainerImplementation::LockingIterator::reset()
{
    it = 0;
}

bool mf::TheSessionContainerImplementation::LockingIterator::is_valid() const
{
    return it < container->apps.size();
}

const std::shared_ptr<mf::Session> mf::TheSessionContainerImplementation::LockingIterator::operator*()
{
    auto app = container->apps[it];
    return app;
}

mf::TheSessionContainerImplementation::LockingIterator::~LockingIterator()
{
    container->unlock();
}

std::shared_ptr<mf::SessionContainer::LockingIterator> mf::TheSessionContainerImplementation::iterator()
{
    lock();
  
    std::shared_ptr<mf::TheSessionContainerImplementation::LockingIterator> it(
        new mf::TheSessionContainerImplementation::LockingIterator(this, 0));
  
    return it;
}


  
