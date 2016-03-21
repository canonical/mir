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

namespace mir
{
namespace client
{
//An internal interface useful in transitioning buffer exchange semantics based on
//the BufferStream response provided by the server
struct ServerBufferSemantics
{
    virtual void deposit(protobuf::Buffer const&, geometry::Size, MirPixelFormat) = 0;
    virtual void set_buffer_cache_size(unsigned int) = 0;
    virtual std::shared_ptr<mir::client::ClientBuffer> current_buffer() = 0;
    virtual uint32_t current_buffer_id() = 0;
    virtual MirWaitHandle* submit(std::function<void()> const&, geometry::Size sz, MirPixelFormat, int stream_id) = 0;
    virtual void lost_connection() = 0;
    virtual void set_size(geom::Size) = 0;
    virtual MirWaitHandle* set_scale(float, mf::BufferStreamId) = 0;
    virtual ~ServerBufferSemantics() = default;
    ServerBufferSemantics() = default;
    ServerBufferSemantics(ServerBufferSemantics const&) = delete;
    ServerBufferSemantics& operator=(ServerBufferSemantics const&) = delete;
};
}
}

namespace
{

struct ExchangeSemantics : mcl::ServerBufferSemantics
{
    ExchangeSemantics(
        mir::protobuf::DisplayServer& server,
        std::shared_ptr<mcl::ClientBufferFactory> const& factory, int max_buffers,
        mp::Buffer const& first_buffer, geom::Size first_size, MirPixelFormat first_pf) :
        wrapped{factory, max_buffers},
        display_server(server)
    {
        wrapped.deposit_package(
            mcl::protobuf_to_native_buffer(first_buffer),
            first_buffer.buffer_id(), first_size, first_pf);
    }

    void deposit(mp::Buffer const& buffer, geom::Size size, MirPixelFormat pf) override
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (on_incoming_buffer)
        {
            wrapped.deposit_package(
                mcl::protobuf_to_native_buffer(buffer),
                buffer.buffer_id(), size, pf);
            if (on_incoming_buffer)
            {
                on_incoming_buffer();
                next_buffer_wait_handle.result_received();
                on_incoming_buffer = std::function<void()>{};
            }
        }
        else
        {
            incoming_buffers.push(buffer);
        }
    }

    void set_buffer_cache_size(unsigned int sz) override
    {
        std::unique_lock<std::mutex> lock(mutex);
        wrapped.set_max_buffers(sz);
    }
    std::shared_ptr<mir::client::ClientBuffer> current_buffer() override
    {
        std::unique_lock<std::mutex> lock(mutex);
        return wrapped.current_buffer();
    }
    uint32_t current_buffer_id() override
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (incoming_buffers.size())
            return incoming_buffers.front().buffer_id();
        return wrapped.current_buffer_id();
    }

    MirWaitHandle* submit(std::function<void()> const& done, geom::Size sz, MirPixelFormat pf, int stream_id) override
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (server_connection_lost)
            BOOST_THROW_EXCEPTION(std::runtime_error("disconnected: no new buffers"));
        //always submit what we have, whether we have a buffer, or will have to wait for an async reply
        auto request = mcl::make_protobuf_object<mp::BufferRequest>();
        request->mutable_id()->set_value(stream_id);
        request->mutable_buffer()->set_buffer_id(wrapped.current_buffer_id());
        lock.unlock();

        display_server.submit_buffer(request.get(), protobuf_void.get(),
            google::protobuf::NewCallback(google::protobuf::DoNothing));

        lock.lock();
        if (server_connection_lost)
            BOOST_THROW_EXCEPTION(std::runtime_error("disconnected: no new buffers"));
        if (incoming_buffers.empty())
        {
            next_buffer_wait_handle.expect_result();
            on_incoming_buffer = done; 
        }
        else
        {
            wrapped.deposit_package(
                mcl::protobuf_to_native_buffer(incoming_buffers.front()),
                incoming_buffers.front().buffer_id(), sz, pf);
            incoming_buffers.pop();
            done();
        }
        return &next_buffer_wait_handle;
    }

    void lost_connection() override
    {
        std::unique_lock<std::mutex> lock(mutex);
        server_connection_lost = true;
        if (on_incoming_buffer)
        {
            on_incoming_buffer();
            on_incoming_buffer = std::function<void()>{};
        }
        if (next_buffer_wait_handle.is_pending())
            next_buffer_wait_handle.result_received();
    }

    void set_size(geom::Size) override
    {
    }

    void on_scale_set(float scale)
    {
        std::unique_lock<decltype(mutex)> lock(mutex);
        scale_ = scale;
        scale_wait_handle.result_received();
    }

    MirWaitHandle* set_scale(float scale, mf::BufferStreamId stream_id) override
    {
        mp::StreamConfiguration configuration;
        configuration.mutable_id()->set_value(stream_id.as_value());
        configuration.set_scale(scale);
        scale_wait_handle.expect_result();

        display_server.configure_buffer_stream(&configuration, protobuf_void.get(),
            google::protobuf::NewCallback(this, &ExchangeSemantics::on_scale_set, scale));
        return &scale_wait_handle;
    }

    std::mutex mutex;
    mcl::ClientBufferDepository wrapped;
    mir::protobuf::DisplayServer& display_server;
    std::function<void()> on_incoming_buffer;
    std::queue<mir::protobuf::Buffer> incoming_buffers;
    std::unique_ptr<mir::protobuf::Void> protobuf_void{std::make_unique<mp::Void>()};
    MirWaitHandle next_buffer_wait_handle;
    bool server_connection_lost {false};
    MirWaitHandle scale_wait_handle;
    float scale_;
};

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

        //note, NewCallback will trigger on exception, deleting this object there
        auto protobuf_void = new mp::Void;
        server.allocate_buffers(&request,  protobuf_void,
            google::protobuf::NewCallback(Requests::ignore_response, protobuf_void));
    }

    void free_buffer(int buffer_id) override
    {
        mp::BufferRelease request;
        request.mutable_id()->set_value(stream_id);
        request.add_buffers()->set_buffer_id(buffer_id);

        //note, NewCallback will trigger on exception, deleting this object there
        auto protobuf_void = new mp::Void;
        server.release_buffers(&request, protobuf_void,
            google::protobuf::NewCallback(Requests::ignore_response, protobuf_void));
    }

    void submit_buffer(mcl::Buffer& buffer) override
    {
        mp::BufferRequest request;
        request.mutable_id()->set_value(stream_id);
        request.mutable_buffer()->set_buffer_id(buffer.rpc_id());

        //note, NewCallback will trigger on exception, deleting this object there
        auto protobuf_void = new mp::Void;
        server.submit_buffer(&request, protobuf_void,
            google::protobuf::NewCallback(Requests::ignore_response, protobuf_void));
    }

    static void ignore_response(mp::Void* void_response)
    {
        delete void_response;
    }

private:
    mclr::DisplayServer& server;
    mp::Void protobuf_void;
    int stream_id;
};

struct NewBufferSemantics : mcl::ServerBufferSemantics
{
    NewBufferSemantics(
        std::shared_ptr<mcl::ClientBufferFactory> const& factory,
        std::shared_ptr<mcl::AsyncBufferFactory> const& mirbuffer_factory,
        std::shared_ptr<mcl::ServerBufferRequests> const& requests,
        std::shared_ptr<mcl::SurfaceMap> const& surface_map,
        geom::Size size, MirPixelFormat format, int usage,
        unsigned int initial_nbuffers) :
        vault(factory, mirbuffer_factory, requests, surface_map, size, format, usage, initial_nbuffers)
    {
    }

    void deposit(mp::Buffer const& buffer, geom::Size, MirPixelFormat) override
    {
        vault.wire_transfer_inbound(buffer);
    }

    void advance_current_buffer(std::unique_lock<std::mutex>& lk)
    {
        lk.unlock();
        auto buffer = vault.withdraw().get();
        lk.lock();
        current = buffer;
    }

    std::shared_ptr<mir::client::ClientBuffer> current_buffer() override
    {
        std::unique_lock<std::mutex> lk(mutex);
        if (!current)
            advance_current_buffer(lk);
        return current->client_buffer();
    }

    uint32_t current_buffer_id() override
    {
        std::unique_lock<std::mutex> lk(mutex);
        if (!current)
            advance_current_buffer(lk);
        return current->rpc_id();
    }

    MirWaitHandle* submit(std::function<void()> const& done, geom::Size, MirPixelFormat, int) override
    {
        std::unique_lock<std::mutex> lk(mutex);
        if (!current)
            advance_current_buffer(lk);
        lk.unlock();

        vault.deposit(current);

        next_buffer_wait_handle.expect_result();
        vault.wire_transfer_outbound(current);
        next_buffer_wait_handle.result_received();

        lk.lock();
        advance_current_buffer(lk);
        done();
        return &next_buffer_wait_handle;
    }

    void set_size(geom::Size size) override
    {
        vault.set_size(size);
    }

    void lost_connection() override
    {
        vault.disconnected();
    }

    void set_buffer_cache_size(unsigned int) override
    {
    }

    MirWaitHandle* set_scale(float scale, mf::BufferStreamId) override
    {
        scale_wait_handle.expect_result();
        scale_wait_handle.result_received();
        vault.set_scale(scale);
        return &scale_wait_handle;
    }

    mcl::BufferVault vault;
    std::mutex mutex;
    std::shared_ptr<mcl::Buffer> current{nullptr};
    MirWaitHandle next_buffer_wait_handle;
    MirWaitHandle scale_wait_handle;
};

}

mcl::BufferStream::BufferStream(
    MirConnection* connection,
    std::shared_ptr<MirWaitHandle> creation_wait_handle,
    mclr::DisplayServer& server,
    std::shared_ptr<mcl::ClientPlatform> const& client_platform,
    mp::BufferStream const& a_protobuf_bs,
    std::shared_ptr<mcl::PerfReport> const& perf_report,
    std::string const& surface_name,
    geom::Size ideal_size,
    size_t nbuffers)
    : connection_(connection),
      display_server(server),
      client_platform(client_platform),
      protobuf_bs{mcl::make_protobuf_object<mir::protobuf::BufferStream>(a_protobuf_bs)},
      scale_(1.0f),
      perf_report(perf_report),
      protobuf_void{mcl::make_protobuf_object<mir::protobuf::Void>()},
      ideal_buffer_size(ideal_size),
      nbuffers(nbuffers),
      creation_wait_handle(creation_wait_handle)
{
    init_swap_interval();
    if (!protobuf_bs->has_id())
    {
        if (!protobuf_bs->has_error())
            protobuf_bs->set_error("Error processing buffer stream create response, no ID (disconnected?)");
    }

    if (protobuf_bs->has_error())
        BOOST_THROW_EXCEPTION(std::runtime_error("Can not create buffer stream: " + std::string(protobuf_bs->error())));

    try
    {
        if (protobuf_bs->has_buffer())
        {
            cached_buffer_size = geom::Size{protobuf_bs->buffer().width(), protobuf_bs->buffer().height()};
            buffer_depository = std::make_unique<ExchangeSemantics>(
                display_server,
                client_platform->create_buffer_factory(),
                mir::frontend::client_buffer_cache_size,
                protobuf_bs->buffer(),
                cached_buffer_size,
                static_cast<MirPixelFormat>(protobuf_bs->pixel_format()));
        }
        else
        {
            cached_buffer_size = ideal_buffer_size;
            buffer_depository = std::make_unique<NewBufferSemantics>(
                client_platform->create_buffer_factory(),
                std::make_shared<mcl::BufferFactory>(),
                std::make_shared<Requests>(display_server, protobuf_bs->id().value()),
                std::make_shared<mcl::ConnectionSurfaceMap>(),
                ideal_buffer_size, static_cast<MirPixelFormat>(protobuf_bs->pixel_format()), 
                protobuf_bs->buffer_usage(), nbuffers);
        }


        egl_native_window_ = client_platform->create_egl_native_window(this);

        // This might seem like something to provide during creation but
        // knowing the swap interval is not a precondition to creation. It's
        // only a precondition to your second and subsequent swaps, so don't
        // bother the creation parameters with this stuff...
        if (fixed_swap_interval)
            force_swap_interval(swap_interval_);
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

void mcl::BufferStream::init_swap_interval()
{
    char const* env = getenv("MIR_CLIENT_FORCE_SWAP_INTERVAL");
    if (env)
    {
        swap_interval_ = atoi(env);
        fixed_swap_interval = true;
    }
    else
    {
        swap_interval_ = 1;
        fixed_swap_interval = false;
    }
}


mcl::BufferStream::BufferStream(
    MirConnection* connection,
    std::shared_ptr<MirWaitHandle> creation_wait_handle,
    mclr::DisplayServer& server,
    std::shared_ptr<mcl::ClientPlatform> const& client_platform,
    mp::BufferStreamParameters const& parameters,
    std::shared_ptr<mcl::PerfReport> const& perf_report,
    size_t nbuffers)
    : connection_(connection),
      display_server(server),
      client_platform(client_platform),
      protobuf_bs{mcl::make_protobuf_object<mir::protobuf::BufferStream>()},
      swap_interval_(1),
      perf_report(perf_report),
      protobuf_void{mcl::make_protobuf_object<mir::protobuf::Void>()},
      ideal_buffer_size(parameters.width(), parameters.height()),
      nbuffers(nbuffers),
      creation_wait_handle(creation_wait_handle)
{
    perf_report->name_surface(std::to_string(reinterpret_cast<long int>(this)).c_str());

    egl_native_window_ = client_platform->create_egl_native_window(this);
    if (protobuf_bs->has_buffer())
    {
        buffer_depository = std::make_unique<ExchangeSemantics>(
            display_server,
            client_platform->create_buffer_factory(),
            mir::frontend::client_buffer_cache_size,
            protobuf_bs->buffer(),
            geom::Size{protobuf_bs->buffer().width(), protobuf_bs->buffer().height()},
            static_cast<MirPixelFormat>(protobuf_bs->pixel_format()));
    }
    else
    {
        buffer_depository = std::make_unique<NewBufferSemantics>(
            client_platform->create_buffer_factory(),
            std::make_shared<mcl::BufferFactory>(),
            std::make_shared<Requests>(display_server, protobuf_bs->id().value()),
            std::make_shared<ConnectionSurfaceMap>(),
            ideal_buffer_size, static_cast<MirPixelFormat>(protobuf_bs->pixel_format()), 0, nbuffers);
    }
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
    if (buffer.has_width() && buffer.has_height())
    {
        cached_buffer_size = geom::Size{buffer.width(), buffer.height()};
    }

    if (buffer.has_error())
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("BufferStream received buffer with error:" + buffer.error()));
    }

    try
    {
        auto pixel_format = static_cast<MirPixelFormat>(protobuf_bs->pixel_format());
        lk.unlock();
        buffer_depository->deposit(buffer, geom::Size{buffer.width(), buffer.height()}, pixel_format);
        perf_report->begin_frame(buffer.buffer_id());
    }
    catch (const std::runtime_error& err)
    {
        mir::log_error("Error processing incoming buffer " + std::string(err.what()));
    }
}

MirWaitHandle* mcl::BufferStream::next_buffer(std::function<void()> const& done)
{
    auto id = buffer_depository->current_buffer_id();
    std::unique_lock<decltype(mutex)> lock(mutex);
    perf_report->end_frame(id);

    secured_region.reset();

    // TODO: We can fix the strange "ID casting" used below in the second phase
    // of buffer stream which generalizes and clarifies the server side logic.
    lock.unlock();
    return buffer_depository->submit(done, cached_buffer_size,
        static_cast<MirPixelFormat>(protobuf_bs->pixel_format()), protobuf_bs->id().value());
}

std::shared_ptr<mcl::ClientBuffer> mcl::BufferStream::get_current_buffer()
{
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
    auto buffer = buffer_depository->current_buffer();
    std::unique_lock<decltype(mutex)> lock(mutex);

    if (!secured_region)
        secured_region = buffer->secure_for_cpu_write();
    return secured_region;
}

/* mcl::EGLNativeSurface interface for EGLNativeWindow integration */
MirSurfaceParameters mcl::BufferStream::get_parameters() const
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

void mcl::BufferStream::request_and_wait_for_next_buffer()
{
    next_buffer([](){})->wait_for_all();
}

void mcl::BufferStream::on_swap_interval_set(int interval)
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    swap_interval_ = interval;
    interval_wait_handle.result_received();
}

void mcl::BufferStream::request_and_wait_for_configure(MirSurfaceAttrib attrib, int interval)
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

uint32_t mcl::BufferStream::get_current_buffer_id()
{
    return buffer_depository->current_buffer_id();
}

int mcl::BufferStream::swap_interval() const
{
    std::unique_lock<decltype(mutex)> lock(mutex);
    return swap_interval_;
}

MirWaitHandle* mcl::BufferStream::set_swap_interval(int interval)
{
    if (fixed_swap_interval)
        return nullptr;
    else
        return force_swap_interval(interval);
}

MirWaitHandle* mcl::BufferStream::force_swap_interval(int interval)
{
    mp::StreamConfiguration configuration;
    configuration.mutable_id()->set_value(protobuf_bs->id().value());
    configuration.set_swapinterval(interval);

    interval_wait_handle.expect_result();
    display_server.configure_buffer_stream(&configuration, protobuf_void.get(),
        google::protobuf::NewCallback(this, &mcl::BufferStream::on_swap_interval_set, interval));

    return &interval_wait_handle;
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

void mcl::BufferStream::set_buffer_cache_size(unsigned int cache_size)
{
    std::unique_lock<std::mutex> lock(mutex);
    buffer_depository->set_buffer_cache_size(cache_size);
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
