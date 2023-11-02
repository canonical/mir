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

#include "platform.h"
#include "display.h"
#include "display_buffer.h"
#include "egl_helper.h"
#include "mir/graphics/egl_error.h"
#include "mir/graphics/platform.h"
#include "mir/options/option.h"
#include "options_parsing_helpers.h"
#include <optional>


namespace mg = mir::graphics;
namespace mgx = mg::X;
namespace geom = mir::geometry;

namespace
{
auto parse_size(std::string const& str) -> mgx::X11OutputConfig
{
    auto parsed = mir::graphics::common::parse_size_with_scale(str);
    return mgx::X11OutputConfig{std::get<0>(parsed), std::get<1>(parsed)};
}
}

auto mgx::Platform::parse_output_sizes(std::string output_sizes) -> std::vector<mgx::X11OutputConfig>
{
    std::vector<mgx::X11OutputConfig> sizes;
    for (int start = 0, end; start - 1 < (int)output_sizes.size(); start = end + 1)
    {
        end = output_sizes.find(':', start);
        if (end == (int)std::string::npos)
            end = output_sizes.size();
        sizes.push_back(parse_size(output_sizes.substr(start, end - start)));
    }
    return sizes;
}

mgx::Platform::Platform(std::shared_ptr<mir::X::X11Resources> const& x11_resources,
                        std::string title,
                        std::vector<X11OutputConfig> output_sizes,
                        std::shared_ptr<mg::DisplayReport> const& report)
    : x11_resources{x11_resources},
      egl{std::make_shared<helpers::EGLHelper>(x11_resources->xlib_dpy)},
      title{std::move(title)},
      report{report},
      output_sizes{std::move(output_sizes)}
{
}

mir::UniqueModulePtr<mg::Display> mgx::Platform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
    std::shared_ptr<GLConfig> const& /*gl_config*/)
{
    return make_module_ptr<mgx::Display>(
        x11_resources,
        egl,
        title,
        output_sizes,
        initial_conf_policy,
        report);
}

namespace
{
class EGLProvider : public mg::GenericEGLDisplayProvider
{
public:
    explicit EGLProvider(EGLDisplay dpy)
        : dpy{dpy}
    {
    }

    auto get_egl_display() -> EGLDisplay override
    {
        return dpy;
    }
private:
    EGLDisplay const dpy;
};
}

auto mgx::Platform::maybe_create_provider(DisplayProvider::Tag const& type_tag) -> std::shared_ptr<DisplayProvider>
{
    if (dynamic_cast<GenericEGLDisplayProvider::Tag const*>(&type_tag))
    {
        return std::make_shared<EGLProvider>(egl->display());
    }
    return nullptr;
}
