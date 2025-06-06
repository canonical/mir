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

#ifndef MIR_TEST_DOUBLES_MOCK_CURSOR_H
#define MIR_TEST_DOUBLES_MOCK_CURSOR_H

#include "mir/graphics/cursor.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mir
{
namespace test
{
namespace doubles
{
struct MockCursor : public mir::graphics::Cursor
{
    MOCK_METHOD0(show, void());
    MOCK_METHOD1(show, void(std::shared_ptr<mir::graphics::CursorImage> const&));
    MOCK_METHOD0(hide, void());

    MOCK_METHOD1(move_to, void(mir::geometry::Point));
    MOCK_METHOD1(scale, void(float));
    MOCK_METHOD0(renderable, std::shared_ptr<mir::graphics::Renderable>());
    MOCK_METHOD(bool, needs_compositing, (), (const, override));
};
}
}
}

#endif //MIR_TEST_DOUBLES_MOCK_CURSOR_H
