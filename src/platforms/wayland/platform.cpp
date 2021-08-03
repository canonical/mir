/*
 * Copyright © 2019 Canonical Ltd.
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

#include "mir/graphics/egl_error.h"

#include <boost/throw_exception.hpp>
#include <wayland-client.h>

namespace mg = mir::graphics;
namespace mgw = mir::graphics::wayland;
using namespace std::literals;

mgw::Platform::Platform(struct wl_display* const wl_display, std::shared_ptr<mg::DisplayReport> const& report) :
    wl_display{wl_display},
    report{report}
{
    if (!wl_display)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to connect to wayland"));
    }
}

mir::UniqueModulePtr<mg::Display> mgw::Platform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const&,
    std::shared_ptr<GLConfig> const& gl_config)
{
  return mir::make_module_ptr<mgw::Display>(wl_display, gl_config, report);
}
