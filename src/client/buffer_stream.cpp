/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#define MIR_LOG_COMPONENT "MirBufferStream"

#include "buffer_stream.h"
#include "buffer.h"
#include "buffer_factory.h"
#include "make_protobuf_object.h"
#include "mir_connection.h"
#include "perf_report.h"
#include "logging/perf_report.h"
#include "rpc/mir_display_server.h"
#include "mir_protobuf.pb.h"
#include "buffer_vault.h"
#include "protobuf_to_native_buffer.h"
#include "buffer.h"
#include "connection_surface_map.h"

#include "mir/log.h"
#include "mir/client_platform.h"
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

namespace gp = google::protobuf;

namespace
{
class Requests : public mcl::ServerBufferRequests
{
public:
    Requests(mclr::DisplayServer& server, int stream_id) :
        server(server),
        stream_id(stream_id)
    {
    }

    void allocate_buffer(geom::Size size, MirPixelFormat format, int usage) override
    {
        mp::BufferAllocation request;
        request.mutable_id()->set_value(stream_id);
        auto buf_params = request.add_buffer_requests();
        buf_params->set_width(size.width.as_int());
        buf_params->set_height(size.height.as_int());
        buf_params->set_pixel_format(format);
        buf_params->set_buffer_usage(usage);

        auto protobuf_void = std::make_shared<mp::Void>();
        server.allocate_buffers(&request, protobuf_void.get(),
            google::protobuf::NewCallback(Requests::ignore_response, protobuf_void));
    }

    void free_buffer(int buffer_id) override
    {
        mp::BufferRelease request;
        request.mutable_id()->set_value(stream_id);
        request.add_buffers()->set_buffer_id(buffer_id);

        auto protobuf_void = std::make_shared<mp::Void>();
        server.release_buffers(&request, protobuf_void.get(),
            google::protobuf::NewCallback(Requests::ignore_response, protobuf_void));
    }

    void submit_buffer(mcl::MirBuffer& buffer) override
    {
        mp::BufferRequest request;
        request.mutable_id()->set_value(stream_id);
        request.mutable_buffer()->set_buffer_id(buffer.rpc_id());

        auto protobuf_void = std::make_shared<mp::Void>();
        server.submit_buffer(&request, protobuf_void.get(),
            google::protobuf::NewCallback(Requests::ignore_response, protobuf_void));
    }

    static void ignore_response(std::shared_ptr<mp::Void>)
    {
    }

private:
    mclr::DisplayServer& server;
    int stream_id;
};

mir::optional_value<int> parse_env_for_swap_interval()
{
    if (auto env = getenv("MIR_CLIENT_FORCE_SWAP_INTERVAL"))
    {
        mir::log_info("overriding swapinterval setting because of presence of MIR_CLIENT_FORCE_SWAP_INTERVAL");
        return {atoi(env)};
    }
    else
    {
        return {};
    }
}
}

namespace mir
{
namespace client
{
//Should be merged with mcl::BufferVault
struct BufferDepository
{
    BufferDepository(
        std::shared_ptr<mcl::ClientBufferFactory> const& factory,
        std::shared_ptr<mcl::AsyncBufferFactory> const& mirbuffer_factory,
        std::shared_ptr<mcl::ServerBufferRequests> const& requests,
        std::weak_ptr<mcl::SurfaceMap> const& surface_map,
        geom::Size size, MirPixelFormat format, int usage,
        unsigned int initial_nbuffers) :
        vault(factory, mirbuffer_factory, requests, surface_map, size, format, usage, initial_nbuffers),
        current(nullptr),
        size_(size)
    {
        future = vault.withdraw();
    }

    void deposit(mp::Buffer const&, mir::optional_value<geom::Size>, MirPixelFormat)
    {
    }

    void advance_current_buffer(std::unique_lock<std::mutex>& lk)
    {
        lk.unlock();
        auto c = future.get();
        lk.lock();
        current = c;
    }

    std::shared_ptr<mir::client::ClientBuffer> current_buffer()
    {
        std::unique_lock<std::mutex> lk(mutex);
        if (!current)
            advance_current_buffer(lk);
        return current->client_buffer();
    }

    uint32_t current_buffer_id()
    {
        std::unique_lock<std::mutex> lk(mutex);
        if (!current)
            advance_current_buffer(lk);
        return current->rpc_id();
    }

    MirWaitHandle* submit(std::function<void()> const& done, MirPixelFormat, int)
    {
        std::unique_lock<std::mutex> lk(mutex);
        if (!current)
            advance_current_buffer(lk);
        auto c = current;
        current = nullptr;
        lk.unlock();

        vault.deposit(c);
        auto wh = vault.wire_transfer_outbound(c, done);
        auto f = vault.withdraw();
        lk.lock();
        future = std::move(f);
        return wh;
    }

    void set_size(geom::Size size)
    {
        {
            std::unique_lock<std::mutex> lk(mutex);
            size_ = size;
        }
        vault.set_size(size);
    }

    geom::Size size() const
    {
        std::unique_lock<std::mutex> lk(mutex);
        return size_;
    }

    void lost_connection()
    {
        vault.disconnected();
    }

    MirWaitHandle* set_scale(float scale, mf::BufferStreamId)
    {
        scale_wait_handle.expect_result();
        scale_wait_handle.result_received();
        vault.set_scale(scale);
        return &scale_wait_handle;
    }

    void set_interval(int interval)
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        interval = std::max(0, std::min(1, interval));
        vault.set_interval(interval);
    }

    // Future must be before vault, to ensure vault's destruction marks future
    // as ready.
    mir::client::NoTLSFuture<std::shared_ptr<mcl::MirBuffer>> future;

    mcl::BufferVault vault;
    std::mutex mutable mutex;
    std::shared_ptr<mcl::MirBuffer> current{nullptr};
    MirWaitHandle scale_wait_handle;
    geom::Size size_;
};
}
}

mcl::BufferStream::BufferStream(
    MirConnection* connection,
    MirRenderSurface* render_surface,
    std::shared_ptr<MirWaitHandle> creation_wait_handle,
    mclr::DisplayServer& server,
    std::shared_ptr<mcl::ClientPlatform> const& client_platform,
    std::weak_ptr<mcl::SurfaceMap> const& map,
    std::shared_ptr<mcl::AsyncBufferFactory> const& factory,
    mp::BufferStream const& a_protobuf_bs,
    std::shared_ptr<mcl::PerfReport> const& perf_report,
    std::string const& surface_name,
    geom::Size ideal_size,
    size_t nbuffers)
    : connection_(connection),
      client_platform(client_platform),
      protobuf_bs{mcl::make_protobuf_object<mir::protobuf::BufferStream>(a_protobuf_bs)},
      user_swap_interval(parse_env_for_swap_interval()),
      interval_config{server, frontend::BufferStreamId{a_protobuf_bs.id().value()}},
      scale_(1.0f),
      perf_report(perf_report),
      protobuf_void{mcl::make_protobuf_object<mir::protobuf::Void>()},
      ideal_buffer_size(ideal_size),
      nbuffers(nbuffers),
      creation_wait_handle(creation_wait_handle),
      map(map),
      factory(factory),
      render_surface_(render_surface)
{
    if (!protobuf_bs->has_id())
    {
        if (!protobuf_bs->has_error())
            protobuf_bs->set_error("Error processing buffer stream create response, no ID (disconnected?)");
    }

    if (protobuf_bs->has_error())
    {
        if (protobuf_bs->has_buffer())
        {
            for (int i = 0; i < protobuf_bs->buffer().fd_size(); i++)
                ::close(protobuf_bs->buffer().fd(i));
        }

        BOOST_THROW_EXCEPTION(std::runtime_error("Can not create buffer stream: " + std::string(protobuf_bs->error())));
    }

    try
    {
        buffer_depository = std::make_unique<BufferDepository>(
            client_platform->create_buffer_factory(), factory,
            std::make_shared<Requests>(server, protobuf_bs->id().value()), map,
            ideal_buffer_size, static_cast<MirPixelFormat>(protobuf_bs->pixel_format()), 
            protobuf_bs->buffer_usage(), nbuffers);

        egl_native_window_ = client_platform->create_egl_native_window(this);

        // This might seem like something to provide during creation but
        // knowing the swap interval is not a precondition to creation. It's
        // only a precondition to your second and subsequent swaps, so don't
        // bother the creation parameters with this stuff...
        if (user_swap_interval.is_set())
            set_swap_interval(user_swap_interval.value());
    }
    catch (std::exception const& error)
    {
        protobuf_bs->set_error(std::string{"Error processing buffer stream creating response:"} +
                               boost::diagnostic_information(error));

        if (!buffer_depository)
        {
            for (int i = 0; i < protobuf_bs->buffer().fd_size(); i++)
                ::close(protobuf_bs->buffer().fd(i));
        }
    }

    if (!valid())
        BOOST_THROW_EXCEPTION(std::runtime_error("Can not create buffer stream: " + std::string(protobuf_bs->error())));
    perf_report->name_surface(surface_name.c_str());
}

mcl::BufferStream::~BufferStream()
{
}

void mcl::BufferStream::process_buffer(mp::Buffer const& buffer)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    process_buffer(buffer, lock);
}

void mcl::BufferStream::process_buffer(protobuf::Buffer const& buffer, std::unique_lock<std::mutex>& lk)
{
    mir::optional_value<geom::Size> size;
    if (buffer.has_width() && buffer.has_height())
        size = geom::Size(buffer.width(), buffer.height());

    if (buffer.has_error())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("BufferStream received buffer with error:" + buffer.error()));
    }

    try
    {
        auto pixel_format = static_cast<MirPixelFormat>(protobuf_bs->pixel_format());
        lk.unlock();
        buffer_depository->deposit(buffer, size, pixel_format);
    }
    catch (const std::runtime_error& err)
    {
        mir::log_error("Error processing incoming buffer " + std::string(err.what()));
    }
}

MirWaitHandle* mcl::BufferStream::swap_buffers(std::function<void()> const& done)
{
    auto id = buffer_depository->current_buffer_id();
    std::unique_lock<decltype(mutex)> lock(mutex);
    perf_report->end_frame(id);

    secured_region.reset();

    // TODO: We can fix the strange "ID casting" used below in the second phase
    // of buffer stream which generalizes and clarifies the server side logic.
    lock.unlock();
    return buffer_depository->submit(done,
        static_cast<MirPixelFormat>(protobuf_bs->pixel_format()), protobuf_bs->id().value());
}

std::shared_ptr<mcl::ClientBuffer> mcl::BufferStream::get_current_buffer()
{
    int current_id = buffer_depository->current_buffer_id();
    perf_report->begin_frame(current_id);
    return buffer_depository->current_buffer();
}

EGLNativeWindowType mcl::BufferStream::egl_native_window()
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    return static_cast<EGLNativeWindowType>(egl_native_window_.get());
}

void mcl::BufferStream::release_cpu_region()
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    secured_region.reset();
}

std::shared_ptr<mcl::MemoryRegion> mcl::BufferStream::secure_for_cpu_write()
{
    auto buffer = get_current_buffer();
    std::unique_lock<decltype(mutex)> lock(mutex);

    if (!secured_region)
        secured_region = buffer->secure_for_cpu_write();
    return secured_region;
}

/* mcl::EGLNativeSurface interface for EGLNativeWindow integration */
MirSurfaceParameters mcl::BufferStream::get_parameters() const
{
    auto size = buffer_depository->size();
    std::unique_lock<decltype(mutex)> lock(mutex);
    return MirSurfaceParameters{
        "",
        size.width.as_int(),
        size.height.as_int(),
        static_cast<MirPixelFormat>(protobuf_bs->pixel_format()),
        static_cast<MirBufferUsage>(protobuf_bs->buffer_usage()),
        mir_display_output_id_invalid};
}

void mcl::BufferStream::swap_buffers_sync()
{
    swap_buffers([](){})->wait_for_all();
}

void mcl::BufferStream::request_and_wait_for_configure(MirSurfaceAttrib attrib, int interval)
{
    if (attrib != mir_surface_attrib_swapinterval)
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Attempt to configure surface attribute " + std::to_string(attrib) +
        " on BufferStream but only mir_surface_attrib_swapinterval is supported")); 
    }
    mir_wait_for(set_swap_interval(interval));
}

uint32_t mcl::BufferStream::get_current_buffer_id()
{
    return buffer_depository->current_buffer_id();
}

int mcl::BufferStream::swap_interval() const
{
    return interval_config.swap_interval();
}

MirWaitHandle* mcl::BufferStream::set_swap_interval(int interval)
{
    if (user_swap_interval.is_set())
        interval = user_swap_interval.value();

    buffer_depository->set_interval(interval);
    return interval_config.set_swap_interval(interval);
}

void mcl::BufferStream::adopted_by(std::shared_ptr<MirSurface> const&)
{
    // TODO
}

void mcl::BufferStream::unadopted_by(std::shared_ptr<MirSurface> const&)
{
    // TODO
}

MirNativeBuffer* mcl::BufferStream::get_current_buffer_package()
{
    auto buffer = get_current_buffer();
    auto handle = buffer->native_buffer_handle();
    return client_platform->convert_native_buffer(handle.get());
}

MirPlatformType mcl::BufferStream::platform_type()
{
    return client_platform->platform_type();
}

mf::BufferStreamId mcl::BufferStream::rpc_id() const
{
    std::unique_lock<decltype(mutex)> lock(mutex);

    return mf::BufferStreamId(protobuf_bs->id().value());
}

bool mcl::BufferStream::valid() const
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    return protobuf_bs->has_id() && !protobuf_bs->has_error();
}

void mcl::BufferStream::set_buffer_cache_size(unsigned int)
{
}

void mcl::BufferStream::buffer_available(mir::protobuf::Buffer const& buffer)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    process_buffer(buffer, lock);
}

void mcl::BufferStream::buffer_unavailable()
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    buffer_depository->lost_connection(); 
}

void mcl::BufferStream::set_size(geom::Size sz)
{
    buffer_depository->set_size(sz);
}

geom::Size mcl::BufferStream::size() const
{
    return buffer_depository->size();
}

MirWaitHandle* mcl::BufferStream::set_scale(float scale)
{
    return buffer_depository->set_scale(scale, mf::BufferStreamId(protobuf_bs->id().value()));
}

char const * mcl::BufferStream::get_error_message() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    if (protobuf_bs->has_error())
    {
        return protobuf_bs->error().c_str();
    }

    return error_message.c_str();
}

MirConnection* mcl::BufferStream::connection() const
{
    return connection_;
}

MirRenderSurface* mcl::BufferStream::render_surface() const
{
    return render_surface_;
}
