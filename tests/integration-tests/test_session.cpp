/*
 * Copyright © 2013-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/default_server_configuration.h"
#include "src/server/input/null_input_manager.h"
#include "mir/compositor/compositor.h"
#include "src/server/scene/application_session.h"
#include "src/server/scene/pixel_buffer.h"
#include "mir/scene/surface.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/scene/null_session_listener.h"
#include "mir/scene/null_surface_observer.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/renderer/renderer.h"
#include "mir/renderer/renderer_factory.h"
#include "mir/frontend/connector.h"

#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/doubles/stub_gl_buffer.h"
#include "mir/test/doubles/stub_buffer_stream_factory.h"
#include "mir/test/doubles/stub_display.h"
#include "mir/test/doubles/null_event_sink.h"
#include "mir/test/doubles/stub_renderer.h"
#include "mir/test/doubles/stub_surface_factory.h"
#include "mir/test/doubles/null_pixel_buffer.h"
#include "mir/test/doubles/stub_display_configuration.h"
#include "mir_test_framework/stubbed_server_configuration.h"

#include <gtest/gtest.h>
#include <condition_variable>
#include <atomic>

namespace mc = mir::compositor;
namespace mtd = mir::test::doubles;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mi = mir::input;
namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{

struct TestServerConfiguration : public mir_test_framework::StubbedServerConfiguration
{
    std::shared_ptr<mg::GraphicBufferAllocator> the_buffer_allocator() override
    {
        return buffer_allocator(
            []
            {
                return std::make_shared<mtd::StubBufferAllocator>();
            });
    }
};

struct StubGLBufferStream : public mtd::StubBufferStream
{
public:
    StubGLBufferStream()
    {
        stub_compositor_buffer = std::make_shared<mtd::StubGLBuffer>();
    }
};

struct StubGLBufferStreamFactory : public mtd::StubBufferStreamFactory
{
    std::shared_ptr<mc::BufferStream> create_buffer_stream(
        mg::BufferProperties const&) override
    {
        return std::make_shared<StubGLBufferStream>();
    }
};

} // anonymouse namespace

TEST(ApplicationSession, stress_test_take_snapshot)
{
    TestServerConfiguration conf;
    // Otherwise the input registrar won't function
    auto dispatcher = conf.the_input_dispatcher();

    ms::ApplicationSession session{
        conf.the_surface_stack(),
        conf.the_surface_factory(),
        std::make_shared<StubGLBufferStreamFactory>(),
        __LINE__,
        "stress",
        conf.the_snapshot_strategy(),
        std::make_shared<ms::NullSessionListener>(),
        std::make_shared<mtd::NullEventSink>(),
        conf.the_buffer_allocator()
    };

    mg::BufferProperties properties(geom::Size{1,1}, mir_pixel_format_abgr_8888, mg::BufferUsage::software);
    auto stream = std::dynamic_pointer_cast<mc::BufferStream>(session.create_buffer_stream(properties));
    session.create_surface(
        ms::a_surface().with_buffer_stream(stream),
        std::make_shared<ms::NullSurfaceObserver>());

    auto compositor = conf.the_compositor();

    compositor->start();
    session.default_surface()->configure(mir_window_attrib_swapinterval, 0);

    std::thread client_thread{
        [stream]
        {
            for (int i = 0; i < 500; ++i)
            {
                stream->submit_buffer(nullptr);
                std::this_thread::sleep_for(std::chrono::microseconds{50});
            }
        }};

    std::thread snapshot_thread{
        [&session]
        {
            for (int i = 0; i < 500; ++i)
            {
                std::atomic<bool> snapshot_taken1{false};
                std::atomic<bool> snapshot_taken2{false};

                session.take_snapshot(
                    [&](ms::Snapshot const&) { snapshot_taken1 = true; });
                session.take_snapshot(
                    [&](ms::Snapshot const&) { snapshot_taken2 = true; });

                while (!snapshot_taken1 || !snapshot_taken2)
                    std::this_thread::sleep_for(std::chrono::microseconds{50});
            }
        }};

    client_thread.join();
    snapshot_thread.join();
    compositor->stop();
}
