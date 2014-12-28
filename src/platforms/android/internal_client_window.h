/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_INTERNAL_CLIENT_WINDOW_H_
#define MIR_GRAPHICS_ANDROID_INTERNAL_CLIENT_WINDOW_H_

#include "mir/graphics/android/android_driver_interpreter.h"
#include "mir/geometry/size.h"
#include "mir_toolkit/common.h"

#include <memory>
#include <unordered_map>

namespace mir
{
namespace graphics
{
class Buffer;
class InternalSurface;
class NativeBuffer;

namespace android
{
class InternalClientWindow : public AndroidDriverInterpreter
{
public:
    InternalClientWindow(std::shared_ptr<InternalSurface> const&);
    graphics::NativeBuffer* driver_requests_buffer();
    void driver_returns_buffer(ANativeWindowBuffer*, int);
    void dispatch_driver_request_format(int);
    int  driver_requests_info(int) const;
    void sync_to_display(bool sync);

private:
    std::shared_ptr<InternalSurface> const surface;
    graphics::Buffer* buffer;
    struct Item
    {
        graphics::Buffer* buffer;
        std::shared_ptr<graphics::NativeBuffer> handle;
    };
    std::unordered_map<ANativeWindowBuffer*, Item> lookup;
    int format;
};
}
}
}
#endif /* MIR_GRAPHICS_ANDROID_INTERNAL_CLIENT_WINDOW_H_ */
