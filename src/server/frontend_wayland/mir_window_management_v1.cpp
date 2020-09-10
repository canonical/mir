/*
 * Copyright © 2020 Canonical Ltd.
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "mir_window_management_v1.h"

#include "window_wl_surface_role.h"
#include "wl_surface.h"
#include "mir/log.h"
#include "mir/scene/surface.h"
#include "mir/shell/shell.h"

#include <boost/throw_exception.hpp>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mw = mir::wayland;

namespace mir
{
namespace frontend
{

class MirWmBaseV1Global
    : public wayland::MirWmBaseV1::Global
{
public:
    MirWmBaseV1Global(wl_display* display, std::shared_ptr<shell::Shell> const& shell);

    std::shared_ptr<shell::Shell> const shell;

private:
    void bind(wl_resource* new_resource) override;
};

class MirWmBaseV1
    : public wayland::MirWmBaseV1
{
public:
    MirWmBaseV1(wl_resource* new_resource, MirWmBaseV1Global& global);
    ~MirWmBaseV1() = default;

private:
    /// Wayland requests
    ///@{
    void destroy() override;
    void get_shell_surface(struct wl_resource* id, struct wl_resource* surface) override;
    ///@}

    MirWmBaseV1Global& global;
};

}
}

auto mf::create_mir_wm_base_v1(
    wl_display* display,
    std::shared_ptr<msh::Shell> const& shell) -> std::shared_ptr<MirWmBaseV1Global>
{
    return std::make_shared<MirWmBaseV1Global>(display, shell);
}

// MirWmBaseV1Global

mf::MirWmBaseV1Global::MirWmBaseV1Global(wl_display* display, std::shared_ptr<shell::Shell> const& shell)
    : Global{display, Version<1>()},
      shell{shell}
{
}

void mf::MirWmBaseV1Global::bind(wl_resource* new_resource)
{
    new MirWmBaseV1{new_resource, *this};
}

// MirWmBaseV1

mf::MirWmBaseV1::MirWmBaseV1(wl_resource* new_resource, MirWmBaseV1Global& global)
    : mw::MirWmBaseV1{new_resource, Version<1>()},
      global{global}
{
}

void mf::MirWmBaseV1::destroy()
{
    destroy_wayland_object();
}

void mf::MirWmBaseV1::get_shell_surface(struct wl_resource* id, struct wl_resource* surface)
{
    new MirShellSurfaceV1(id, WlSurface::from(surface), global);
}

// MirShellSurfaceV1

std::mutex mf::MirShellSurfaceV1::surface_map_mutex;
std::unordered_map<mf::WlSurface*, mw::Weak<mf::MirShellSurfaceV1>> mf::MirShellSurfaceV1::surface_map;

mf::MirShellSurfaceV1::MirShellSurfaceV1(wl_resource* resource, WlSurface* surface, MirWmBaseV1Global& global)
    : mw::MirShellSurfaceV1{resource, Version<1>()},
      global{global},
      surface{surface},
      surface_unsafe{surface}
{
    std::lock_guard<std::mutex> lock{surface_map_mutex};
    auto const result = surface_map.insert(std::make_pair(surface, mw::make_weak(this)));
    if (!result.second)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error{
            "Multiple MirShellSurfaceV1s made for wl_surface@" +
            std::to_string(wl_resource_get_id(surface->resource))});
    }
}

mf::MirShellSurfaceV1::~MirShellSurfaceV1()
{
    std::lock_guard<std::mutex> lock{surface_map_mutex};
    surface_map.erase(surface_unsafe);
}

void mf::MirShellSurfaceV1::prepare_for_surface_creation(ms::SurfaceCreationParameters& creation_params)
{
    if (creation_params.type)
    {
        original_type = creation_params.type.value();
    }

    if (override_type)
    {
        creation_params.type = override_type.value();
    }
}

auto mf::MirShellSurfaceV1::from(WlSurface* surface) -> mw::Weak<MirShellSurfaceV1>
{
    std::lock_guard<std::mutex> lock{surface_map_mutex};
    auto const result = surface_map.find(surface);
    if (result == surface_map.end())
    {
        return {};
    }
    else
    {
        if (!result->second)
        {
            log_warning(
                "MirShellSurfaceV1 for surface %d destroyed without being removed from map",
                wl_resource_get_id(surface->resource));
            surface_map.erase(result);
        }
        return result->second;
    }
}

void mf::MirShellSurfaceV1::destroy()
{
    destroy_wayland_object();
}

void mf::MirShellSurfaceV1::set_window_type(uint32_t type)
{
    switch (type)
    {
    case WindowType::default_:
        override_type = std::experimental::nullopt;
        break;
    case WindowType::satellite:
        override_type = mir_window_type_satellite;
        break;
    default:
        wl_resource_post_error(resource, Error::invalid_window_type, "%d is not a valid window type", type);
        BOOST_THROW_EXCEPTION(std::runtime_error{"Invalid window type " + std::to_string(type)});
    }

    auto const scene_surface = surface ? surface.value().scene_surface().value_or(nullptr) : nullptr;

    if (!scene_surface)
    {
        // If it's not a window surface, this request can be ignored
        // If its scene surface is not created yet, prepare_for_surface_creation() will be called in the future
        return;
    }

    if (!original_type)
    {
        original_type = scene_surface->type();
    }

    auto const session = scene_surface->session().lock();
    if (!session)
    {
        log_error("MirShellSurfaceV1::set_window_type(): scene surface does not have a session");
        return;
    }

    msh::SurfaceSpecification spec;
    spec.type = override_type.value_or(original_type.value_or(mir_window_type_normal));
    global.shell->modify_surface(session, scene_surface, spec);
}
