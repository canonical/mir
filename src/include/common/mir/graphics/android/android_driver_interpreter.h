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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_DRIVER_INTERPRETER_H_
#define MIR_GRAPHICS_ANDROID_DRIVER_INTERPRETER_H_

#include "mir/graphics/android/native_buffer.h"
#include <system/window.h>
#include <memory>

namespace mir
{
namespace graphics
{
namespace android
{
class AndroidDriverInterpreter
{
public:
    virtual NativeBuffer* driver_requests_buffer() = 0;
    virtual void driver_returns_buffer(ANativeWindowBuffer*, int fence) = 0;
    virtual void dispatch_driver_request_format(int format) = 0;
    virtual void dispatch_driver_request_buffer_count(unsigned int count) = 0;
    virtual int driver_requests_info(int key) const = 0;
    virtual void sync_to_display(bool sync) = 0;
protected:
    AndroidDriverInterpreter() {};
    virtual ~AndroidDriverInterpreter() {};
    AndroidDriverInterpreter(AndroidDriverInterpreter const&) = delete;
    AndroidDriverInterpreter& operator=(AndroidDriverInterpreter const&) = delete;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_DRIVER_INTERPRETER_H_ */
