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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
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
    MOCK_METHOD5(added_display,
                 void(int,int,int,int,
                      compositor::CompositorReport::SubCompositorId));
    MOCK_METHOD1(began_frame,
                 void(compositor::CompositorReport::SubCompositorId));
    MOCK_METHOD2(renderables_in_frame,
                 void(compositor::CompositorReport::SubCompositorId, graphics::RenderableList const&));
    MOCK_METHOD1(rendered_frame,
                 void(compositor::CompositorReport::SubCompositorId));
    MOCK_METHOD1(finished_frame,
                 void(compositor::CompositorReport::SubCompositorId));
    MOCK_METHOD0(started, void());
    MOCK_METHOD0(stopped, void());
    MOCK_METHOD0(scheduled, void());
};

} // namespace doubles
} // namespace test
} // namespace mir

#endif
