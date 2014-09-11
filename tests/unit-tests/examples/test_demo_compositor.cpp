/*
 * Copyright © 2014 Canonical Ltd.
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

#include "playground/demo-shell/demo_compositor.h"

#include "mir/geometry/rectangle.h"
#include "src/server/scene/surface_stack.h"
#include "src/server/scene/basic_surface.h"
#include "src/server/report/null_report_factory.h"

#include "mir_test_doubles/mock_gl.h"
#include "mir_test_doubles/stub_display_buffer.h"
#include "mir_test_doubles/stub_gl_program_factory.h"
#include "mir_test_doubles/stub_buffer_stream.h"
#include "mir_test_doubles/null_surface_configurator.h"
#include "mir_test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtd = mir::test::doubles;
namespace geom = mir::geometry;
namespace me = mir::examples;
namespace mt = mir::test;
namespace ms = mir::scene;

namespace
{

struct DemoCompositor : testing::Test
{
    DemoCompositor()
    {
        using namespace testing;

        surface_stack.add_surface(
            mt::fake_shared(stub_surface),
            ms::DepthId{0},
            mir::input::InputReceptionMode::normal);

        post_surface_buffer();
    }

    void post_surface_buffer()
    {
        mir::graphics::Buffer* old_buffer;

        stub_surface.swap_buffers(
            nullptr,
            [&old_buffer] (mir::graphics::Buffer* buffer)
            {
                old_buffer = buffer;
            });

        stub_surface.swap_buffers(
            old_buffer,
            [] (mir::graphics::Buffer*) {});
    }

    testing::NiceMock<mtd::MockGL> mock_gl;
    mtd::StubDisplayBuffer stub_display_buffer{
        geom::Rectangle{{0,0}, {1000,1000}}};
    mir::scene::SurfaceStack surface_stack{mir::report::null_scene_report()};
    mtd::StubGLProgramFactory stub_factory;
    ms::BasicSurface stub_surface{
        std::string("stub"),
        geom::Rectangle{{0,0},{100,100}},
        false,
        std::make_shared<mtd::StubBufferStream>(),
        std::shared_ptr<mir::input::InputChannel>(),
        std::shared_ptr<mir::input::InputSender>(),
        std::make_shared<mtd::NullSurfaceConfigurator>(),
        std::shared_ptr<mir::graphics::CursorImage>(),
        mir::report::null_scene_report()};

    me::DemoCompositor demo_compositor{
        stub_display_buffer,
        mt::fake_shared(surface_stack),
        stub_factory,
        mir::report::null_compositor_report()};
};

}

// Regression test for https://bugs.launchpad.net/mir/+bug/1359487
TEST_F(DemoCompositor, sets_surface_visibility)
{
    using namespace testing;

    demo_compositor.composite();

    EXPECT_THAT(
        stub_surface.query(mir_surface_attrib_visibility),
        Eq(mir_surface_visibility_exposed));

    stub_surface.hide();

    demo_compositor.composite();

    EXPECT_THAT(
        stub_surface.query(mir_surface_attrib_visibility),
        Eq(mir_surface_visibility_occluded));
}
