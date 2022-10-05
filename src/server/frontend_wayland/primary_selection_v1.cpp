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
 */

#include "primary_selection_v1.h"

namespace mf = mir::frontend;
namespace mw = mir::wayland;

namespace
{
class PrimarySelectionSource : public mw::PrimarySelectionSourceV1
{
public:
    PrimarySelectionSource(wl_resource* resource)
        : PrimarySelectionSourceV1{resource, Version<1>()}
    {
    }

    void offer(std::string const& mime_type) override
    {
        (void)mime_type;
    }
};

class PrimarySelectionDevice : public mw::PrimarySelectionDeviceV1
{
public:
    PrimarySelectionDevice(wl_resource* resource)
        : PrimarySelectionDeviceV1{resource, Version<1>()}
    {
    }

    void set_selection(std::optional<struct wl_resource*> const& source, uint32_t serial) override
    {
        (void)source;
        (void)serial;
    }
};

class PrimarySelectionManager : public mw::PrimarySelectionDeviceManagerV1
{
public:
    PrimarySelectionManager(wl_resource* manager)
        : PrimarySelectionDeviceManagerV1{manager, Version<1>()}
    {
    }

    void create_source(struct wl_resource* id) override
    {
        new PrimarySelectionSource{id};
    }

    void get_device(struct wl_resource* id, struct wl_resource* seat) override
    {
        new PrimarySelectionDevice{id};
        (void)seat;
    }
};

class PrimarySelectionGlobal : public mw::PrimarySelectionDeviceManagerV1::Global
{
public:
    PrimarySelectionGlobal(wl_display* display)
        : Global{display, Version<1>()}
    {
    }

    void bind(wl_resource* manager) override
    {
        new PrimarySelectionManager{manager};
    }
};
}

auto mf::create_primary_selection_device_manager_v1(wl_display* display)
-> std::shared_ptr<mw::PrimarySelectionDeviceManagerV1::Global>
{
    return std::make_shared<PrimarySelectionGlobal>(display);
}
