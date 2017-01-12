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

#ifndef MIR_CLIENT_NATIVE_ANDROID_EGL_NATIVE_SURFACE_INTERPRETER_H_
#define MIR_CLIENT_NATIVE_ANDROID_EGL_NATIVE_SURFACE_INTERPRETER_H_

#include "android_driver_interpreter.h"
#include "mir/egl_native_surface.h"
#include <boost/throw_exception.hpp>

namespace mir
{
namespace graphics
{
namespace android
{
class SyncFileOps;
}
}
namespace client
{
namespace android
{

class ErrorDriverInterpreter : public graphics::android::AndroidDriverInterpreter
{
public:
#define THROW_EXCEPTION \
{ \
    BOOST_THROW_EXCEPTION(std::logic_error("error: use_egl_native_window(...) has not yet been called"));\
}
    graphics::android::NativeBuffer* driver_requests_buffer() override THROW_EXCEPTION
    void driver_returns_buffer(ANativeWindowBuffer*, int) override THROW_EXCEPTION
    void dispatch_driver_request_format(int) override THROW_EXCEPTION
    void dispatch_driver_request_buffer_count(unsigned int) override THROW_EXCEPTION
    void dispatch_driver_request_buffer_size(geometry::Size) override THROW_EXCEPTION
    int  driver_requests_info(int) const override THROW_EXCEPTION
    void sync_to_display(bool) override THROW_EXCEPTION
#undef THROW_EXCEPTION
};

class EGLNativeSurfaceInterpreter : public graphics::android::AndroidDriverInterpreter
{
public:
    explicit EGLNativeSurfaceInterpreter(EGLNativeSurface& surface);

    graphics::android::NativeBuffer* driver_requests_buffer() override;
    void driver_returns_buffer(ANativeWindowBuffer*, int fence_fd) override;
    void dispatch_driver_request_format(int format) override;
    void dispatch_driver_request_buffer_count(unsigned int count) override;
    void dispatch_driver_request_buffer_size(geometry::Size size) override;
    int  driver_requests_info(int key) const override;
    void sync_to_display(bool) override;

private:
    EGLNativeSurface& surface;
    int driver_pixel_format;
    std::shared_ptr<graphics::android::SyncFileOps> const sync_ops;
    unsigned int const hardware_bits;
    unsigned int const software_bits;
};

}
}
}

#endif /* MIR_CLIENT_NATIVE_ANDROID_EGL_NATIVE_SURFACE_INTERPRETER_H_ */
