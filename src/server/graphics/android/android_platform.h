/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_ANDROID_PLATFORM_H_
#define MIR_GRAPHICS_ANDROID_ANDROID_PLATFORM_H_

#include "mir/graphics/platform.h"
#include "mir/graphics/native_platform.h"

namespace mir
{
namespace graphics
{
class DisplayReport;
namespace android
{
class GraphicBufferAllocator;
class FramebufferFactory;
class DisplayBuilder;

class AndroidPlatform : public Platform, public NativePlatform
{
public:
    AndroidPlatform(
        std::shared_ptr<DisplayBuilder> const& display_builder,
        std::shared_ptr<DisplayReport> const& display_report);

    /* From Platform */
    std::shared_ptr<graphics::GraphicBufferAllocator> create_buffer_allocator(
            std::shared_ptr<BufferInitializer> const& buffer_initializer);
    std::shared_ptr<Display> create_display(
        std::shared_ptr<graphics::DisplayConfigurationPolicy> const&);
    std::shared_ptr<PlatformIPCPackage> get_ipc_package();
    std::shared_ptr<InternalClient> create_internal_client();
    void fill_ipc_package(BufferIPCPacker* packer, graphics::Buffer const* buffer) const;
    EGLNativeDisplayType egl_native_display() const;

private:
    std::shared_ptr<Display> create_fb_backup_display();

    void initialize(std::shared_ptr<NestedContext> const& nested_context) override;

    // TODO a design that has this and create_buffer_allocator is missing simplicity
    virtual std::shared_ptr<GraphicBufferAllocator> create_mga_buffer_allocator(
        const std::shared_ptr<BufferInitializer>& buffer_initializer);

    std::shared_ptr<DisplayBuilder> const display_builder;
    std::shared_ptr<DisplayReport> const display_report;
};

}
}
}
#endif /* MIR_GRAPHICS_ANDROID_ANDROID_PLATFORM_H_ */
