/*
 * Copyright © 2015 Canonical Ltd.
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
#include "mir/client/client_platform.h"
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
    Requests(
        mclr::DisplayServer& server,
        int stream_id,
        std::shared_ptr<mcl::ClientPlatform> const& platform) :
        server(server),
        stream_id(stream_id),
        platform(platform)
    {
    }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    void allocate_buffer(geom::Size size, MirPixelFormat format, int usage) override
    {
        mp::BufferAllocation request;
        request.mutable_id()->set_value(stream_id);
        auto buf_params = request.add_buffer_requests();
        buf_params->set_width(size.width.as_int());
        buf_params->set_height(size.height.as_int());

        if (usage == mir_buffer_usage_hardware)
        {
            buf_params->set_native_format(platform->native_format_for(format));
            buf_params->set_flags(platform->native_flags_for(static_cast<MirBufferUsage>(usage), size));
        }
        else
        {
            buf_params->set_pixel_format(format);
            buf_params->set_buffer_usage(usage);
        }
        auto protobuf_void = std::make_shared<mp::Void>();
        server.allocate_buffers(&request, protobuf_void.get(),
            google::protobuf::NewCallback(Requests::ignore_response, protobuf_void));
    }
#pragma GCC diagnostic pop
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
    std::shared_ptr<mcl::ClientPlatform> const platform;
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

    MirWaitHandle* submit(std::function<void()> const& done)
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
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
      using_client_side_vsync(false),
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
    /*
     * Since we try to use client-side vsync where possible, a separate
     * copy of the current swap interval is required to represent that
     * the stream might have a non-zero interval while we want the server
     * to use a zero interval. This is not stored inside interval_config
     * because with some luck interval_config will eventually go away
     * leaving just int current_swap_interval.
     */
    current_swap_interval = interval_config.swap_interval();

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
            std::make_shared<Requests>(server, protobuf_bs->id().value(), client_platform),
            map,
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
#pragma GCC diagnostic pop

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

    if (!using_client_side_vsync &&
        current_swap_interval != interval_config.swap_interval())
        set_server_swap_interval(current_swap_interval);

    secured_region.reset();

    lock.unlock();
    return buffer_depository->submit(done);
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

std::shared_ptr<mcl::MemoryRegion> mcl::BufferStream::secure_for_cpu_write()
{
    auto buffer = get_current_buffer();
    std::unique_lock<decltype(mutex)> lock(mutex);

    if (!secured_region)
        secured_region = buffer->secure_for_cpu_write();
    return secured_region;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
/* mcl::EGLNativeSurface interface for EGLNativeWindow integration */
MirWindowParameters mcl::BufferStream::get_parameters() const
{
    auto size = buffer_depository->size();
    std::unique_lock<decltype(mutex)> lock(mutex);
    return MirWindowParameters{
        "",
        size.width.as_int(),
        size.height.as_int(),
        static_cast<MirPixelFormat>(protobuf_bs->pixel_format()),
        static_cast<MirBufferUsage>(protobuf_bs->buffer_usage()),
        mir_display_output_id_invalid};
}
#pragma GCC diagnostic pop

std::chrono::microseconds mcl::BufferStream::microseconds_till_vblank() const
{
    std::chrono::microseconds ret(0);
    mir::time::PosixTimestamp last;
    std::shared_ptr<FrameClock> clock;

    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        last = last_vsync;
        clock = frame_clock;
    }

    if (clock)
    {
        // We are unlocked because in future this call might ping the server:
        mir::time::PosixTimestamp const target = clock->next_frame_after(last);
        auto const now = mir::time::PosixTimestamp::now(target.clock_id);
        if (target > now)
        {
            ret = std::chrono::duration_cast<std::chrono::microseconds>(
                  target - now);
        }

        std::lock_guard<decltype(mutex)> lock(mutex);
        next_vsync = target;
    }

    return ret;
}

void mcl::BufferStream::wait_for_vsync()
{
    mir::time::PosixTimestamp last, target;
    std::shared_ptr<FrameClock> clock;

    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        if (!frame_clock)
            return;
        last = last_vsync;
        clock = frame_clock;
    }

    target = clock->next_frame_after(last);
    sleep_until(target);

    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        if (target > last_vsync)
            last_vsync = target;
    }
}

MirWaitHandle* mcl::BufferStream::set_server_swap_interval(int i)
{
    /*
     * TODO: Remove these functions in future, after
     *       mir_buffer_stream_swap_buffers has been converted to use
     *       client-side vsync, and the server has been told to always use
     *       dropping for BufferStream. Then there will be no need to ever
     *       change the server swap interval from zero.
     */
    buffer_depository->set_interval(i);
    return interval_config.set_swap_interval(i);
}

void mcl::BufferStream::swap_buffers_sync()
{
    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        using_client_side_vsync = true;
    }

    /*
     * Until recently it seemed like server-side vsync could co-exist with
     * client-side vsync. However my original theory that it was risky has
     * proven to be true. If we leave server-side vsync throttling to interval
     * one at the same time as using client-side, there's a risk the server
     * will not get scheduled sufficiently to drain the queue as fast as
     * we fill it, creating lag. The acceptance test `ClientLatency' has now
     * proven this is a real problem so we must be sure to put the server in
     * interval 0 when using client-side vsync. This guarantees that random
     * scheduling imperfections won't create queuing lag.
     */
    if (interval_config.swap_interval() != 0)
        set_server_swap_interval(0);

    swap_buffers([](){})->wait_for_all();

    {
        std::lock_guard<decltype(mutex)> lock(mutex);
        using_client_side_vsync = false;
    }

    int interval = swap_interval();
    for (int i = 0; i < interval; ++i)
        wait_for_vsync();

    if (!interval)  // wait_for_vsync wasn't called to update last_vsync
        last_vsync = next_vsync;
}

void mcl::BufferStream::request_and_wait_for_configure(MirWindowAttrib attrib, int interval)
{
    if (attrib != mir_window_attrib_swapinterval)
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Attempt to configure surface attribute " + std::to_string(attrib) +
        " on BufferStream but only mir_window_attrib_swapinterval is supported")); 
    }

    if (auto wh = set_swap_interval(interval))
        wh->wait_for_all();
}

uint32_t mcl::BufferStream::get_current_buffer_id()
{
    return buffer_depository->current_buffer_id();
}

int mcl::BufferStream::swap_interval() const
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    return current_swap_interval;
}

MirWaitHandle* mcl::BufferStream::set_swap_interval(int interval)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    current_swap_interval = user_swap_interval.is_set() ?
        user_swap_interval.value() : interval;

    return set_server_swap_interval(current_swap_interval);
}

void mcl::BufferStream::adopted_by(MirWindow* win)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    /*
     * Yes, we're storing raw pointers here. That's safe so long as MirWindow
     * remembers to always call unadopted_by prior to its destruction.
     *   The alternative of MirWindow passing in a shared_ptr to itself is
     * actually uglier than this...
     */
    users.insert(win);
    if (!frame_clock)
        frame_clock = win->get_frame_clock();
}

void mcl::BufferStream::unadopted_by(MirWindow* win)
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    users.erase(win);
    if (frame_clock == win->get_frame_clock())
    {
        frame_clock = users.empty() ? nullptr
                                    : (*users.begin())->get_frame_clock();
    }
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
MirRenderSurface* mcl::BufferStream::render_surface() const
{
    return render_surface_;
}
#pragma GCC diagnostic pop
