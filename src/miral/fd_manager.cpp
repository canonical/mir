/*
 * Copyright © Canonical Ltd.
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

#include <boost/throw_exception.hpp>

using namespace miral;

FdManager::FdManager()
{
}

FdManager::~FdManager()
{
}

auto FdManager::register_handler(mir::Fd fd, std::function<void(int)> const& handler)
-> std::unique_ptr<FdHandle>
{
    std::lock_guard<std::mutex> lock{mutex};

    auto handle = std::make_unique<FdHandleImpl>(fd, shared_from_this());

    if (auto const main_loop = weak_main_loop.lock())
    {
        main_loop->register_fd_handler({fd}, handle.get(), handler);
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
    std::lock_guard<std::mutex> lock{mutex};

    if (auto const main_loop = weak_main_loop.lock())
    {
        main_loop->unregister_fd_handler(owner);
    }
    else
    {
        // Remove all entries with same owner from backlog
        backlog.erase(remove_if(begin(backlog), end(backlog), [&](auto& info){ return info.owner == owner; }), end(backlog));
    }
}

void FdManager::set_main_loop(std::shared_ptr<mir::MainLoop> main_loop)
{
    std::lock_guard<std::mutex> lock{mutex};

    for (auto const& handle : backlog)
    {
        main_loop->register_fd_handler({handle.fd}, handle.owner, handle.handler);
    }

    backlog.clear();

    weak_main_loop = main_loop;
}

miral::FdHandle::~FdHandle() = default;

FdHandleImpl::FdHandleImpl(mir::Fd fd, std::shared_ptr<FdManager> manager)
: fd{fd},
  manager{manager}
{
}

FdHandleImpl::~FdHandleImpl()
{
    manager->unregister_handler(this);
}
