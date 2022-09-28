/*
 * Copyright © 2022 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "fd_manager.h"
#include "mir/main_loop.h"
#include "mir/server.h"

#include <boost/throw_exception.hpp>

using namespace miral;

FdManager::FdManager(std::weak_ptr<mir::Server> weak_server)
: weak_server{weak_server}
{
}

FdManager::~FdManager()
{
}

auto FdManager::register_handler(mir::Fd fd, std::function<void(int)> const& handler)
-> std::unique_ptr<FdHandle>
{
    auto handle = std::make_unique<FdHandle>(shared_from_this(), fd, handler);

    if (auto const server = weak_server.lock().get())
    {
        auto const main_loop = server->the_main_loop().get();
        main_loop->register_fd_handler({fd}, &handle, handler);
    }
    else
    {
        auto fd_info = FdInfo{fd, handle.get(), handler};
        backlog.push_back(fd_info);
    }

    return handle;
}

void FdManager::unregister_handler(void const* owner)
{
    if (auto const server = weak_server.lock().get())
    {
        auto const main_loop = server->the_main_loop().get();
        main_loop->unregister_fd_handler(owner);
    }
    else
    {
        // Remove all entries with same owner from backlog
        backlog.erase(remove_if(begin(backlog), end(backlog), [&](auto& info){ return info.owner == owner; }), end(backlog));
    }
}

void FdManager::process_backlog()
{
    if (auto const server = weak_server.lock().get())
    {
        auto const main_loop = server->the_main_loop().get();
        
        for (auto const& handle : backlog)
        {
            main_loop->register_fd_handler({handle.fd}, handle.owner, handle.handler);
        }

        backlog.clear();
    }
    else
    {
        // This should never be hit
        BOOST_THROW_EXCEPTION(std::runtime_error{"File descriptor backlog processed before Server started"});
    }
}

FdHandle::FdHandle(std::shared_ptr<FdManager> manager, mir::Fd fd, std::function<void(int)> const& handler)
: manager{manager},
  fd{fd},
  handler{handler}
{
}

FdHandle::~FdHandle()
{
    manager->unregister_handler(this);
}
