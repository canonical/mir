/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_TEST_DOUBLES_MOCK_COMPOSITOR_REPORT_H_
#define MIR_TEST_DOUBLES_MOCK_COMPOSITOR_REPORT_H_

#include "mir/compositor/compositor_report.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class MockCompositorReport : public compositor::CompositorReport
{
public:
    MOCK_METHOD(void, added_display, (int,int,int,int, compositor::CompositorReport::SubCompositorId), (override));
    MOCK_METHOD(void, began_frame, (compositor::CompositorReport::SubCompositorId), (override));
    MOCK_METHOD(void, renderables_in_frame,
                 (compositor::CompositorReport::SubCompositorId, graphics::RenderableList const&), (override));
    MOCK_METHOD(void, rendered_frame, (compositor::CompositorReport::SubCompositorId), (override));
    MOCK_METHOD(void, finished_frame, (compositor::CompositorReport::SubCompositorId), (override));
    MOCK_METHOD(void, started, (), (override));
    MOCK_METHOD(void, stopped, (), (override));
    MOCK_METHOD(void, scheduled, (), (override));
};

} // namespace doubles
} // namespace test
} // namespace mir

#endif
