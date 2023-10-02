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
    : egl_provider{std::make_shared<InterfaceProvider>(x11_resources->xlib_dpy)},
      x11_resources{x11_resources},
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
        std::dynamic_pointer_cast<Platform>(shared_from_this()),
        x11_resources,
        title,
        output_sizes,
        initial_conf_policy,
        report);
}

class mgx::Platform::InterfaceProvider : public mg::DisplayInterfaceProvider
{
public:
    InterfaceProvider(::Display* x_dpy)
        : egl{std::make_shared<mgx::helpers::EGLHelper>(x_dpy)},
          connection{nullptr},
          win{std::nullopt}
    {
    }

    InterfaceProvider(
        InterfaceProvider const& from,
        std::shared_ptr<mir::X::X11Resources> connection,
        xcb_window_t win)
        : egl{from.egl},
          connection{std::move(connection)},
          win{win}
    {
    }

    class EGLDisplayProvider : public mg::GenericEGLDisplayProvider
    {
    public:
        EGLDisplayProvider(
            std::shared_ptr<mgx::helpers::EGLHelper> egl,
            std::shared_ptr<mir::X::X11Resources> connection,
            std::optional<xcb_window_t> x_win)
            : egl{std::move(egl)},
              connection{std::move(connection)},
              x_win{std::move(x_win)}
        {
        }

        auto get_egl_display() -> EGLDisplay
        {
            return egl->display();
        }

        auto alloc_framebuffer(mg::GLConfig const& config, EGLContext share_context)
            -> std::unique_ptr<mg::GenericEGLDisplayProvider::EGLFramebuffer>
        {        
            return egl->framebuffer_for_window(config, connection->conn->connection(), x_win.value(), share_context); 
        }    
    private:
        std::shared_ptr<mgx::helpers::EGLHelper> const egl;
        std::shared_ptr<mir::X::X11Resources> const connection;
        std::optional<xcb_window_t> const x_win;
    };

    auto maybe_create_interface(mir::graphics::DisplayProvider::Tag const& type_tag)
        -> std::shared_ptr<DisplayProvider>
    {
        if (dynamic_cast<mg::GenericEGLDisplayProvider::Tag const*>(&type_tag))
        {
            return std::make_shared<EGLDisplayProvider>(egl, connection, win);
        }
        return {};
    }

private:
    std::shared_ptr<mgx::helpers::EGLHelper> const egl;
    std::shared_ptr<mir::X::X11Resources> const connection;
    std::optional<xcb_window_t> const win;
};

auto mgx::Platform::interface_for() -> std::shared_ptr<DisplayInterfaceProvider>
{
    return egl_provider;
}

auto mgx::Platform::provider_for_window(xcb_window_t x_win) -> std::shared_ptr<DisplayInterfaceProvider>
{
    return std::make_shared<InterfaceProvider>(*egl_provider, x11_resources, x_win);
}