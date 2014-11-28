/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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
 * Authored by:
 * Eleni Maria Stea <elenimaria.stea@canonical.com>
 * Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_GRAPHICS_MESA_NATIVE_PLATFORM_H_
#define MIR_GRAPHICS_MESA_NATIVE_PLATFORM_H_

#include "mir/graphics/native_platform.h"
#include "display_helpers.h"

namespace mir
{
namespace graphics
{
namespace mesa
{
class InternalNativeDisplay; 

class NativePlatform : public graphics::NativePlatform
{
public:
    NativePlatform(std::shared_ptr<NestedContext> const& nested_context_arg);
    virtual ~NativePlatform();

    std::shared_ptr<GraphicBufferAllocator> create_buffer_allocator() override;
    std::shared_ptr<graphics::BufferWriter> make_buffer_writer() override;
    std::shared_ptr<PlatformIpcOperations> make_ipc_operations() const override;
    
    static std::shared_ptr<InternalNativeDisplay> internal_native_display();
    static bool internal_native_display_in_use();

private:
    std::shared_ptr<NestedContext> nested_context;
    helpers::GBMHelper gbm;
    std::shared_ptr<PlatformIpcOperations> ipc_ops;

    static std::shared_ptr<InternalNativeDisplay> ensure_internal_native_display(std::shared_ptr<PlatformIPCPackage> const& package);
    static void finish_internal_native_display();
};
}
}
}

#endif // MIR_GRAPHICS_MESA_NATIVE_PLATFORM_H_
