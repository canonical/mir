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

#ifndef MIR_TEST_DOUBLES_STUB_DISPLAY_LAYOUT_H
#define MIR_TEST_DOUBLES_STUB_DISPLAY_LAYOUT_H

#include <mir/shell/display_layout.h>

namespace mir
{
namespace test
{
namespace doubles
{
class StubDisplayLayout : public mir::shell::DisplayLayout
{
public:
    void clip_to_output(mir::geometry::Rectangle&) override {}
    void size_to_output(mir::geometry::Rectangle&) override {}
    auto place_in_output(mir::graphics::DisplayConfigurationOutputId, mir::geometry::Rectangle&) -> bool override
    {
        return false;
    }
};
}
}
}

#endif //MIR_TEST_DOUBLES_STUB_DISPLAY_LAYOUT_H
