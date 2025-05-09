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
#include <memory>

namespace mir::test::doubles
{
struct MockCursor : public graphics::Cursor
{
    MOCK_METHOD(void, show, (std::shared_ptr<graphics::CursorImage> const&), (override));
    MOCK_METHOD(void, hide, (), (override));
    MOCK_METHOD(void, move_to, (mir::geometry::Point), (override));
    MOCK_METHOD(void, scale, (float), (override));
    MOCK_METHOD(bool, is, (std::shared_ptr<mir::graphics::Renderable> const&), (const override));
};
}

#endif //MIR_TEST_DOUBLES_MOCK_CURSOR_H
