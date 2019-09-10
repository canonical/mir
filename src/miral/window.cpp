/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "miral/window.h"

#include <mir/scene/session.h>
#include <mir/scene/surface.h>

struct miral::Window::Self
{
    Self(std::shared_ptr<mir::scene::Session> const& session, std::shared_ptr<mir::scene::Surface> const& surface);

    std::weak_ptr<mir::scene::Session> const session;
    std::weak_ptr<mir::scene::Surface> const surface;
};

miral::Window::Self::Self(std::shared_ptr<mir::scene::Session> const& session, std::shared_ptr<mir::scene::Surface> const& surface) :
    session{session}, surface{surface} {}

miral::Window::Window(Application const& application, std::shared_ptr<mir::scene::Surface> const& surface) :
    self{std::make_shared<Self>(application, surface)}
{
}

miral::Window::Window()
{
}

miral::Window::~Window() = default;

miral::Window::operator bool() const
{
    return !!operator std::shared_ptr<mir::scene::Surface>();
}

miral::Window::operator std::shared_ptr<mir::scene::Surface>() const
{
    if (!self) return {};
    return self->surface.lock();
}

miral::Window::operator std::weak_ptr<mir::scene::Surface>() const
{
    if (!self) return {};
    return self->surface;
}

void miral::Window::resize(mir::geometry::Size const& size)
{
    if (!self) return;
    if (auto const surface = self->surface.lock())
        surface->resize(size);
}

void miral::Window::move_to(mir::geometry::Point top_left)
{
    if (!self) return;
    if (auto const surface = self->surface.lock())
        surface->move_to(top_left);
}

auto miral::Window::top_left() const
-> mir::geometry::Point
{
    if (self)
    {
        if (auto const surface = self->surface.lock())
            return surface->top_left();
    }

    return {};
}

auto miral::Window::size() const
-> mir::geometry::Size
{
    if (self)
    {
        if (auto const surface = self->surface.lock())
            return surface->size();
    }

    return {};
}

auto miral::Window::application() const
-> Application
{
    if (!self) return {};
    return self->session.lock();
}

bool miral::operator==(Window const& lhs, Window const& rhs)
{
    return lhs.self == rhs.self;
}

bool miral::operator==(std::shared_ptr<mir::scene::Surface> const& lhs, Window const& rhs)
{
    if (!rhs.self) return !lhs;
    return lhs == rhs.self->surface.lock();
}

bool miral::operator==(Window const& lhs, std::shared_ptr<mir::scene::Surface> const& rhs)
{
    return rhs == lhs;
}

bool miral::operator<(Window const& lhs, Window const& rhs)
{
    return lhs.self.owner_before(rhs.self);
}
