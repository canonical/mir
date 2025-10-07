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

#include "miral/idle_listener.h"
#include <mir/server.h>
#include <mir/shell/idle_handler.h>

class miral::IdleListener::Impl : public mir::shell::IdleHandlerObserver
{
public:
    explicit Impl()
        : on_dim{[]{}},
          on_off{[]{}},
          on_wake{[]{}}
    {
    }

    void dim() override
    {
        on_dim();
    }

    void off() override
    {
        on_off();
    }

    void wake() override
    {
        on_wake();
    }

    miral::IdleListener::Callback on_dim;
    miral::IdleListener::Callback on_off;
    miral::IdleListener::Callback on_wake;
};

miral::IdleListener::IdleListener()
    : self{std::make_shared<Impl>()}
{
}

miral::IdleListener& miral::IdleListener::on_dim(miral::IdleListener::Callback const& cb)
{
    self->on_dim = cb;
    return *this;
}

miral::IdleListener& miral::IdleListener::on_off(miral::IdleListener::Callback const& cb)
{
    self->on_off = cb;
    return *this;
}

miral::IdleListener& miral::IdleListener::on_wake(miral::IdleListener::Callback const& cb)
{
    self->on_wake = cb;
    return *this;
}

void miral::IdleListener::operator()(mir::Server& server) const
{
    server.add_init_callback([&]()
    {
        server.the_idle_handler()->register_interest(self);
    });
}
