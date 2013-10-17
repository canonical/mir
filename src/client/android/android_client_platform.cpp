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
 * Authored by: Kevin DuBois<kevin.dubois@canonical.com>
 */

#include "mir/graphics/android/mir_native_window.h"
#include "android_client_platform.h"
#include "android_registrar_gralloc.h"
#include "android_client_buffer_factory.h"
#include "client_surface_interpreter.h"
#include "../mir_connection.h"
#include "../native_client_platform_factory.h"

#include <EGL/egl.h>

#include <boost/throw_exception.hpp>

namespace mcl=mir::client;
namespace mcla=mir::client::android;
namespace mga=mir::graphics::android;

namespace
{

struct EmptyDeleter
{
    void operator()(void*)
    {
    }
};

}

std::shared_ptr<mcl::ClientPlatform>
mcl::NativeClientPlatformFactory::create_client_platform(mcl::ClientContext* /*context*/)
{
    return std::make_shared<mcla::AndroidClientPlatform>();
}

std::shared_ptr<mcl::ClientBufferFactory> mcla::AndroidClientPlatform::create_buffer_factory()
{
    const hw_module_t *hw_module;
    int error = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &hw_module);
    if (error < 0)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Could not open hardware module"));
    }

    gralloc_module_t* gr_dev = (gralloc_module_t*) hw_module;
    /* we use an empty deleter because hw_get_module does not give us the ownership of the ptr */
    EmptyDeleter empty_del;
    auto gralloc_dev = std::shared_ptr<gralloc_module_t>(gr_dev, empty_del);
    auto registrar = std::make_shared<mcla::AndroidRegistrarGralloc>(gralloc_dev);
    return std::make_shared<mcla::AndroidClientBufferFactory>(registrar);
}

namespace
{
struct MirNativeWindowDeleter
{
    MirNativeWindowDeleter(mga::MirNativeWindow* window)
     : window(window) {}

    void operator()(EGLNativeWindowType* type)
    {
        delete type;
        delete window;
    }

private:
    mga::MirNativeWindow *window;
};
}

std::shared_ptr<EGLNativeWindowType> mcla::AndroidClientPlatform::create_egl_native_window(ClientSurface *surface)
{
    auto anativewindow_interpreter = std::make_shared<mcla::ClientSurfaceInterpreter>(*surface);
    auto mir_native_window = new mga::MirNativeWindow(anativewindow_interpreter);
    auto egl_native_window = new EGLNativeWindowType;
    *egl_native_window = mir_native_window;
    MirNativeWindowDeleter deleter = MirNativeWindowDeleter(mir_native_window);
    return std::shared_ptr<EGLNativeWindowType>(egl_native_window, deleter);
}

std::shared_ptr<EGLNativeDisplayType>
mcla::AndroidClientPlatform::create_egl_native_display()
{
    auto native_display = std::make_shared<EGLNativeDisplayType>();
    *native_display = EGL_DEFAULT_DISPLAY;
    return native_display;
}

MirPlatformType mcla::AndroidClientPlatform::platform_type() const
{
    return mir_platform_type_android; 
}
 
MirNativeBuffer* mcla::AndroidClientPlatform::convert_native_buffer(graphics::NativeBuffer* buf) const
{
    return buf->anwb();
}
