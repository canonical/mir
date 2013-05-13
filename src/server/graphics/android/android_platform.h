/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_ANDROID_PLATFORM_H_
#define MIR_GRAPHICS_ANDROID_ANDROID_PLATFORM_H_

#include "mir/graphics/platform.h"

namespace mir
{
namespace graphics
{
class DisplayReport;
namespace android
{

class AndroidPlatform : public Platform
{
public:
    /* From Platform */
    AndroidPlatform(std::shared_ptr<DisplayReport> const& display_report);

    std::shared_ptr<compositor::GraphicBufferAllocator> create_buffer_allocator(
            const std::shared_ptr<BufferInitializer>& buffer_initializer);
    std::shared_ptr<Display> create_display();
    std::shared_ptr<PlatformIPCPackage> get_ipc_package(); 
    std::shared_ptr<InternalClient> create_internal_client(std::shared_ptr<frontend::Surface> const&);

private:
    std::shared_ptr<DisplayReport> const display_report;
};

}
}
}
#endif /* MIR_GRAPHICS_ANDROID_ANDROID_PLATFORM_H_ */
