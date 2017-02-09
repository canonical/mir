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

#define MIR_LOG_COMPONENT "android extension"
#include "mir_native_window.h"
#include "android_format_conversion-inl.h"
#include "mir/client_context.h"
#include "mir/mir_buffer.h"
#include "mir/mir_render_surface.h"
#include "mir/client_buffer.h"
#include "mir/mir_buffer_stream.h"
#include "android_client_platform.h"
#include "gralloc_registrar.h"
#include "android_client_buffer_factory.h"
#include "egl_native_surface_interpreter.h"
#include "native_window_report.h"
#include "android_format_conversion-inl.h"

#include <chrono>
#include <hardware/gralloc.h>
#include "mir/weak_egl.h"
#include "mir_toolkit/mir_connection.h"
#include "mir/uncaught.h"
#include <EGL/egl.h>

#include <boost/throw_exception.hpp>

namespace mcl=mir::client;
namespace mcla=mir::client::android;
namespace mga=mir::graphics::android;

namespace
{
void* native_display_type(MirConnection*) noexcept
{
    static EGLNativeDisplayType type = EGL_DEFAULT_DISPLAY;
    return &type;
}

ANativeWindowBuffer* create_anwb(MirBuffer* b) noexcept
try
{
    auto buffer = reinterpret_cast<mcl::MirBuffer*>(b);
    auto native = mga::to_native_buffer_checked(buffer->client_buffer()->native_buffer_handle());
    return native->anwb();
}
catch (std::exception& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

void destroy_anwb(ANativeWindowBuffer*) noexcept
{
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
ANativeWindow* create_anw(
    MirRenderSurface* rs_key,
    int width, int height,
    unsigned int hal_pixel_format,
    unsigned int gralloc_usage_flags)
{
    auto format = mga::to_mir_format(hal_pixel_format);
    if (format == mir_pixel_format_invalid)
        return nullptr;

    //TODO: will be able to pass through the actual requested flags once buffers have generic flags.
    MirBufferUsage usage;
    if (gralloc_usage_flags & (GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK))
        usage = mir_buffer_usage_software;
    else if (gralloc_usage_flags == (GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER))
        usage = mir_buffer_usage_hardware;
    else
        return nullptr;

    auto rs = mcl::render_surface_lookup(rs_key);
    if (!rs)
        return nullptr;
    auto buffer_stream =  rs->get_buffer_stream(width, height, format, usage);
    return static_cast<ANativeWindow*>(buffer_stream->egl_native_window());
}
#pragma GCC diagnostic pop

void destroy_anw(ANativeWindow*)
{
}

int get_fence(MirBuffer* b) noexcept
try
{
    if (!b)
        std::abort();
    auto buffer = reinterpret_cast<mcl::MirBuffer*>(b);
    auto native = mga::to_native_buffer_checked(buffer->client_buffer()->native_buffer_handle());
    return native->fence();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return mir::Fd::invalid;
}

bool validate_access(MirBufferAccess access)
{
    return access == mir_none || access == mir_read || access == mir_read_write; 
}

void associate_fence(MirBuffer* b, int fence, MirBufferAccess access) noexcept
try
{
    if (!b || !validate_access(access))
        std::abort();
    auto buffer = reinterpret_cast<mcl::MirBuffer*>(b);
    auto native_buffer = mga::to_native_buffer_checked(buffer->client_buffer()->native_buffer_handle());

    mga::NativeFence f = fence;
    if (fence <= mir::Fd::invalid)
        native_buffer->reset_fence();
    else if (access == mir_read)
        native_buffer->update_usage(f, mga::BufferAccess::read); 
    else if (access == mir_read_write)
        native_buffer->update_usage(f, mga::BufferAccess::write); 
    else
        BOOST_THROW_EXCEPTION(std::invalid_argument("invalid MirBufferAccess"));
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

int wait_for_access(MirBuffer* b, MirBufferAccess access, int timeout) noexcept
try
{
    if (!b || !validate_access(access))
        std::abort();
    auto buffer = reinterpret_cast<mcl::MirBuffer*>(b);
    auto native_buffer = mga::to_native_buffer_checked(buffer->client_buffer()->native_buffer_handle());

    // could use std::chrono::floor once we're using C++17
    auto ns = std::chrono::nanoseconds(timeout);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(ns);
    if (ms > ns)
        ms = ms - std::chrono::milliseconds{1};

    bool rc = true;
    if (access == mir_read)
        rc = native_buffer->ensure_available_for(mga::BufferAccess::read, ms); 
    if (access == mir_read_write)
        rc = native_buffer->ensure_available_for(mga::BufferAccess::write, ms); 

    return rc;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return -1;
}

void create_buffer(
    MirConnection* connection,
    int width, int height,
    unsigned int hal_pixel_format,
    unsigned int gralloc_usage_flags,
    MirBufferCallback available_callback, void* available_context)
{
    auto context = mcl::to_client_context(connection);
    context->allocate_buffer(
        mir::geometry::Size{width, height}, hal_pixel_format, gralloc_usage_flags,
        available_callback, available_context); 
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
MirBufferStream* get_hw_stream(
    MirRenderSurface* rs_key,
    int width, int height,
    MirPixelFormat format)
{
    auto rs = mcl::render_surface_lookup(rs_key);
    if (!rs)
        return nullptr;
    return rs->get_buffer_stream(width, height, format, mir_buffer_usage_hardware);
}
#pragma GCC diagnostic pop

}

mcla::AndroidClientPlatform::AndroidClientPlatform(
    ClientContext* const context,
    std::shared_ptr<logging::Logger> const& logger) :
    context{context},
    logger{logger},
    native_display{std::make_shared<EGLNativeDisplayType>(EGL_DEFAULT_DISPLAY)},
    android_types_extension{native_display_type, create_anw, destroy_anw, create_anwb, destroy_anwb},
    fence_extension{get_fence, associate_fence, wait_for_access},
    buffer_extension{create_buffer},
    hw_stream{get_hw_stream}
{
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
    auto gralloc_dev = std::shared_ptr<gralloc_module_t>(gr_dev, [](auto){});
    auto registrar = std::make_shared<mcla::GrallocRegistrar>(gralloc_dev);
    return std::make_shared<mcla::AndroidClientBufferFactory>(registrar);
}

void mcla::AndroidClientPlatform::use_egl_native_window(std::shared_ptr<void> native_window, EGLNativeSurface* surface)
{
    auto anw = std::static_pointer_cast<mga::MirNativeWindow>(native_window);
    anw->use_native_surface(std::make_shared<mcla::EGLNativeSurfaceInterpreter>(*surface));
}

std::shared_ptr<void> mcla::AndroidClientPlatform::create_egl_native_window(EGLNativeSurface* surface)
{
    auto log = getenv("MIR_CLIENT_ANDROID_WINDOW_REPORT");
    std::shared_ptr<mga::NativeWindowReport> report;
    char const* on_val = "log";
    if (log && !strncmp(log, on_val, strlen(on_val)))
        report = std::make_shared<mga::ConsoleNativeWindowReport>(logger);
    else
        report = std::make_shared<mga::NullNativeWindowReport>();

    std::shared_ptr<mga::AndroidDriverInterpreter> surface_interpreter;
    if (surface)
        surface_interpreter = std::make_shared<mcla::EGLNativeSurfaceInterpreter>(*surface);
    else
        surface_interpreter = std::make_shared<mcla::ErrorDriverInterpreter>();

    return std::make_shared<mga::MirNativeWindow>(surface_interpreter, report);
}

std::shared_ptr<EGLNativeDisplayType>
mcla::AndroidClientPlatform::create_egl_native_display()
{
    return native_display;
}

MirPlatformType mcla::AndroidClientPlatform::platform_type() const
{
    return mir_platform_type_android;
}

void mcla::AndroidClientPlatform::populate(MirPlatformPackage& package) const
{
    context->populate_server_package(package);
}

MirPlatformMessage* mcla::AndroidClientPlatform::platform_operation(
    MirPlatformMessage const*)
{
    return nullptr;
}

MirNativeBuffer* mcla::AndroidClientPlatform::convert_native_buffer(graphics::NativeBuffer* buf) const
{
    return mga::to_native_buffer_checked(buf)->anwb();
}

MirPixelFormat mcla::AndroidClientPlatform::get_egl_pixel_format(
    EGLDisplay disp, EGLConfig conf) const
{
    MirPixelFormat mir_format = mir_pixel_format_invalid;

    // EGL_KHR_platform_android says this will always work...
    EGLint vis = 0;
    mcl::WeakEGL weak;
    if (weak.eglGetConfigAttrib(disp, conf, EGL_NATIVE_VISUAL_ID, &vis))
        mir_format = mir::graphics::android::to_mir_format(vis);

    return mir_format;
}

void* mcla::AndroidClientPlatform::request_interface(char const* name, int version)
{
    if (!strcmp(name, "mir_extension_android_egl") && (version == 1))
        return &android_types_extension;
    if (!strcmp(name, "mir_extension_android_buffer") && (version == 1))
        return &buffer_extension;
    if (!strcmp(name, "mir_extension_fenced_buffers") && version == 1)
        return &fence_extension;
    if (!strcmp(name, "mir_extension_hardware_buffer_stream") && (version == 1))
        return &hw_stream;
    return nullptr;
}

uint32_t mcla::AndroidClientPlatform::native_format_for(MirPixelFormat format) const
{
    return mga::to_android_format(format);
}

uint32_t mcla::AndroidClientPlatform::native_flags_for(MirBufferUsage usage, mir::geometry::Size) const
{
    return mga::convert_to_android_usage(static_cast<mir::graphics::BufferUsage>(usage));
}
