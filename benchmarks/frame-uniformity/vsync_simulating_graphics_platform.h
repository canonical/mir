/*
 * Copyright © 2014 Canonical Ltd.
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
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef VSYNC_SIMULATING_GRAPHICS_PLATFORM_H_
#define VSYNC_SIMULATING_GRAPHICS_PLATFORM_H_

#include "mir/graphics/platform.h"
#include "mir/geometry/rectangle.h"

#include "mir/test/doubles/null_platform.h"

class VsyncSimulatingPlatform : public mir::test::doubles::NullPlatform
{
public:
    VsyncSimulatingPlatform(mir::geometry::Size const& output_size, int vsync_rate_in_hz);
    ~VsyncSimulatingPlatform() = default;
    
    mir::UniqueModulePtr<mir::graphics::GraphicBufferAllocator> create_buffer_allocator(
        mir::graphics::Display const& output);
    
    mir::UniqueModulePtr<mir::graphics::Display> create_display(
        std::shared_ptr<mir::graphics::DisplayConfigurationPolicy> const& initial_conf_policy,
        std::shared_ptr<mir::graphics::GLConfig> const& gl_config);
    
    mir::UniqueModulePtr<mir::graphics::PlatformIpcOperations> make_ipc_operations() const;

private:
    mir::geometry::Size const output_size;
    int const vsync_rate_in_hz;
};

#endif // VSYNC_SIMULATING_GRAPHICS_PLATFORM_H_
