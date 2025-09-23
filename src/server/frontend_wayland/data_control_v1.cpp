/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include "data_control_v1.h"

namespace mf = mir::frontend;

namespace mir
{
namespace frontend
{
class DataControlManagerV1 : public wayland::DataControlManagerV1::Global
{
public:
    DataControlManagerV1(struct wl_display*);
    ~DataControlManagerV1() = default;

private:
    class Instance : public wayland::DataControlManagerV1
    {
    public:
        Instance(struct wl_resource*);

    private:
        void create_data_source(struct wl_resource* id) override;
        void get_data_device(struct wl_resource* id, struct wl_resource* seat) override;
    };

    void bind(wl_resource*) override;
};
class DataControlDeviceV1 : public wayland::DataControlDeviceV1
{
public:
    DataControlDeviceV1(struct wl_resource* resource);

private:
    void set_selection(std::optional<struct wl_resource*> const& source) override;
    void set_primary_selection(std::optional<struct wl_resource*> const& source) override;
};

class DataControlSourceV1 : public wayland::DataControlSourceV1
{
public:
    DataControlSourceV1(struct wl_resource* resource);

private:
    void offer(std::string const& mime_type) override;
};

class DataControlOfferV1 : public wayland::DataControlOfferV1
{
public:
    DataControlOfferV1(DataControlDeviceV1 const& parent);

private:
    virtual void receive(std::string const& mime_type, mir::Fd fd) override;
};
}
}

mf::DataControlManagerV1::DataControlManagerV1(struct wl_display* display)
    : wayland::DataControlManagerV1::Global(display, Version<1>{})
{
}

void mf::DataControlManagerV1::bind(wl_resource*)
{
}

mf::DataControlManagerV1::Instance::Instance(struct wl_resource* resource)
    : wayland::DataControlManagerV1(resource, Version<1>{})
{

}

void mf::DataControlManagerV1::Instance::create_data_source(struct wl_resource*)
{
    // TODO
}

void mf::DataControlManagerV1::Instance::get_data_device(struct wl_resource*, struct wl_resource*)
{
    // TODO
}

mf::DataControlDeviceV1::DataControlDeviceV1(struct wl_resource* resource)
    : wayland::DataControlDeviceV1(resource, Version<1>{})
{
}

void mf::DataControlDeviceV1::set_selection(std::optional<struct wl_resource*> const&)
{
    // TODO
}

void mf::DataControlDeviceV1::set_primary_selection(std::optional<struct wl_resource*> const&)
{
}

void mf::DataControlSourceV1::offer(std::string const&)
{
}

mf::DataControlOfferV1::DataControlOfferV1(DataControlDeviceV1 const& parent)
    : wayland::DataControlOfferV1(parent)
{
}

void mf::DataControlOfferV1::receive(std::string const&, mir::Fd)
{
    // TODO
}

auto mf::create_data_control_manager_v1(struct wl_display* display)
    -> std::shared_ptr<wayland::DataControlManagerV1::Global>
{
    return std::make_shared<DataControlManagerV1>(display);
}
