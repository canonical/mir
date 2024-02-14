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
#include <mir/frontend/session_locker.h>


class miral::SessionLockListener::Impl : public mir::frontend::SessionLockObserver
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


    std::shared_ptr<mir::frontend::SessionLocker> session_locker;
    Callback on_lock_;
    Callback on_unlock_;
};

miral::SessionLockListener::SessionLockListener(Callback const& on_lock, Callback const& on_unlock)
    : self{std::make_shared<Impl>(on_lock, on_unlock)}
{
}

miral::SessionLockListener::~SessionLockListener()
{
    if (self->session_locker != nullptr)
    {
        self->session_locker->remove_observer(self);
        self->session_locker = nullptr;
    }
}

void miral::SessionLockListener::operator()(mir::Server& server) const
{
    server.add_init_callback([&]()
    {
        self->session_locker = server.the_session_locker();
        self->session_locker->add_observer(self);
    });
}