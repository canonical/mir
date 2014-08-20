/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_DISPLAY_LAYOUT_H_
#define MIR_TEST_DOUBLES_MOCK_DISPLAY_LAYOUT_H_

#include "mir/shell/display_layout.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class MockDisplayLayout : public shell::DisplayLayout
{
public:
    MOCK_METHOD1(clip_to_output, void(geometry::Rectangle& rect));
    MOCK_METHOD1(size_to_output, void(geometry::Rectangle& rect));
    MOCK_METHOD2(place_in_output, void(graphics::DisplayConfigurationOutputId id,
                                       geometry::Rectangle& rect));
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_DISPLAY_LAYOUT_H_ */

