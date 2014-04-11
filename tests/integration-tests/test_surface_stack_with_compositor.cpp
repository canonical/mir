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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_test_doubles/stub_input_registrar.h"
#include "src/server/report/null_report_factory.h"
#include "src/server/scene/surface_stack.h"
#include "mir_test/fake_shared.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ms = mir::scene;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mr = mir::report;

namespace
{

struct SurfaceStackCompositorInteractions : public testing::Test
{
    mtd::StubInputRegistrar stub_input_registrar;
    std::shared_ptr<ms::SceneReport> null_scene_report{mr::null_scene_report()};
    std::shared_ptr<mc::CompositorReport> null_comp_report{null_compositor_report()};
};
}

TEST_F(SurfaceStackCompositorInteractions, composes_on_start_if_told_to_in_constructor)
{
    ms::SurfaceStack stack(mt::fake_shared(stub_input_registrar), null_report);
    StubDisplay stub_display;
    StubDisplayBufferCompositorFactory stub_dbc_factory;

    mc::MultiThreadedCompositor mt_compositor(
        mt::fake_shared(stub_display),
        mt::fake_shared(stack),
        mt::fake_shared(stub_dbc_factory),
        null_comp_report, true);

    mt_compositor.start();
    EXPECT_THAT(stub_primary_db.post_count, Eq(1));
    EXPECT_THAT(stub_secondary_db.post_count, Eq(1));
}

#if 0
TEST_F(SurfaceStackCompositorInteractions, does_not_composes_on_start_if_told_not_to_in_constructor)
{
    ms::SurfaceStack stack(mt::fake_shared(stub_input_registrar), null_report);
    StubDisplay stub_display;
    StubDisplayBufferCompositorFactory stub_dbc_factory;

    mc::MultiThreadedCompositor mt_compositor(
        mt::fake_shared(stub_display),
        mt::fake_shared(stack),
        mt::fake_shared(stub_dbc_factory),
        null_comp_report, false);

    mt_compositor.start();
    EXPECT_THAT(stub_primary_db.post_count, Eq(0));
    EXPECT_THAT(stub_secondary_db.post_count, Eq(0));
}
#endif
//TEST_F(SurfaceStackCompositorInteractions, adding_a_surface_triggers_a_composition_once_the_surface_has_posted)
//TEST_F(SurfaceStackCompositorInteractions, removing_a_surface_triggers_a_composition_immediately)

//TEST_F(SurfaceStackCompositorInteractions, moving_a_surface_triggers_composition)
//TEST_F(SurfaceStackCompositorInteractions, surface_buffer_updates_triggers_composition)
//TEST_F(SurfaceStackCompositorInteractions, surface_buffer_updates_triggers_composition_until_all_consumed)
