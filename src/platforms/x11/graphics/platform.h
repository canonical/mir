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

#ifndef MIR_GRAPHICS_X_PLATFORM_H_
#define MIR_GRAPHICS_X_PLATFORM_H_

#include "mir/graphics/display_report.h"
#include "mir/graphics/platform.h"
#include "mir/geometry/size.h"

#include <xcb/xcb.h>
#include <mutex>

namespace mir
{
namespace X
{
class X11Resources;
}

namespace graphics
{
namespace X
{
struct X11OutputConfig
{
    inline X11OutputConfig(geometry::Size const& size)
        : size{size},
          scale{1.0f}
    {
    }

    inline X11OutputConfig(geometry::Size const& size, float scale)
        : size{size},
          scale{scale}
    {
    }

    ~X11OutputConfig() = default;

    geometry::Size size;
    float scale;
};

class Platform : public graphics::DisplayPlatform
{
public:
    // Parses colon separated list of sizes in the form WIDTHxHEIGHT^SCALE (^SCALE is optional)
    static auto parse_output_sizes(std::string output_sizes) -> std::vector<X11OutputConfig>;

    Platform(
        std::shared_ptr<mir::X::X11Resources> const& x11_resources,
        std::string const title,
        std::vector<X11OutputConfig> output_sizes,
        std::shared_ptr<DisplayReport> const& report);
    ~Platform() = default;

    /* From Platform */
    auto create_display(
        std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
        std::shared_ptr<GLConfig> const& gl_config) -> UniqueModulePtr<graphics::Display> override;

    auto display_target_for_window(xcb_window_t x_win) -> std::shared_ptr<DisplayTarget>;
protected:
    auto target() -> std::shared_ptr<DisplayTarget> override;

private:
    class X11DisplayTarget;
    std::shared_ptr<X11DisplayTarget> const x11_display_target;
    std::shared_ptr<mir::X::X11Resources> const x11_resources;
    std::string const title;
    std::shared_ptr<DisplayReport> const report;
    std::vector<X11OutputConfig> const output_sizes;
};
}
}
}
#endif /* MIR_GRAPHICS_X_PLATFORM_H_ */
