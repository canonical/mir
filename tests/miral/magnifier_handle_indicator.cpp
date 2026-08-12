/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../../src/miral/magnifier_handle_indicator.h"

#include "../../src/server/report/null_report_factory.h"
#include <mir/test/doubles/fake_display_configuration_observer_registrar.h>
#include <mir/test/doubles/stub_buffer_allocator.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mmc = miral::magnifier_controls;
namespace mr = mir::report;
namespace mtd = mir::test::doubles;

using namespace testing;

TEST(HandleIndicator, excludes_itself_from_the_capture_compositor)
{
    int capture_compositor{};
    int display_compositor{};
    miral::HandleIndicator indicator{
        {{0, 0}, {48, 48}},
        mmc::HandleKind::drag,
        mc::CompositorID{&capture_compositor},
        std::make_shared<mtd::StubBufferAllocator>(),
        mr::null_scene_report(),
        std::make_shared<mtd::FakeDisplayConfigurationObserverRegistrar>()};
    indicator.show();

    EXPECT_THAT(indicator.generate_renderables(&capture_compositor), IsEmpty());
    EXPECT_THAT(indicator.generate_renderables(&display_compositor), SizeIs(1));
}
