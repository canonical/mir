/*
 * Copyright © 2021 Canonical Ltd.
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

#include "virtual_pointer_v1.h"
#include "wayland_wrapper.h"

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace mi = mir::input;

namespace mir
{
namespace frontend
{
struct VirtualPointerV1Ctx
{
    std::shared_ptr<mi::InputDeviceRegistry> const device_registry;
};

class VirtualPointerManagerV1Global
    : public wayland::VirtualPointerManagerV1::Global
{
public:
    VirtualPointerManagerV1Global(wl_display* display, std::shared_ptr<VirtualPointerV1Ctx> const& ctx);

private:
    void bind(wl_resource* new_resource) override;

    std::shared_ptr<VirtualPointerV1Ctx> const ctx;
};

class VirtualPointerManagerV1
    : public wayland::VirtualPointerManagerV1
{
public:
    VirtualPointerManagerV1(wl_resource* resource, std::shared_ptr<VirtualPointerV1Ctx> const& ctx);

private:
    void create_virtual_pointer(std::optional<struct wl_resource*> const& seat, struct wl_resource* id) override;
    void create_virtual_pointer_with_output(
        std::optional<struct wl_resource*> const& seat,
        std::optional<struct wl_resource*> const& output,
        struct wl_resource* id) override;

    std::shared_ptr<VirtualPointerV1Ctx> const ctx;
};

class VirtualPointerV1
    : public wayland::VirtualPointerV1
{
public:
    VirtualPointerV1(wl_resource* resource, std::shared_ptr<VirtualPointerV1Ctx> const& ctx);

private:
    void motion(uint32_t time, double dx, double dy) override;
    void motion_absolute(uint32_t time, uint32_t x, uint32_t y, uint32_t x_extent, uint32_t y_extent) override;
    void button(uint32_t time, uint32_t button, uint32_t state) override;
    void axis(uint32_t time, uint32_t axis, double value) override;
    void frame() override;
    void axis_source(uint32_t axis_source) override;
    void axis_stop(uint32_t time, uint32_t axis) override;
    void axis_discrete(uint32_t time, uint32_t axis, double value, int32_t discrete) override;

    std::shared_ptr<VirtualPointerV1Ctx> const ctx;
};
}
}

auto mf::create_virtual_pointer_manager_v1(
    wl_display* display,
    std::shared_ptr<mi::InputDeviceRegistry> const& device_registry)
-> std::shared_ptr<mw::VirtualPointerManagerV1::Global>
{
    auto ctx = std::shared_ptr<VirtualPointerV1Ctx>{new VirtualPointerV1Ctx{device_registry}};
    return std::make_shared<VirtualPointerManagerV1Global>(display, std::move(ctx));
}

mf::VirtualPointerManagerV1Global::VirtualPointerManagerV1Global(
    wl_display* display,
    std::shared_ptr<VirtualPointerV1Ctx> const& ctx)
    : Global{display, Version<2>()},
      ctx{ctx}
{
}

void mf::VirtualPointerManagerV1Global::bind(wl_resource* new_resource)
{
    new VirtualPointerManagerV1{new_resource, ctx};
}

mf::VirtualPointerManagerV1::VirtualPointerManagerV1(
    wl_resource* resource,
    std::shared_ptr<VirtualPointerV1Ctx> const& ctx)
    : wayland::VirtualPointerManagerV1{resource, Version<2>()},
      ctx{ctx}
{
}

void mf::VirtualPointerManagerV1::create_virtual_pointer(
    std::optional<struct wl_resource*> const& seat,
    struct wl_resource* id)
{
    (void)seat;
    new VirtualPointerV1{id, ctx};
}

void mf::VirtualPointerManagerV1::create_virtual_pointer_with_output(
    std::optional<struct wl_resource*> const& seat,
    std::optional<struct wl_resource*> const& output,
    struct wl_resource* id)
{
    (void)seat;
    (void)output;
    new VirtualPointerV1{id, ctx};
}


mf::VirtualPointerV1::VirtualPointerV1(
    wl_resource* resource,
    std::shared_ptr<VirtualPointerV1Ctx> const& ctx)
    : wayland::VirtualPointerV1{resource, Version<2>()},
      ctx{ctx}
{
}

void mf::VirtualPointerV1::motion(uint32_t time, double dx, double dy)
{
    (void)time;
    (void)dx;
    (void)dy;
    // TODO
}

void mf::VirtualPointerV1::motion_absolute(uint32_t time, uint32_t x, uint32_t y, uint32_t x_extent, uint32_t y_extent)
{
    (void)time;
    (void)x;
    (void)y;
    (void)x_extent;
    (void)y_extent;
    // TODO
}

void mf::VirtualPointerV1::button(uint32_t time, uint32_t button, uint32_t state)
{
    (void)time;
    (void)button;
    (void)state;
    // TODO
}

void mf::VirtualPointerV1::axis(uint32_t time, uint32_t axis, double value)
{
    (void)time;
    (void)axis;
    (void)value;
    // TODO
}

void mf::VirtualPointerV1::frame()
{
    // TODO
}

void mf::VirtualPointerV1::axis_source(uint32_t axis_source)
{
    (void)axis_source;
    // TODO
}

void mf::VirtualPointerV1::axis_stop(uint32_t time, uint32_t axis)
{
    (void)time;
    (void)axis;
    // TODO
}

void mf::VirtualPointerV1::axis_discrete(uint32_t time, uint32_t axis, double value, int32_t discrete)
{
    (void)time;
    (void)axis;
    (void)value;
    (void)discrete;
    // TODO
}

