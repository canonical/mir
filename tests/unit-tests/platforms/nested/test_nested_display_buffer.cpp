/*
 * Copyright © 2016 Canonical Ltd.
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

#include "src/server/graphics/nested/display_buffer.h"

#include "mir/events/event_builders.h"

#include "mir/test/doubles/mock_egl.h"
#include "mir/test/doubles/stub_gl_config.h"
#include "mir/test/doubles/stub_host_connection.h"
#include "mir/test/doubles/stub_renderable.h"
#include "mir/test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace mgnd = mgn::detail;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace geom = mir::geometry;

namespace
{

class EventHostSurface : public mgn::HostSurface
{
public:
    EGLNativeWindowType egl_native_window() override { return {}; }

    void set_event_handler(mir_surface_event_callback cb, void* ctx) override
    {
        std::lock_guard<std::mutex> lock{event_mutex};
        event_handler = cb;
        event_context = ctx;
    }

    void emit_input_event()
    {
        auto const ev = mir::events::make_event(
            MirInputDeviceId(), {}, std::vector<uint8_t>{},
            MirKeyboardAction(), 0, 0, MirInputEventModifiers());

        std::lock_guard<std::mutex> lock{event_mutex};
        // Normally we shouldn't call external code under lock, but, for our
        // test use case, doing so is safe and simplifies the test code.
        if (event_handler)
            event_handler(nullptr, ev.get(), event_context);
    }

private:
    std::mutex event_mutex;
    mir_surface_event_callback event_handler;
    void* event_context;
};

struct NestedDisplayBuffer : testing::Test
{
    auto create_display_buffer(mgn::PassthroughOption option)
    {
        return std::make_shared<mgnd::DisplayBuffer>(
            egl_display,
            mt::fake_shared(host_surface),
            rect,
            MirPixelFormat{},
            std::make_shared<mtd::StubHostConnection>(),
            option
            );
    }

    auto create_display_buffer()
    {
        return create_display_buffer(mgn::PassthroughOption::disabled);
    }
    

    geom::Size const size { 1024, 768 }; 
    geom::Rectangle const rect {{0,0}, size};
    testing::NiceMock<mtd::MockEGL> mock_egl;
    mgnd::EGLDisplayHandle egl_display{nullptr, std::make_shared<mtd::StubGLConfig>()};
    EventHostSurface host_surface;
};

}

// Regression test for LP: #1612012
// This test is trying to reproduce a race, so it's not deterministic, but
// in practice the reproduction rate is very close to 100%.
TEST_F(NestedDisplayBuffer, event_dispatch_does_not_race_with_destruction)
{
    auto display_buffer = create_display_buffer();
    std::thread t{
        [&]
        {
            for (int i = 0; i < 100; ++i)
                host_surface.emit_input_event();
        }};

    display_buffer.reset();
    t.join();
}

TEST_F(NestedDisplayBuffer, DISABLED_respects_passthrough_option)
{
    auto optimizable_list = mg::RenderableList{ std::make_shared<mtd::StubRenderable>(rect) };

    auto enabled_display_buffer = create_display_buffer(mgn::PassthroughOption::enabled);
    auto disabled_display_buffer = create_display_buffer(mgn::PassthroughOption::disabled);

    EXPECT_TRUE(enabled_display_buffer->post_renderables_if_optimizable(optimizable_list));
    EXPECT_FALSE(disabled_display_buffer->post_renderables_if_optimizable(optimizable_list));
}
