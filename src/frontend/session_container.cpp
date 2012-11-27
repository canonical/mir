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

#include "mir/frontend/session_container.h"
#include "mir/frontend/session.h"

#include <memory>
#include <cassert>
#include <algorithm>


namespace mf = mir::frontend;

mf::SessionContainer::SessionContainer()
{

}

mf::SessionContainer::~SessionContainer()
{
}

void mf::SessionContainer::insert_session(std::shared_ptr<mf::Session> const& session)
{
    std::unique_lock<std::mutex> lk(guard);
    auto name = session->get_name();

    apps.push_back(session);
}

void mf::SessionContainer::remove_session(std::shared_ptr<mf::Session> const& session)
{
    std::unique_lock<std::mutex> lk(guard);

    auto it = std::find(apps.begin(), apps.end(), session);
    apps.erase(it);
}

void mf::SessionContainer::for_each(std::function<void(std::shared_ptr<Session> const&)> f) const
{
    std::unique_lock<std::mutex> lk(guard);

    for (auto const ptr : apps)
    {
        f(ptr);
    }
}


void mf::SessionContainer::lock()
{
    guard.lock();
}

void mf::SessionContainer::unlock()
{
    guard.unlock();
}
