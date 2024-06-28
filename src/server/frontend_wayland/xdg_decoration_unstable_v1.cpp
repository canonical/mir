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
#include "mir/wayland/client.h"
#include "mir/wayland/protocol_error.h"
#include "mir/xdg_decorations_interface.h"
#include "mir/log.h"

#include "xdg-decoration-unstable-v1_wrapper.h"
#include "xdg_output_v1.h"
#include "xdg_shell_stable.h"

#include <memory>
#include <unordered_set>

namespace mir
{

using ServerMode = mir::XdgDecorationsInterface::Mode;

namespace frontend
{
class XdgDecorationManagerV1 : public wayland::XdgDecorationManagerV1
{
public:
    XdgDecorationManagerV1(wl_resource* resource, std::shared_ptr<mir::XdgDecorationsInterface> xdg_decorations);

    class Global : public wayland::XdgDecorationManagerV1::Global
    {
    public:
        Global(wl_display* display, std::shared_ptr<mir::XdgDecorationsInterface> xdg_decorations);

    private:
        void bind(wl_resource* new_zxdg_decoration_manager_v1) override;
        std::shared_ptr<mir::XdgDecorationsInterface> xdg_decorations;
    };

private:
    void get_toplevel_decoration(wl_resource* id, wl_resource* toplevel) override;
    std::shared_ptr<std::unordered_set<wl_resource*>> toplevels_with_decorations;
    std::shared_ptr<mir::XdgDecorationsInterface> xdg_decorations;
};

class XdgToplevelDecorationV1 : public wayland::XdgToplevelDecorationV1
{
public:
    XdgToplevelDecorationV1(
        wl_resource* id,
        mir::frontend::XdgToplevelStable* toplevel,
        std::shared_ptr<mir::XdgDecorationsInterface> xdg_decorations);

    void set_mode(uint32_t mode) override;
    void unset_mode() override;

private:
    void update_mode(uint32_t new_mode);

    mir::frontend::XdgToplevelStable* toplevel;
    std::shared_ptr<mir::XdgDecorationsInterface> xdg_decorations;
    uint32_t mode;
};
} // namespace frontend
} // namespace mir

auto mir::frontend::create_xdg_decoration_unstable_v1(
    wl_display* display, std::shared_ptr<mir::XdgDecorationsInterface> xdg_decorations)
    -> std::shared_ptr<mir::wayland::XdgDecorationManagerV1::Global>
{
    return std::make_shared<XdgDecorationManagerV1::Global>(display, xdg_decorations);
}

mir::frontend::XdgDecorationManagerV1::Global::Global(
    wl_display* display, std::shared_ptr<mir::XdgDecorationsInterface> xdg_decorations) :
    wayland::XdgDecorationManagerV1::Global::Global{display, Version<1>{}},
    xdg_decorations{xdg_decorations}
{
}

void mir::frontend::XdgDecorationManagerV1::Global::bind(wl_resource* new_zxdg_decoration_manager_v1)
{
    new XdgDecorationManagerV1{new_zxdg_decoration_manager_v1, xdg_decorations};
}

mir::frontend::XdgDecorationManagerV1::XdgDecorationManagerV1(
    wl_resource* resource, std::shared_ptr<mir::XdgDecorationsInterface> xdg_decorations) :
    mir::wayland::XdgDecorationManagerV1{resource, Version<1>{}},
    toplevels_with_decorations(std::make_shared<std::unordered_set<wl_resource*>>()),
    xdg_decorations{xdg_decorations}
{
}

void mir::frontend::XdgDecorationManagerV1::get_toplevel_decoration(wl_resource* id, wl_resource* toplevel)
{
    using Error = mir::frontend::XdgToplevelDecorationV1::Error;

    if (toplevels_with_decorations->contains(toplevel))
    {
        BOOST_THROW_EXCEPTION(mir::wayland::ProtocolError(
            resource, Error::already_constructed, "Decoration already constructed for this toplevel"));
    }

    auto* tl = mir::frontend::XdgToplevelStable::from(toplevel);
    if (!tl)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid toplevel pointer"));
    }

    auto decoration = new XdgToplevelDecorationV1{id, tl, xdg_decorations};
    toplevels_with_decorations->insert(toplevel);

    decoration->add_destroy_listener([toplevels_with_decorations = this->toplevels_with_decorations, toplevel]()
                                     { toplevels_with_decorations->erase(toplevel); });

    tl->add_destroy_listener(
        [toplevels_with_decorations = this->toplevels_with_decorations, client = this->client, toplevel]()
        {
            if (!client->is_being_destroyed() && toplevels_with_decorations->erase(toplevel) > 0)
            {
                mir::log_warning("Toplevel destroyed before attached decoration!");
                // https://github.com/canonical/mir/issues/3452
                /* BOOST_THROW_EXCEPTION(mir::wayland::ProtocolError( */
                /*     resource, Error::orphaned, "Toplevel destroyed before its attached decoration")); */
            }
        });
}

mir::frontend::XdgToplevelDecorationV1::XdgToplevelDecorationV1(
    wl_resource* id,
    mir::frontend::XdgToplevelStable* toplevel,
    std::shared_ptr<mir::XdgDecorationsInterface> xdg_decorations) :
    wayland::XdgToplevelDecorationV1{id, Version<1>{}},
    toplevel{toplevel},
    xdg_decorations{xdg_decorations}
{
}

void mir::frontend::XdgToplevelDecorationV1::update_mode(uint32_t new_mode)
{
    switch (xdg_decorations->mode())
    {
    case ServerMode::always_ssd:
        mode = Mode::server_side;
        break;
    case ServerMode::always_csd:
        mode = Mode::client_side;
        break;
    case ServerMode::prefer_ssd:
    case ServerMode::prefer_csd:
        mode = new_mode;
        break;
    }

    auto spec = shell::SurfaceSpecification{};

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

void mir::frontend::XdgToplevelDecorationV1::unset_mode()
{
    uint32_t new_mode = Mode::client_side; // To silence the warning
    switch (xdg_decorations->mode())
    {
    case ServerMode::prefer_ssd:
    case ServerMode::always_ssd:
        new_mode = Mode::server_side;
        break;
    case ServerMode::always_csd:
    case ServerMode::prefer_csd:
        new_mode = Mode::client_side;
        break;
    }

    update_mode(new_mode);
}
