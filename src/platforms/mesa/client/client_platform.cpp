/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Kevin DuBois<kevin.dubois@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include "client_platform.h"
#include "client_buffer_factory.h"
#include "mesa_native_display_container.h"
#include "native_surface.h"
#include "mir/client/client_buffer_factory.h"
#include "mir/client/client_context.h"
#include "mir/client/client_buffer.h"
#include "mir/mir_render_surface.h"
#include "mir/mir_buffer.h"
#include "mir/weak_egl.h"
#include "mir/platform_message.h"
#include "mir_toolkit/mesa/platform_operation.h"
#include "native_buffer.h"
#include "gbm_format_conversions.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <cstring>
#include <mutex>
#include <condition_variable>

namespace mgm=mir::graphics::mesa;
namespace mcl=mir::client;
namespace mclm=mir::client::mesa;
namespace geom=mir::geometry;

namespace
{

struct NativeDisplayDeleter
{
    NativeDisplayDeleter(mcl::EGLNativeDisplayContainer& container)
    : container(container)
    {
    }
    void operator() (EGLNativeDisplayType* p)
    {
        auto display = *(reinterpret_cast<MirEGLNativeDisplayType*>(p));
        container.release(display);
        delete p;
    }

    mcl::EGLNativeDisplayContainer& container;
};

constexpr size_t division_ceiling(size_t a, size_t b)
{
    return ((a - 1) / b) + 1;
}

struct AuthFdContext
{
    MirAuthFdCallback cb;
    void* context;
};

void auth_fd_cb(
    MirConnection*, MirPlatformMessage* reply, void* context)
{
    AuthFdContext* ctx = reinterpret_cast<AuthFdContext*>(context);
    int auth_fd{-1};

    if (reply->fds.size() == 1)
        auth_fd = reply->fds[0];
    ctx->cb(auth_fd, ctx->context);
    delete ctx;
}

void auth_fd_ext(MirConnection* conn, MirAuthFdCallback cb, void* context)
{
    auto connection = reinterpret_cast<mcl::ClientContext*>(conn);
    MirPlatformMessage msg(MirMesaPlatformOperation::auth_fd);
    auto ctx = new AuthFdContext{cb, context};
    connection->platform_operation(&msg, auth_fd_cb, ctx);
}

struct AuthMagicContext
{
    MirAuthMagicCallback cb;
    void* context;
};

void auth_magic_cb(MirConnection*, MirPlatformMessage* reply, void* context)
{
    AuthMagicContext* ctx = reinterpret_cast<AuthMagicContext*>(context);
    int auth_magic_response{-1};
    if (reply->data.size() == sizeof(auth_magic_response))
        memcpy(&auth_magic_response, reply->data.data(), sizeof(auth_magic_response));
    ctx->cb(auth_magic_response, ctx->context);
    delete ctx;
}

void auth_magic_ext(MirConnection* conn, int magic, MirAuthMagicCallback cb, void* context)
{
    auto connection = reinterpret_cast<mcl::ClientContext*>(conn);
    MirPlatformMessage msg(MirMesaPlatformOperation::auth_magic);
    auto m = reinterpret_cast<char*>(&magic);
    msg.data.assign(m, m + sizeof(magic));
    auto ctx = new AuthMagicContext{cb, context};
    connection->platform_operation(&msg, auth_magic_cb, ctx);
}

void set_device(gbm_device* device, void* context)
{
    auto platform = reinterpret_cast<mclm::ClientPlatform*>(context);
    platform->set_gbm_device(device);
}

void allocate_buffer_gbm(
    MirConnection* connection,
    uint32_t width, uint32_t height,
    uint32_t gbm_pixel_format,
    uint32_t gbm_bo_flags,
    MirBufferCallback available_callback, void* available_context)
{
    auto context = mcl::to_client_context(connection);
    context->allocate_buffer(
        mir::geometry::Size{width, height}, gbm_pixel_format, gbm_bo_flags,
        available_callback, available_context); 
}

void allocate_buffer_gbm_legacy(
    MirConnection* connection,
    int width, int height,
    unsigned int gbm_pixel_format,
    unsigned int gbm_bo_flags,
    MirBufferCallback available_callback, void* available_context)
{
    allocate_buffer_gbm(
        connection, static_cast<uint32_t>(width), static_cast<uint32_t>(height),
        static_cast<uint32_t>(gbm_pixel_format), static_cast<uint32_t>(gbm_bo_flags),
        available_callback, available_context);
}

namespace
{
    struct BufferSync
    {
        void set_buffer(MirBuffer* b)
        {
            std::unique_lock<decltype(mutex)> lk(mutex);
            buffer = b;
            cv.notify_all();
        }

        MirBuffer* wait_for_buffer()
        {
            std::unique_lock<decltype(mutex)> lk(mutex);
            cv.wait(lk, [this]{ return buffer; });
            return buffer;
        }
    private:
        std::mutex mutex;
        std::condition_variable cv;
        MirBuffer* buffer = nullptr;
    };

    void allocate_buffer_gbm_sync_cb(MirBuffer* b, void* context)
    {
        reinterpret_cast<BufferSync*>(context)->set_buffer(b);
    }
}

MirBuffer* allocate_buffer_gbm_sync(
    MirConnection* connection,
    uint32_t width, uint32_t height,
    uint32_t gbm_pixel_format,
    uint32_t gbm_bo_flags)
try
{
    BufferSync sync;
    allocate_buffer_gbm(
        connection, width, height, gbm_pixel_format, gbm_bo_flags,
        &allocate_buffer_gbm_sync_cb, &sync);
    return sync.wait_for_buffer();
}
catch (...)
{
    return nullptr;
}

bool is_gbm_importable(MirBuffer const* b)
try
{
    if (!b)
        return false;
    auto const buffer = reinterpret_cast<mcl::MirBuffer const*>(b);
    auto native = dynamic_cast<mgm::NativeBuffer*>(buffer->client_buffer()->native_buffer_handle().get());
    if (!native)
        return false;
    return native->is_gbm_buffer;
}
catch (...)
{
    return false;
}

int import_fd(MirBuffer const* b)
try
{
    if (!is_gbm_importable(b))
        return -1;
    auto const buffer = reinterpret_cast<mcl::MirBuffer const*>(b);
    auto native = dynamic_cast<mgm::NativeBuffer*>(buffer->client_buffer()->native_buffer_handle().get());
    return native->fd[0];
}
catch (...)
{
    return -1;
}

uint32_t buffer_stride(MirBuffer const* b)
try
{
    if (!is_gbm_importable(b))
        return 0;
    auto const buffer = reinterpret_cast<mcl::MirBuffer const*>(b);
    auto native = dynamic_cast<mgm::NativeBuffer*>(buffer->client_buffer()->native_buffer_handle().get());
    return native->stride;
}
catch (...)
{
    return 0;
}

uint32_t buffer_format(MirBuffer const* b)
try
{
    if (!is_gbm_importable(b))
        return 0;
    auto const buffer = reinterpret_cast<mcl::MirBuffer const*>(b);
    auto native = dynamic_cast<mgm::NativeBuffer*>(buffer->client_buffer()->native_buffer_handle().get());
    return native->native_format;
}
catch (...)
{
    return 0;
}

uint32_t buffer_flags(MirBuffer const* b)
try
{
    if (!is_gbm_importable(b))
        return 0;
    auto const buffer = reinterpret_cast<mcl::MirBuffer const*>(b);
    auto native = dynamic_cast<mgm::NativeBuffer*>(buffer->client_buffer()->native_buffer_handle().get());
    return native->native_flags;
}
catch (...)
{
    return 0;
}

unsigned int buffer_age(MirBuffer const* b)
try
{
    if (!is_gbm_importable(b))
        return 0;
    auto const buffer = reinterpret_cast<mcl::MirBuffer const*>(b);
    auto native = dynamic_cast<mgm::NativeBuffer*>(buffer->client_buffer()->native_buffer_handle().get());
    return native->age;
}
catch (...)
{
    return 0;
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

void mclm::ClientPlatform::set_gbm_device(gbm_device* device)
{
    gbm_dev = device;
}

mclm::ClientPlatform::ClientPlatform(
        ClientContext* const context,
        std::shared_ptr<BufferFileOps> const& buffer_file_ops,
        mcl::EGLNativeDisplayContainer& display_container)
    : context{context},
      buffer_file_ops{buffer_file_ops},
      display_container(display_container),
      gbm_dev{nullptr},
      drm_extensions{auth_fd_ext, auth_magic_ext},
      mesa_auth{set_device, this},
      gbm_buffer1{allocate_buffer_gbm_legacy},
      gbm_buffer2{allocate_buffer_gbm, allocate_buffer_gbm_sync,
                 is_gbm_importable, import_fd, buffer_stride, buffer_format, buffer_flags, buffer_age},
      hw_stream{get_hw_stream}
{
}

std::shared_ptr<mcl::ClientBufferFactory> mclm::ClientPlatform::create_buffer_factory()
{
    return std::make_shared<mclm::ClientBufferFactory>(buffer_file_ops);
}

void mclm::ClientPlatform::use_egl_native_window(std::shared_ptr<void> native_window, EGLNativeSurface* surface)
{
    auto mnw = std::static_pointer_cast<NativeSurface>(native_window);
    mnw->use_native_surface(surface);
}

std::shared_ptr<void> mclm::ClientPlatform::create_egl_native_window(EGLNativeSurface* client_surface)
{
    return std::make_shared<NativeSurface>(client_surface);
}

std::shared_ptr<EGLNativeDisplayType> mclm::ClientPlatform::create_egl_native_display()
{
    MirEGLNativeDisplayType *mir_native_display = new MirEGLNativeDisplayType;
    *mir_native_display = display_container.create(this);
    auto egl_native_display = reinterpret_cast<EGLNativeDisplayType*>(mir_native_display);

    return std::shared_ptr<EGLNativeDisplayType>(egl_native_display, NativeDisplayDeleter(display_container));
}

MirPlatformType mclm::ClientPlatform::platform_type() const
{
    return mir_platform_type_gbm;
}

void mclm::ClientPlatform::populate(MirPlatformPackage& package) const
{
    size_t constexpr pointer_size_in_ints = division_ceiling(sizeof(gbm_dev), sizeof(int));

    context->populate_server_package(package);

    auto const total_data_size = package.data_items + pointer_size_in_ints;
    if (gbm_dev && total_data_size <= mir_platform_package_max)
    {
        int gbm_ptr[pointer_size_in_ints]{};
        std::memcpy(&gbm_ptr, &gbm_dev, sizeof(gbm_dev));

        for (auto i : gbm_ptr)
            package.data[package.data_items++] = i;
    }
}

MirPlatformMessage* mclm::ClientPlatform::platform_operation(
    MirPlatformMessage const* msg)
{
    if (msg->opcode == MirMesaPlatformOperation::set_gbm_device &&
        msg->data.size() == sizeof(MirMesaSetGBMDeviceRequest))
    {
        MirMesaSetGBMDeviceRequest set_gbm_device_request{nullptr};
        std::memcpy(&set_gbm_device_request, msg->data.data(), msg->data.size());

        gbm_dev = set_gbm_device_request.device;

        static int const success{0};
        MirMesaSetGBMDeviceResponse response{success};

        auto response_msg = new MirPlatformMessage(msg->opcode);
        auto r = reinterpret_cast<char*>(&response);
        response_msg->data.assign(r, r + sizeof(response));

        return response_msg;
    }

    return nullptr;
}

MirNativeBuffer* mclm::ClientPlatform::convert_native_buffer(graphics::NativeBuffer* buf) const
{
    if (auto native = dynamic_cast<mir::graphics::mesa::NativeBuffer*>(buf))
        return native;
    BOOST_THROW_EXCEPTION(std::invalid_argument("could not convert NativeBuffer"));
}


MirPixelFormat mclm::ClientPlatform::get_egl_pixel_format(
    EGLDisplay disp, EGLConfig conf) const
{
    MirPixelFormat mir_format = mir_pixel_format_invalid;

    /*
     * This is based on gbm_dri_is_format_supported() however we can't call it
     * via the public API gbm_device_is_format_supported because that is
     * too buggy right now (LP: #1473901).
     *
     * Ideally Mesa should implement EGL_NATIVE_VISUAL_ID for all platforms
     * to explicitly return the exact GBM pixel format. But it doesn't do that
     * yet (for most platforms). It does however successfully return zero for
     * EGL_NATIVE_VISUAL_ID, so ignore that for now.
     */
    EGLint r = 0, g = 0, b = 0, a = 0;
    mcl::WeakEGL weak;
    weak.eglGetConfigAttrib(disp, conf, EGL_RED_SIZE, &r);
    weak.eglGetConfigAttrib(disp, conf, EGL_GREEN_SIZE, &g);
    weak.eglGetConfigAttrib(disp, conf, EGL_BLUE_SIZE, &b);
    weak.eglGetConfigAttrib(disp, conf, EGL_ALPHA_SIZE, &a);

    if (r == 8 && g == 8 && b == 8)
    {
        // GBM is very limited, which at least makes this simple...
        if (a == 8)
            mir_format = mir_pixel_format_argb_8888;
        else if (a == 0)
            mir_format = mir_pixel_format_xrgb_8888;
    }

    return mir_format;
}

void* mclm::ClientPlatform::request_interface(char const* extension_name, int version)
{
    if (!strcmp("mir_extension_mesa_drm_auth", extension_name) && (version == 1))
        return &drm_extensions;
    if (!strcmp(extension_name, "mir_extension_set_gbm_device") && (version == 1))
        return &mesa_auth;
    if (!strcmp(extension_name, "mir_extension_gbm_buffer") && (version == 1))
        return &gbm_buffer1;
    if (!strcmp(extension_name, "mir_extension_gbm_buffer") && (version == 2))
        return &gbm_buffer2;
    if (!strcmp(extension_name, "mir_extension_hardware_buffer_stream") && (version == 1))
        return &hw_stream;

    return nullptr;
}

uint32_t mclm::ClientPlatform::native_format_for(MirPixelFormat format) const
{
    return mgm::mir_format_to_gbm_format(format);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
uint32_t mclm::ClientPlatform::native_flags_for(MirBufferUsage, mir::geometry::Size size) const
{
#pragma GCC diagnostic pop

    uint32_t bo_flags{GBM_BO_USE_RENDERING};
    if (size.width.as_uint32_t() >= 800 && size.height.as_uint32_t() >= 600)
        bo_flags |= GBM_BO_USE_SCANOUT;
    return bo_flags;
}
