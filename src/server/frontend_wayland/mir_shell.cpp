/*
 * Copyright © Canonical Ltd.
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

#include "mir_shell.h"

#include "xdg_shell_stable.h"

#include "wl_surface.h"
#include "mir/scene/surface.h"
#include "mir/shell/surface_specification.h"
#include "mir/shell/shell.h"

namespace mf = mir::frontend;
namespace mw = mir::wayland;

using mir::shell::Shell;
using mir::shell::SurfaceSpecification;

namespace
{
mw::Resource::Version<1> version;

struct Global : mw::MirShellV1::Global
{
    Global(struct wl_display* display, std::shared_ptr<Shell> shell) :
        mw::MirShellV1::Global{display, Version<1>{}},
        shell(std::move(shell)) {}

    void bind(wl_resource* new_zmir_mir_shell_v1) override;

    std::shared_ptr<Shell> const shell;
};

struct Instance : mw::MirShellV1
{
    Instance(struct wl_resource* resource, std::shared_ptr<Shell> shell) :
        MirShellV1{resource, version},
        shell(std::move(shell)) {}

    void get_normal_surface(struct wl_resource* id, struct wl_resource* surface) override;

    void get_utility_surface(struct wl_resource* id, struct wl_resource* surface) override;

    void get_dialog_surface(struct wl_resource* id, struct wl_resource* surface) override;

    void
    get_satellite_surface(struct wl_resource* id, struct wl_resource* surface, struct wl_resource* positioner) override;

    void get_freestyle_surface(
        struct wl_resource* id, struct wl_resource* surface,
        std::optional<struct wl_resource*> const& positioner) override;

    std::shared_ptr<Shell> const shell;
};

template<typename MirSurface, MirWindowType type>
struct Surface : MirSurface
{
    Surface(std::shared_ptr<Shell> const& shell, struct wl_resource* resource, mf::WlSurface* wl_surface) :
        MirSurface{resource, version}
    {
        if (auto const ms = wl_surface->scene_surface())
        {
            SurfaceSpecification spec;
            spec.type = type;

            shell->modify_surface(
                ms.value()->session().lock(),
                ms.value(),
                spec);
        }
    }
};
}

auto mf::create_mir_shell_v1(struct wl_display* display, std::shared_ptr<Shell> shell) -> std::shared_ptr<mw::MirShellV1::Global>
{
    return std::make_shared<Global>(display, shell);
}

void Global::bind(wl_resource* new_zmir_mir_shell_v1)
{
    new Instance{new_zmir_mir_shell_v1, nullptr};
}

void Instance::get_normal_surface(struct wl_resource* id, struct wl_resource* surface)
{
    new Surface<mw::MirNormalSurfaceV1, mir_window_type_normal>{shell, id, mf::WlSurface::from(surface)};
}

void Instance::get_utility_surface(struct wl_resource* id, struct wl_resource* surface)
{
    new Surface<mw::MirUtilitySurfaceV1, mir_window_type_utility>{shell, id, mf::WlSurface::from(surface)};
}

void Instance::get_dialog_surface(struct wl_resource* id, struct wl_resource* surface)
{
    new Surface<mw::MirDialogSurfaceV1, mir_window_type_dialog>{shell, id, mf::WlSurface::from(surface)};
}

void Instance::get_satellite_surface(wl_resource* id, wl_resource* surface, wl_resource* positioner)
{
    struct Surface : mw::MirSatelliteSurfaceV1
    {
        Surface(std::shared_ptr<Shell> const& shell, struct wl_resource* resource, mf::WlSurface* wl_surface, struct wl_resource* positioner) :
            mw::MirSatelliteSurfaceV1{resource, version},
            shell{shell},
            wl_surface{wl_surface}
        {
            auto pspec = dynamic_cast<SurfaceSpecification*>(mw::XdgPositioner::from(positioner));

            if (auto const ms = wl_surface->scene_surface())
            {
                auto spec = pspec ? *pspec : SurfaceSpecification{};
                spec.type = mir_window_type_satellite;

                shell->modify_surface(
                    ms.value()->session().lock(),
                    ms.value(),
                    spec);
            }
        }

        void reposition(struct wl_resource* positioner, uint32_t /*token*/) override
        {
            auto pspec = dynamic_cast<mf::XdgPositionerStable*>(mw::XdgPositioner::from(positioner));

            if (auto const ms = wl_surface->scene_surface())
            {
                auto spec = pspec ? *pspec : SurfaceSpecification{};
                spec.type = mir_window_type_satellite;

                shell->modify_surface(
                    ms.value()->session().lock(),
                    ms.value(),
                    spec);
            }
        }

        std::shared_ptr<Shell> const shell;
        mf::WlSurface* const wl_surface;
    };

    new Surface{shell, id, mf::WlSurface::from(surface), positioner};
}

void Instance::get_freestyle_surface(
    struct wl_resource* id, struct wl_resource* surface, std::optional<struct wl_resource*> const& positioner)
{
    struct Surface : mw::MirFreestyleSurfaceV1
    {
        Surface(std::shared_ptr<Shell> const& shell, struct wl_resource* resource, mf::WlSurface* wl_surface, struct wl_resource* positioner) :
            mw::MirFreestyleSurfaceV1{resource, version},
            shell{shell},
            wl_surface{wl_surface}
        {
            auto pspec = dynamic_cast<SurfaceSpecification*>(mw::XdgPositioner::from(positioner));

            if (auto const ms = wl_surface->scene_surface())
            {
                auto spec = pspec ? *pspec : SurfaceSpecification{};
                spec.type = mir_window_type_freestyle;

                shell->modify_surface(
                    ms.value()->session().lock(),
                    ms.value(),
                    spec);
            }
        }

        void reposition(struct wl_resource* positioner, uint32_t /*token*/) override
        {
            auto pspec = dynamic_cast<mf::XdgPositionerStable*>(mw::XdgPositioner::from(positioner));

            if (auto const ms = wl_surface->scene_surface())
            {
                auto spec = pspec ? *pspec : SurfaceSpecification{};
                spec.type = mir_window_type_freestyle;

                shell->modify_surface(
                    ms.value()->session().lock(),
                    ms.value(),
                    spec);
            }
        }

        std::shared_ptr<Shell> const shell;
        mf::WlSurface* const wl_surface;
    };

    new Surface{shell, id, mf::WlSurface::from(surface), positioner.value_or(nullptr)};
}
