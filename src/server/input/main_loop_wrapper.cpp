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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */


#include "main_loop_wrapper.h"

#include "mir/main_loop.h"

namespace mi = mir::input;

mi::MainLoopWrapper::MainLoopWrapper(std::shared_ptr<MainLoop> const& loop)
    : loop(loop)
{
}

void mi::MainLoopWrapper::register_fd_handler(std::initializer_list<int> fds,
        void const* owner, std::function<void(int)> const&& handler)
{
    loop->register_fd_handler(fds,owner,std::move(handler));
}

void mi::MainLoopWrapper::unregister_fd_handler(void const* owner)
{
    loop->unregister_fd_handler(owner);
}


void mi::MainLoopWrapper::register_handler(std::function<void()> const&& handler)
{
    loop->enqueue(this, std::move(handler));
}

