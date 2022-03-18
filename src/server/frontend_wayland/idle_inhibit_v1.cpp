/*
 * Copyright © 2022 Canonical Ltd.
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
 * Authored by: Grayson Guarino <grayson.guarino@canonical.com>
 */

#include "idle_inhibit_v1.h"

#include "mir/log.h"

#include <deque>

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace ms = mir::scene;

namespace mir
{
namespace frontend
{
class IdleInhibitManagerV1Global
    : public wayland::IdleInhibitManagerV1::Global
{
public:
    IdleInhibitManagerV1Global(wl_display* display);

private:
    void bind(wl_resource* new_resource) override;
};

class IdleInhibitManagerV1
    : public wayland::IdleInhibitManagerV1
{
public:
    IdleInhibitManagerV1(wl_resource* resource);
    ~IdleInhibitManagerV1();

private:
    void create_inhibitor(struct wl_resource* id, struct wl_resource* surface) override;
};

class IdleInhibitorV1
    : public wayland::IdleInhibitorV1
{
public:
    IdleInhibitorV1(wl_resource* resource);
    ~IdleInhibitorV1();

private:
};
}
}

auto mf::create_idle_inhibit_manager_v1(wl_display* display) 
-> std::shared_ptr<mw::IdleInhibitManagerV1::Global> {
    return std::make_shared<IdleInhibitManagerV1Global>(display);
}

mf::IdleInhibitManagerV1Global::IdleInhibitManagerV1Global(wl_display *display)
    : Global{display, Version<1>()}
{
}

void mf::IdleInhibitManagerV1Global::bind(wl_resource *new_resource)
{
    new IdleInhibitManagerV1{new_resource};
}

mf::IdleInhibitManagerV1::IdleInhibitManagerV1(wl_resource *resource)
    : wayland::IdleInhibitManagerV1{resource, Version<1>()}
{
}

void mf::IdleInhibitManagerV1::create_inhibitor(struct wl_resource *id, struct wl_resource *surface)
{
    (void)id;
    new wayland::IdleInhibitorV1{surface, Version<1>()};
}

mf::IdleInhibitManagerV1::~IdleInhibitManagerV1()
{
}