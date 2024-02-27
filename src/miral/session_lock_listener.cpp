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

#include "miral/session_lock_listener.h"

#include <mir/server.h>
#include <mir/scene/session_lock.h>


class miral::SessionLockListener::Impl : public mir::scene::SessionLockObserver
{
public:
    Impl(Callback const& on_lock, Callback const& on_unlock)
        : on_lock_{on_lock},
          on_unlock_{on_unlock}
    {
    }

    void on_lock() override
    {
        on_lock_();
    }

    void on_unlock() override
    {
        on_unlock_();
    }


    Callback const on_lock_;
    Callback const on_unlock_;
};

miral::SessionLockListener::SessionLockListener(Callback const& on_lock, Callback const& on_unlock)
    : self{std::make_shared<Impl>(on_lock, on_unlock)}
{
}

void miral::SessionLockListener::operator()(mir::Server& server) const
{
    server.add_init_callback([&]()
    {
        server.the_session_lock()->register_interest(self);
    });
}