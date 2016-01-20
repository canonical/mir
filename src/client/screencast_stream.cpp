/*
 * Copyright © 2016 Canonical Ltd.
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

#include "screencast_stream.h"
#include "make_protobuf_object.h"
#include "mir_connection.h"
#include "rpc/mir_display_server.h"
#include "mir_protobuf.pb.h"
#include "buffer_vault.h"

#include "mir/log.h"
#include "mir/client_platform.h"
#include "mir/client_buffer_factory.h"
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
    mp::BufferStream const& a_protobuf_bs,
    geom::Size ideal_size) :
      connection_(connection),
      display_server(server),
      client_platform(client_platform),
      factory(client_platform->create_buffer_factory()),
      protobuf_bs{mcl::make_protobuf_object<mir::protobuf::BufferStream>(a_protobuf_bs)},
      swap_interval_(1),
      protobuf_void{mcl::make_protobuf_object<mir::protobuf::Void>()},
      ideal_buffer_size(ideal_size)
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

    cached_buffer_size = geom::Size{protobuf_bs->buffer().width(), protobuf_bs->buffer().height()};

    
    try
    {
        process_buffer(protobuf_bs->buffer());
        egl_native_window_ = client_platform->create_egl_native_window(this);
    }
    catch (std::exception const& error)
    {
        protobuf_bs->set_error(std::string{"Error processing buffer stream creating response:"} +
                               boost::diagnostic_information(error));
        if (!current_buffer)
        {
            for (int i = 0; i < protobuf_bs->buffer().fd_size(); i++)
                ::close(protobuf_bs->buffer().fd(i));
        }
    }

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
    if (buffer.has_width() && buffer.has_height())
        cached_buffer_size = geom::Size{buffer.width(), buffer.height()};

    if (buffer.has_error())
        BOOST_THROW_EXCEPTION(std::runtime_error("BufferStream received buffer with error:" + buffer.error()));

    try
    {
        auto package = std::make_shared<MirBufferPackage>();
        package->data_items = buffer.data_size();
        package->fd_items = buffer.fd_size();
        for (int i = 0; i != buffer.data_size(); ++i)
            package->data[i] = buffer.data(i);
        for (int i = 0; i != buffer.fd_size(); ++i)
            package->fd[i] = buffer.fd(i);
        package->stride = buffer.stride();
        package->flags = buffer.flags();
        package->width = buffer.width();
        package->height = buffer.height();
        current_buffer = factory->create_buffer(
            package,
            cached_buffer_size, mir_pixel_format_abgr_8888);
//            static_cast<MirPixelFormat>(buffer.pixel_format()));
    }
    catch (const std::runtime_error& err)
    {
        mir::log_error("Error processing incoming buffer " + std::string(err.what()));
    }
}

MirWaitHandle* mcl::ScreencastStream::next_buffer(std::function<void()> const& done)
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
    std::unique_lock<decltype(mutex)> lock(mutex);
    return current_buffer;
}

EGLNativeWindowType mcl::ScreencastStream::egl_native_window()
{
    //probabbly dont need to lock
    std::unique_lock<decltype(mutex)> lock(mutex);
    return *egl_native_window_;
}

void mcl::ScreencastStream::release_cpu_region()
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    secured_region.reset();
}

std::shared_ptr<mcl::MemoryRegion> mcl::ScreencastStream::secure_for_cpu_write()
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    if (!secured_region)
        secured_region = current_buffer->secure_for_cpu_write();
    return secured_region;
}

void mcl::ScreencastStream::screencast_buffer_received(std::function<void()> done)
{
    process_buffer(protobuf_bs->buffer());
    done();
    screencast_wait_handle.result_received();
}

/* mcl::EGLNativeSurface interface for EGLNativeWindow integration */
MirSurfaceParameters mcl::ScreencastStream::get_parameters() const
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    return MirSurfaceParameters{
        "",
        cached_buffer_size.width.as_int(),
        cached_buffer_size.height.as_int(),
        static_cast<MirPixelFormat>(protobuf_bs->pixel_format()),
        static_cast<MirBufferUsage>(protobuf_bs->buffer_usage()),
        mir_display_output_id_invalid};
}

void mcl::ScreencastStream::request_and_wait_for_next_buffer()
{
    next_buffer([](){})->wait_for_all();
}

void mcl::ScreencastStream::request_and_wait_for_configure(MirSurfaceAttrib attrib, int interval)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    if (attrib != mir_surface_attrib_swapinterval)
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Attempt to configure surface attribute " + std::to_string(attrib) +
        " on BufferStream but only mir_surface_attrib_swapinterval is supported")); 
    }

    auto i = interval;
    lock.unlock();
    set_swap_interval(i);
    interval_wait_handle.wait_for_all();
}

uint32_t mcl::ScreencastStream::get_current_buffer_id()
{
//    std::unique_lock<decltype(mutex)> lock(mutex);
//    return buffer_depository->current_buffer_id();
    return 0;
}

int mcl::ScreencastStream::swap_interval() const
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    return swap_interval_;
}

MirWaitHandle* mcl::ScreencastStream::set_swap_interval(int)
{
    BOOST_THROW_EXCEPTION(std::logic_error("Attempt to set swap interval on screencast is invalid"));
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
    std::unique_lock<decltype(mutex)> lock(mutex);
    return mf::BufferStreamId(protobuf_bs->id().value());
}

bool mcl::ScreencastStream::valid() const
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    return protobuf_bs->has_id() && !protobuf_bs->has_error();
}

void mcl::ScreencastStream::set_buffer_cache_size(unsigned int)
{
//    std::unique_lock<std::mutex> lock(mutex);
//    buffer_depository->set_buffer_cache_size(cache_size);
}

void mcl::ScreencastStream::buffer_available(mir::protobuf::Buffer const& buffer)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    process_buffer(buffer, lock);
}

void mcl::ScreencastStream::buffer_unavailable()
{
//    std::unique_lock<decltype(mutex)> lock(mutex);
//    buffer_depository->lost_connection(); 
}

void mcl::ScreencastStream::set_size(geom::Size)
{
    BOOST_THROW_EXCEPTION(std::logic_error("Attempt to set size on screencast is invalid"));
}

MirWaitHandle* mcl::ScreencastStream::set_scale(float)
{
    BOOST_THROW_EXCEPTION(std::logic_error("Attempt to set scale on screencast is invalid"));
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
