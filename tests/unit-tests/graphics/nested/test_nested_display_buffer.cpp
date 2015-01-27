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
 * Authored by: Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#define MIR_INCLUDE_DEPRECATED_EVENT_HEADER

#include "src/server/graphics/nested/nested_output.h"
#include "src/server/graphics/nested/host_connection.h"
#include "src/server/input/null_input_dispatcher.h"

#include "mir_test_doubles/mock_egl.h"
#include "mir_test_doubles/stub_gl_config.h"
#include "mir_test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace geom = mir::geometry;
namespace mgn = mir::graphics::nested;
namespace mgnd = mir::graphics::nested::detail;
namespace mi = mir::input;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

class NullHostSurface : public mgn::HostSurface
{
public:
    EGLNativeWindowType egl_native_window() override { return {}; }
    void set_event_handler(MirEventDelegate const*) override {}
};

struct NestedDisplayBufferTest : testing::Test
{
    NestedDisplayBufferTest()
        : egl_disp_handle{mock_egl.fake_egl_display,
                          mt::fake_shared(stub_gl_config)},
          default_rect{{100,100}, {800,600}}
    {}
    testing::NiceMock<mtd::MockEGL> mock_egl;
    mtd::StubGLConfig stub_gl_config;
    NullHostSurface null_host_surface;
    mi::NullInputDispatcher null_input_dispatcher;
    mgnd::EGLDisplayHandle egl_disp_handle;
    geom::Rectangle const default_rect;
};

TEST_F(NestedDisplayBufferTest, alpha_enabled_pixel_format_enables_destination_alpha)
{
    mgnd::NestedOutput db{
        egl_disp_handle,
        mt::fake_shared(null_host_surface),
        default_rect,
        mt::fake_shared(null_input_dispatcher),
        mir_pixel_format_abgr_8888};

    EXPECT_TRUE(db.uses_alpha());
}

TEST_F(NestedDisplayBufferTest, non_alpha_pixel_format_disables_destination_alpha)
{
    mgnd::NestedOutput db{
        egl_disp_handle,
        mt::fake_shared(null_host_surface),
        default_rect,
        mt::fake_shared(null_input_dispatcher),
        mir_pixel_format_xbgr_8888};

    EXPECT_FALSE(db.uses_alpha());
}
