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

#include "buffer_stream.h"

#include "mir/frontend/client_constants.h"

#include "mir_toolkit/mir_native_buffer.h"
#include "egl_native_window_factory.h"

#include "perf_report.h"
#include "logging/perf_report.h"

namespace mcl = mir::client;
namespace ml = mir::logging;
namespace mp = mir::protobuf;

namespace
{

void populate_buffer_package(
    MirBufferPackage& buffer_package,
    mir::protobuf::Buffer const& protobuf_buffer)
{
    if (!protobuf_buffer.has_error())
    {
        buffer_package.data_items = protobuf_buffer.data_size();
        for (int i = 0; i != protobuf_buffer.data_size(); ++i)
        {
            buffer_package.data[i] = protobuf_buffer.data(i);
        }

        buffer_package.fd_items = protobuf_buffer.fd_size();

        for (int i = 0; i != protobuf_buffer.fd_size(); ++i)
        {
            buffer_package.fd[i] = protobuf_buffer.fd(i);
        }

        buffer_package.stride = protobuf_buffer.stride();
        buffer_package.flags = protobuf_buffer.flags();
        buffer_package.width = protobuf_buffer.width();
        buffer_package.height = protobuf_buffer.height();
    }
    else
    {
        buffer_package.data_items = 0;
        buffer_package.fd_items = 0;
        buffer_package.stride = 0;
        buffer_package.flags = 0;
        buffer_package.width = 0;
        buffer_package.height = 0;
    }
}

std::shared_ptr<mcl::PerfReport>
make_perf_report(std::shared_ptr<ml::Logger> const& logger)
{
    // TODO: It seems strange that this directly uses getenv
    const char* report_target = getenv("MIR_CLIENT_PERF_REPORT");
    if (report_target && !strcmp(report_target, "log"))
    {
        return std::make_shared<mir::client::logging::PerfReport>(logger);
    }
    else
    {
        return std::make_shared<mir::client::NullPerfReport>();
    }
}
}

// TODO: Examine mutexing requirements
// TODO: It seems like a bit of a wart that we have to pass the Logger specifically here...perhaps
// due to the lack of an easily mockable client configuration interface (passing around
// connection can complicate unit tests ala MirSurface and test_client_mir_surface.cpp)
mcl::BufferStream::BufferStream(mp::DisplayServer& server,
    mcl::BufferStreamMode mode,
    std::shared_ptr<mcl::ClientBufferFactory> const& buffer_factory,
    std::shared_ptr<mcl::EGLNativeWindowFactory> const& native_window_factory,
    mp::BufferStream const& protobuf_bs,
    std::shared_ptr<ml::Logger> const& logger)
    : display_server(server),
      mode(mode),
      native_window_factory(native_window_factory),
      protobuf_bs(protobuf_bs),
      buffer_depository{buffer_factory, mir::frontend::client_buffer_cache_size},
      perf_report(make_perf_report(logger))
      
{
    // TODO: When to verify error on protobuf screencast/buffer stream?
    egl_native_window_ = native_window_factory->create_egl_native_window(this);
    process_buffer(protobuf_bs.buffer());

    // TODO: Probably something better than this available...
    perf_report->name_surface(std::to_string(reinterpret_cast<long unsigned int>(this)).c_str());
}

mcl::BufferStream::~BufferStream()
{
}

void mcl::BufferStream::process_buffer(mp::Buffer const& buffer)
{
    auto buffer_package = std::make_shared<MirBufferPackage>();
    populate_buffer_package(*buffer_package, buffer);

    try
    {
        auto pixel_format = static_cast<MirPixelFormat>(protobuf_bs.pixel_format());
        buffer_depository.deposit_package(buffer_package,
            buffer.buffer_id(),
            {buffer_package->width, buffer_package->height}, pixel_format);
        perf_report->begin_frame(buffer.buffer_id());
    }
    catch (const std::runtime_error& err)
    {
        // TODO: Report the error
    }
}

MirWaitHandle* mcl::BufferStream::next_buffer(std::function<void()> const& done)
{
    perf_report->end_frame(get_current_buffer_id());

    mir::protobuf::BufferStreamId buffer_stream_id;
    buffer_stream_id.set_value(protobuf_bs.id().value());

    next_buffer_wait_handle.expect_result();
    
    // TODO: Fix signature
    std::function<void()> copy_done = done;

    if (mode == mcl::BufferStreamMode::Producer)
    {
        mp::BufferRequest request;
// TODO: Fix after porting ID's
        request.mutable_id()->set_value(protobuf_bs.id().value());
        request.mutable_buffer()->set_buffer_id(protobuf_bs.buffer().buffer_id());
        display_server.exchange_buffer(
            nullptr,
            &request,
            protobuf_bs.mutable_buffer(),
            google::protobuf::NewCallback(
            this, &mcl::BufferStream::next_buffer_received,
            copy_done));
    }
    else
    {
        // TODO: Fix after porting screencast
        mp::ScreencastId screencast_id;
        screencast_id.set_value(protobuf_bs.id().value());

        display_server.screencast_buffer(
            nullptr,
            &screencast_id,
            protobuf_bs.mutable_buffer(),
            google::protobuf::NewCallback(
            this, &mcl::BufferStream::next_buffer_received,
            copy_done));
    }


    return &next_buffer_wait_handle;
}

std::shared_ptr<mcl::ClientBuffer> mcl::BufferStream::get_current_buffer()
{
    return buffer_depository.current_buffer();
}

EGLNativeWindowType mcl::BufferStream::egl_native_window()
{
    return *egl_native_window_;
}

std::shared_ptr<mcl::MemoryRegion> mcl::BufferStream::secure_for_cpu_write()
{
// TODO: Who owns this region? e.g. do we have to track a secured region ala mir surface or will
// surface screencast and buffer stream do it...
    return get_current_buffer()->secure_for_cpu_write();
}

void mcl::BufferStream::next_buffer_received(
    std::function<void()> done)                                             
{
    process_buffer(protobuf_bs.buffer());

    done();

    next_buffer_wait_handle.result_received();
}

/* mcl::ClientSurface interface for EGLNativeWindow integration */
MirSurfaceParameters mcl::BufferStream::get_parameters() const
{
    return MirSurfaceParameters{
        "",
        protobuf_bs.buffer().width(),
        protobuf_bs.buffer().height(),
        static_cast<MirPixelFormat>(protobuf_bs.pixel_format()),
        static_cast<MirBufferUsage>(protobuf_bs.buffer_usage()),
        mir_display_output_id_invalid};
}

void mcl::BufferStream::request_and_wait_for_next_buffer()
{
    next_buffer([](){})->wait_for_all();
}

void mcl::BufferStream::request_and_wait_for_configure(MirSurfaceAttrib, int)
{
}

uint32_t mcl::BufferStream::get_current_buffer_id()
{
    return buffer_depository.current_buffer_id();
}
