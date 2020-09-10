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

#ifndef MIR_FRONTEND_MIR_WINDOW_MANAGEMENT_V1_H
#define MIR_FRONTEND_MIR_WINDOW_MANAGEMENT_V1_H

#include <memory>
#include <experimental/optional>
#include <unordered_map>
#include <mutex>

#include "mir-window-management-unstable-v1_wrapper.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir_toolkit/common.h"

struct wl_display;
struct wl_resource;

namespace mir
{
namespace shell
{
class Shell;
}
namespace frontend
{
class MirWmBaseV1Global;
class WlSurface;

auto create_mir_wm_base_v1(
    wl_display* display,
    std::shared_ptr<shell::Shell> const& shell) -> std::shared_ptr<MirWmBaseV1Global>;

class MirShellSurfaceV1
    : public wayland::MirShellSurfaceV1
{
public:
    MirShellSurfaceV1(wl_resource* resource, WlSurface* surface, MirWmBaseV1Global& global);
    ~MirShellSurfaceV1();

    /// Called by WindowWlSurfaceRole before surface is created in order to modify creation params
    void prepare_for_surface_creation(scene::SurfaceCreationParameters& creation_params);

    /// Returns the Mir shell surface created for the given wl_surface
    static auto from(WlSurface* surface) -> wayland::Weak<MirShellSurfaceV1>;

private:
    /// Wayland requests
    ///@{
    void destroy() override;
    void set_window_type(uint32_t type) override;
    ///@}

    MirWmBaseV1Global& global;
    wayland::Weak<WlSurface> const surface;
    WlSurface* const surface_unsafe; ///< Used for removal from the map

    std::experimental::optional<MirWindowType> original_type;
    std::experimental::optional<MirWindowType> override_type;

    static std::mutex surface_map_mutex; ///< Just in case there are multiple wayland displays and Wayland threads
    static std::unordered_map<WlSurface*, wayland::Weak<MirShellSurfaceV1>> surface_map;
};
}
}

#endif // MIR_FRONTEND_MIR_WINDOW_MANAGEMENT_V1_H
