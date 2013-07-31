/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
#include "mir/input/null_input_configuration.h"
#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/compositor/compositor.h"
#include "mir/shell/application_session.h"
#include "mir/shell/pixel_buffer.h"
#include "mir/shell/placement_strategy.h"
#include "mir/shell/surface.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/shell/null_session_listener.h"
#include "mir/surfaces/buffer_stream.h"
#include "mir/compositor/renderer.h"
#include "mir/compositor/renderer_factory.h"
#include "mir/frontend/communicator.h"

#include "mir_test_doubles/stub_buffer.h"
#include "mir_test_doubles/null_display.h"
#include "mir_test_doubles/null_event_sink.h"
#include "mir_test_doubles/stub_display_buffer.h"

#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mtd = mir::test::doubles;
namespace msh = mir::shell;
namespace mi = mir::input;
namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{

struct TestServerConfiguration : public mir::DefaultServerConfiguration
{
    TestServerConfiguration() : DefaultServerConfiguration(0, nullptr) {}

    std::shared_ptr<mi::InputConfiguration> the_input_configuration() override
    {
        if (!input_configuration)
            input_configuration = std::make_shared<mi::NullInputConfiguration>();

        return input_configuration;
    }

    std::shared_ptr<mf::Communicator> the_communicator() override
    {
        struct NullCommunicator : public mf::Communicator
        {
            void start() {}
            void stop() {}
        };

        return std::make_shared<NullCommunicator>();
    }

    std::shared_ptr<mc::GraphicBufferAllocator> the_buffer_allocator() override
    {
        struct StubBufferAllocator : public mc::GraphicBufferAllocator
        {
            std::shared_ptr<mg::Buffer> alloc_buffer(mc::BufferProperties const& buffer_properties)
            {
                return std::make_shared<mtd::StubBuffer>(buffer_properties);
            }

            std::vector<geom::PixelFormat> supported_pixel_formats() { return {}; }
        };

        return buffer_allocator(
            []
            {
                return std::make_shared<StubBufferAllocator>();
            });
    }

    std::shared_ptr<mc::RendererFactory> the_renderer_factory() override
    {
        struct StubRenderer : public mc::Renderer
        {
            void clear() {}
            void render(std::function<void(std::shared_ptr<void> const&)>,
                        mc::CompositingCriteria const&, mir::surfaces::BufferStream& stream)
            {
                stream.lock_compositor_buffer();
            }

            void ensure_no_live_buffers_bound() {}
        };

        struct StubRendererFactory : public mc::RendererFactory
        {
            std::unique_ptr<mc::Renderer> create_renderer_for(geom::Rectangle const&)
            {
                auto raw = new StubRenderer{};
                return std::unique_ptr<StubRenderer>(raw);
            }
        };

        return std::make_shared<StubRendererFactory>();
    }


    std::shared_ptr<msh::PixelBuffer> the_shell_pixel_buffer() override
    {
        struct StubPixelBuffer : public msh::PixelBuffer
        {
            void fill_from(mg::Buffer&) {}
            void const* as_argb_8888() { return nullptr; }
            geom::Size size() const { return geom::Size(); }
            geom::Stride stride() const { return geom::Stride(); }
        };

        return shell_pixel_buffer(
            []
            {
                return std::make_shared<StubPixelBuffer>();
            });
    }

    std::shared_ptr<mg::Display> the_display() override
    {
        struct StubDisplay : public mtd::NullDisplay
        {
            StubDisplay()
                : buffers{mtd::StubDisplayBuffer{geom::Rectangle{{0,0},{100,100}}},
                          mtd::StubDisplayBuffer{geom::Rectangle{{100,0},{100,100}}},
                          mtd::StubDisplayBuffer{geom::Rectangle{{0,100},{100,100}}}}
            {

            }

            void for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f)
            {
                for (auto& db : buffers)
                    f(db);
            }

            std::vector<mtd::StubDisplayBuffer> buffers;
        };

        return display(
            []()
            {
                return std::make_shared<StubDisplay>();
            });
    }

    std::shared_ptr<mi::NullInputConfiguration> input_configuration;
};

}

TEST(ShellSessionTest, stress_test_take_snapshot)
{
    TestServerConfiguration conf;

    msh::ApplicationSession session{
        conf.the_shell_surface_factory(),
        "stress",
        conf.the_shell_snapshot_strategy(),
        std::make_shared<msh::NullSessionListener>(),
        std::make_shared<mtd::NullEventSink>()
    };
    session.create_surface(msh::a_surface());

    auto compositor = conf.the_compositor();

    compositor->start();

    std::thread client_thread{
        [&session]
        {
            for (int i = 0; i < 500; ++i)
            {
                auto surface = session.default_surface();
                surface->advance_client_buffer();
                std::this_thread::sleep_for(std::chrono::microseconds{50});
            }
        }};

    std::thread snapshot_thread{
        [&session]
        {
            for (int i = 0; i < 500; ++i)
            {
                bool snapshot_taken1 = false;
                bool snapshot_taken2 = false;

                session.take_snapshot(
                    [&](msh::Snapshot const&) { snapshot_taken1 = true; });
                session.take_snapshot(
                    [&](msh::Snapshot const&) { snapshot_taken2 = true; });

                while (!snapshot_taken1 || !snapshot_taken2)
                    std::this_thread::sleep_for(std::chrono::microseconds{50});
            }
        }};

    client_thread.join();
    snapshot_thread.join();
    compositor->stop();
}
