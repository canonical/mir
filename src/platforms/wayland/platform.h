/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_GRAPHICS_WAYLAND_PLATFORM_H_
#define MIR_GRAPHICS_WAYLAND_PLATFORM_H_

#include <mir/graphics/platform.h>
#include <mir/graphics/display.h>
#include <mir/geometry/size.h>

#include <wayland-client.h>
#include <wayland-egl.h>

#include <vector>
#include <string>

namespace mir
{
namespace graphics
{
namespace wayland
{
class WlDisplayProvider;

struct WaylandOutputConfig
{
    explicit WaylandOutputConfig(geometry::Size const& size, float scale = 1.0f)
        : size{size},
          scale{scale}
    {
    }

    geometry::Size size;
    float scale;
};

class Platform : public graphics::DisplayPlatform
{
public:
    // Parses a list of output specs in the form WIDTHxHEIGHT[^SCALE] (^SCALE is optional)
    static auto parse_output_sizes(std::vector<std::string> const& outputs) -> std::vector<WaylandOutputConfig>;

    Platform(struct wl_display* const wl_display,
        std::shared_ptr<DisplayReport> const& report,
        std::optional<std::string> const& app_id,
        std::optional<std::string> const& title,
        std::vector<WaylandOutputConfig> output_configs = {});
    ~Platform() = default;

    UniqueModulePtr<Display> create_display(
        std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
        std::shared_ptr<GLConfig> const& gl_config) override;

protected:
    auto maybe_create_provider(DisplayProvider::Tag const& type_tag) -> std::shared_ptr<DisplayProvider> override;
private:

    struct wl_display* const wl_display;
    std::shared_ptr<DisplayReport> const report;
    std::optional<std::string> const app_id;
    std::optional<std::string> const title;
    std::vector<WaylandOutputConfig> const output_configs;

    std::shared_ptr<WlDisplayProvider> const provider;
};
}
}
}

#endif // MIR_GRAPHICS_WAYLAND_PLATFORM_H_
