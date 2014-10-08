/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "src/server/scene/basic_surface.h"
#include "src/server/report/null_report_factory.h"
#include "src/server/compositor/buffer_stream_surfaces.h"
#include "src/server/compositor/buffer_queue.h"

#include "mir_test_doubles/null_surface_configurator.h"
#include "mir_test_doubles/stub_buffer_allocator.h"
#include "mir_test_doubles/stub_frame_dropping_policy_factory.h"
#include "mir_test_doubles/stub_input_sender.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace geom = mir::geometry;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace mr = mir::report;
namespace ms = mir::scene;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{
struct SurfaceComposition : Test
{
    auto create_surface() const
    -> std::shared_ptr<ms::Surface>
    {
        return std::make_shared<ms::BasicSurface>(
            std::string("SurfaceComposition"),
            geom::Rectangle{{},{}},
            false,
            create_buffer_stream(),
            create_input_channel(),
            create_input_sender(),
            create_surface_configurator(),
            create_cursor_image(),
            mr::null_scene_report());

    }

    int const number_of_buffers = 3;

    auto create_buffer_stream() const
    ->std::shared_ptr<mc::BufferStream>
    {
        return std::make_shared<mc::BufferStreamSurfaces>(create_buffer_bundle());
    }

    auto create_buffer_bundle() const
    -> std::shared_ptr<mc::BufferBundle>
    {
        return std::make_shared<mc::BufferQueue>(
            number_of_buffers,
            allocator,
            basic_properties,
            policy_factory);
    }

    auto create_input_channel() const
    -> std::shared_ptr<mi::InputChannel> { return {}; }

    auto create_cursor_image() const
    -> std::shared_ptr<mg::CursorImage>  { return {}; }

    auto create_surface_configurator() const
    -> std::shared_ptr<ms::SurfaceConfigurator>
    { return std::make_shared<mtd::NullSurfaceConfigurator>(); }

    auto create_input_sender() const
    -> std::shared_ptr<mi::InputSender>
    { return std::make_shared<mtd::StubInputSender>(); }

    std::shared_ptr<mg::GraphicBufferAllocator> const allocator
        {std::make_shared<mtd::StubBufferAllocator>()};

    mg::BufferProperties const basic_properties
        { geom::Size{3, 4}, mir_pixel_format_abgr_8888, mg::BufferUsage::hardware };

    mtd::StubFrameDroppingPolicyFactory policy_factory;
};
}

// Presumptive cause of lp:1376324
TEST_F(SurfaceComposition, does_not_send_client_buffers_to_dead_surfaces)
{
    auto surface = create_surface();

    mg::Buffer* old_buffer{nullptr};

    auto const callback = [&] (mg::Buffer* new_buffer)
        {
            // If surface is dead then callback is not expected
            EXPECT_THAT(surface.get(), NotNull());
            old_buffer = new_buffer;
        };

    // Exhaust the buffers to ensure we have a pending swap to complete
    for (auto i = 0; i != number_of_buffers; ++i)
        surface->swap_buffers(old_buffer, callback);

    auto const renderable = surface->compositor_snapshot(this);

    surface.reset();
}
