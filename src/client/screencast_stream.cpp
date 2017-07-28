/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
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

#include "screencast_stream.h"
#include "make_protobuf_object.h"
#include "mir_connection.h"
#include "rpc/mir_display_server.h"
#include "mir_protobuf.pb.h"
#include "buffer_vault.h"
#include "protobuf_to_native_buffer.h"

#include "mir/log.h"
#include "mir/client/client_platform.h"
#include "mir/client/client_buffer_factory.h"
#include "mir/frontend/client_constants.h"
#include "mir_toolkit/mir_native_buffer.h"

#include <boost/throw_exception.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include <stdexcept>

namespace mcl = mir::client;
namespace mclr = mir::client::rpc;
namespace mf = mir::frontend;
namespace ml = mir::logging;
namespace mp = mir::protobuf;
namespace geom = mir::geometry;
namespace mf = mir::frontend;

namespace gp = google::protobuf;

mcl::ScreencastStream::ScreencastStream(
    MirConnection* connection,
    mclr::DisplayServer& server,
    std::shared_ptr<mcl::ClientPlatform> const& client_platform,
    mp::BufferStream const& a_protobuf_bs) :
    connection_(connection),
    display_server(server),
    client_platform(client_platform),
    factory(client_platform->create_buffer_factory()),
    protobuf_bs{mcl::make_protobuf_object<mir::protobuf::BufferStream>(a_protobuf_bs)},
    protobuf_void{mcl::make_protobuf_object<mir::protobuf::Void>()}
{
    if (!protobuf_bs->has_id())
    {
        if (!protobuf_bs->has_error())
            protobuf_bs->set_error("Error processing buffer stream create response, no ID (disconnected?)");
    }
    if (!protobuf_bs->has_buffer())
        protobuf_bs->set_error("Error processing buffer stream create response, no buffer");
    if (protobuf_bs->has_error())
        BOOST_THROW_EXCEPTION(std::runtime_error("Can not create buffer stream: " + std::string(protobuf_bs->error())));

    buffer_size = geom::Size{protobuf_bs->buffer().width(), protobuf_bs->buffer().height()};
    process_buffer(protobuf_bs->buffer());
    egl_native_window_ = client_platform->create_egl_native_window(this);

    if (!valid())
        BOOST_THROW_EXCEPTION(std::runtime_error("Can not create buffer stream: " + std::string(protobuf_bs->error())));
}


void mcl::ScreencastStream::process_buffer(mp::Buffer const& buffer)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    process_buffer(buffer, lock);
}

void mcl::ScreencastStream::process_buffer(protobuf::Buffer const& buffer, std::unique_lock<std::mutex>&)
{
    if (buffer.has_error())
    {
        for (int i = 0; i < buffer.fd_size(); i++)
            ::close(buffer.fd(i));
        if (screencast_wait_handle.is_pending())
            screencast_wait_handle.result_received();
        BOOST_THROW_EXCEPTION(std::runtime_error("BufferStream received buffer with error:" + buffer.error()));
    }

    if (buffer.has_width() && buffer.has_height())
        buffer_size = geom::Size{buffer.width(), buffer.height()};

    try
    {
        auto const pixel_format = static_cast<MirPixelFormat>(protobuf_bs->pixel_format());
        if (current_buffer_id != buffer.buffer_id())
        {
            current_buffer = factory->create_buffer(
                mcl::protobuf_to_native_buffer(buffer), buffer_size, pixel_format);
            current_buffer_id = buffer.buffer_id();
        }
    }
    catch (const std::runtime_error& error)
    {
        for (int i = 0; i < buffer.fd_size(); i++)
            ::close(buffer.fd(i));
        protobuf_bs->set_error(
            std::string{"Error processing buffer stream creating response:"} +
            boost::diagnostic_information(error));
        if (screencast_wait_handle.is_pending())
            screencast_wait_handle.result_received();
        throw error;
    }
}

MirWaitHandle* mcl::ScreencastStream::swap_buffers(std::function<void()> const& done)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    secured_region.reset();

    mp::ScreencastId screencast_id;
    screencast_id.set_value(protobuf_bs->id().value());

    lock.unlock();
    screencast_wait_handle.expect_result();

    display_server.screencast_buffer(
        &screencast_id,
        protobuf_bs->mutable_buffer(),
        google::protobuf::NewCallback(
        this, &mcl::ScreencastStream::screencast_buffer_received,
        done));

    return &screencast_wait_handle;
}

std::shared_ptr<mcl::ClientBuffer> mcl::ScreencastStream::get_current_buffer()
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    return current_buffer;
}

EGLNativeWindowType mcl::ScreencastStream::egl_native_window()
{
    return static_cast<EGLNativeWindowType>(egl_native_window_.get());
}

std::shared_ptr<mcl::MemoryRegion> mcl::ScreencastStream::secure_for_cpu_write()
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    if (!secured_region)
        secured_region = current_buffer->secure_for_cpu_write();
    return secured_region;
}

void mcl::ScreencastStream::screencast_buffer_received(std::function<void()> done)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    process_buffer(protobuf_bs->buffer(), lock);
    lock.unlock();

    done();
    screencast_wait_handle.result_received();
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
MirWindowParameters mcl::ScreencastStream::get_parameters() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    return MirWindowParameters{
        "Screencast",
        buffer_size.width.as_int(),
        buffer_size.height.as_int(),
        static_cast<MirPixelFormat>(protobuf_bs->pixel_format()),
        static_cast<MirBufferUsage>(protobuf_bs->buffer_usage()),
        mir_display_output_id_invalid};
}
#pragma GCC diagnostic pop

void mcl::ScreencastStream::swap_buffers_sync()
{
    swap_buffers([](){})->wait_for_all();
}

uint32_t mcl::ScreencastStream::get_current_buffer_id()
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    return current_buffer_id;
}

int mcl::ScreencastStream::swap_interval() const
{
    return swap_interval_;
}

MirNativeBuffer* mcl::ScreencastStream::get_current_buffer_package()
{
    auto buffer = get_current_buffer();
    auto handle = buffer->native_buffer_handle();
    return client_platform->convert_native_buffer(handle.get());
}

MirPlatformType mcl::ScreencastStream::platform_type()
{
    return client_platform->platform_type();
}

mf::BufferStreamId mcl::ScreencastStream::rpc_id() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    return mf::BufferStreamId(protobuf_bs->id().value());
}

bool mcl::ScreencastStream::valid() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    return protobuf_bs->has_id() && !protobuf_bs->has_error();
}

void mcl::ScreencastStream::buffer_available(mir::protobuf::Buffer const& buffer)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    process_buffer(buffer, lock);
}

void mcl::ScreencastStream::buffer_unavailable()
{
}

char const * mcl::ScreencastStream::get_error_message() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    if (protobuf_bs->has_error())
        return protobuf_bs->error().c_str();
    return error_message.c_str();
}

MirConnection* mcl::ScreencastStream::connection() const
{
    return connection_;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
MirRenderSurface* mcl::ScreencastStream::render_surface() const
{
    return nullptr;
}
#pragma GCC diagnostic pop

void mcl::ScreencastStream::set_buffer_cache_size(unsigned int)
{
    BOOST_THROW_EXCEPTION(std::logic_error("Attempt to set cache size on screencast is invalid"));
}

void mcl::ScreencastStream::request_and_wait_for_configure(MirWindowAttrib, int)
{
    BOOST_THROW_EXCEPTION(std::logic_error("Attempt to set attrib on screencast is invalid"));
}

MirWaitHandle* mcl::ScreencastStream::set_swap_interval(int)
{
    BOOST_THROW_EXCEPTION(std::logic_error("Attempt to set swap interval on screencast is invalid"));
}

void mcl::ScreencastStream::adopted_by(MirWindow*)
{
}

void mcl::ScreencastStream::unadopted_by(MirWindow*)
{
}

std::chrono::microseconds mcl::ScreencastStream::microseconds_till_vblank() const
{
    return std::chrono::microseconds::zero();
}

void mcl::ScreencastStream::set_size(geom::Size)
{
    BOOST_THROW_EXCEPTION(std::logic_error("Attempt to set size on screencast is invalid"));
}

geom::Size mcl::ScreencastStream::size() const
{
    BOOST_THROW_EXCEPTION(std::logic_error("Attempt to get size on screencast is invalid"));
}

MirWaitHandle* mcl::ScreencastStream::set_scale(float)
{
    BOOST_THROW_EXCEPTION(std::logic_error("Attempt to set scale on screencast is invalid"));
}
