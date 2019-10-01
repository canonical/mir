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
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "platform.h"

#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/display.h"
#include "mir/graphics/platform_ipc_operations.h"

namespace mg = mir::graphics;

mg::rpi::Platform::Platform()
    : display_platform{std::make_unique<rpi::DisplayPlatform>()},
      render_platform{std::make_unique<rpi::RenderingPlatform>()}
{
}

auto mg::rpi::Platform::create_buffer_allocator(Display const &output)
    -> UniqueModulePtr<GraphicBufferAllocator>
{
    return render_platform->create_buffer_allocator(output);
}

auto mg::rpi::Platform::make_ipc_operations() const -> UniqueModulePtr<PlatformIpcOperations>
{
    return render_platform->make_ipc_operations();
}

auto mg::rpi::Platform::native_rendering_platform() ->NativeRenderingPlatform *
{
    return render_platform->native_rendering_platform();
}

auto mg::rpi::Platform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const &initial_conf_policy,
    std::shared_ptr<GLConfig> const &gl_config) -> UniqueModulePtr<Display>
{
    return display_platform->create_display(initial_conf_policy, gl_config);
}

auto mg::rpi::Platform::native_display_platform() -> NativeDisplayPlatform *
{
    return display_platform->native_display_platform();
}

auto mg::rpi::Platform::extensions() const -> std::vector<ExtensionDescription>
{
    return display_platform->extensions();
}

