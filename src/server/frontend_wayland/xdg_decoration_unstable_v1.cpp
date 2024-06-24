/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#include "xdg_decoration_unstable_v1.h"

#include "mir/shell/surface_specification.h"
#include "mir/wayland/protocol_error.h"

#include "xdg-decoration-unstable-v1_wrapper.h"
#include "xdg_output_v1.h"
#include "xdg_shell_stable.h"

#include <unordered_set>

namespace mir
{
namespace frontend
{
class XdgDecorationManagerV1 : public wayland::XdgDecorationManagerV1
{
public:
    XdgDecorationManagerV1(wl_resource* resource);

    class Global : public wayland::XdgDecorationManagerV1::Global
    {
    public:
        Global(wl_display* display);

    private:
        void bind(wl_resource* new_zxdg_decoration_manager_v1) override;
    };

private:
    void get_toplevel_decoration(wl_resource* id, wl_resource* toplevel) override;
    std::unordered_set<wl_resource*> toplevels_with_decorations;
};

class XdgToplevelDecorationV1 : public wayland::XdgToplevelDecorationV1
{
public:
    XdgToplevelDecorationV1(wl_resource* id, mir::frontend::XdgToplevelStable* toplevel);

    void set_mode(uint32_t mode) override;
    void unset_mode() override;

private:
    void update_mode(uint32_t new_mode);

    static uint32_t const default_mode = Mode::client_side;

    mir::frontend::XdgToplevelStable* toplevel;
    uint32_t mode;
};
} // namespace frontend
} // namespace mir

auto mir::frontend::create_xdg_decoration_unstable_v1(wl_display* display)
    -> std::shared_ptr<mir::wayland::XdgDecorationManagerV1::Global>
{
    return std::make_shared<XdgDecorationManagerV1::Global>(display);
}

mir::frontend::XdgDecorationManagerV1::Global::Global(wl_display* display) :
    wayland::XdgDecorationManagerV1::Global::Global{display, Version<1>{}}
{
}

void mir::frontend::XdgDecorationManagerV1::Global::bind(wl_resource* new_zxdg_decoration_manager_v1)
{
    new XdgDecorationManagerV1{new_zxdg_decoration_manager_v1};
}

mir::frontend::XdgDecorationManagerV1::XdgDecorationManagerV1(wl_resource* resource) :
    mir::wayland::XdgDecorationManagerV1{resource, Version<1>{}}
{
}

void mir::frontend::XdgDecorationManagerV1::get_toplevel_decoration(wl_resource* id, wl_resource* toplevel)
{
    using Error = mir::frontend::XdgToplevelDecorationV1::Error;

    if (toplevels_with_decorations.contains(toplevel))
    {
        BOOST_THROW_EXCEPTION(mir::wayland::ProtocolError(
            resource, Error::already_constructed, "Decoration already constructed for this toplevel"));
    }

    auto* tl = mir::frontend::XdgToplevelStable::from(toplevel);
    if (!tl)
    {
        BOOST_THROW_EXCEPTION(
            mir::wayland::ProtocolError(resource, Error::orphaned, "Toplevel destroyed before attached decoration"));
    }


    new XdgToplevelDecorationV1{id, tl};
    toplevels_with_decorations.insert(toplevel);
}

mir::frontend::XdgToplevelDecorationV1::XdgToplevelDecorationV1(
    wl_resource* id, mir::frontend::XdgToplevelStable* toplevel) :
    wayland::XdgToplevelDecorationV1{id, Version<1>{}},
    toplevel{toplevel}
{
}

void mir::frontend::XdgToplevelDecorationV1::update_mode(uint32_t new_mode)
{
    auto spec = shell::SurfaceSpecification{};

    mode = new_mode;
    switch (mode)
    {
    case Mode::client_side:
        spec.server_side_decorated = false;
        break;
    case Mode::server_side:
        spec.server_side_decorated = true;
        break;
    }

    this->toplevel->apply_spec(spec);
    send_configure_event(mode);
}

void mir::frontend::XdgToplevelDecorationV1::set_mode(uint32_t mode) { update_mode(mode); }

void mir::frontend::XdgToplevelDecorationV1::unset_mode() { update_mode(default_mode); }
