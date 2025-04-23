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

#ifndef MIR_TEST_DOUBLES_MOCK_SCREEN_SHOOTER_H
#define MIR_TEST_DOUBLES_MOCK_SCREEN_SHOOTER_H

#include "mir/compositor/screen_shooter.h"
#include <gmock/gmock.h>

namespace mir::test::doubles
{
class MockScreenShooter : public mir::compositor::ScreenShooter
{
public:
    MOCK_METHOD(void, capture,
        (std::shared_ptr<renderer::software::WriteMappableBuffer> const& buffer,
         mir::geometry::Rectangle const& area,
         std::function<void(std::optional<time::Timestamp>)>&& callback),
        (override));

    MOCK_METHOD(void, capture_with_cursor,
        (std::shared_ptr<renderer::software::WriteMappableBuffer> const& buffer,
         mir::geometry::Rectangle const& area,
         bool with_cursor,
         std::function<void(std::optional<time::Timestamp>)>&& callback),
        (override));

    MOCK_METHOD(void, capture_with_filter,
        (std::shared_ptr<renderer::software::WriteMappableBuffer> const& buffer,
         geometry::Rectangle const& area,
         std::function<bool(std::shared_ptr<mir::compositor::SceneElement const> const&)> const& filter,
         bool with_cursor,
         std::function<void(std::optional<time::Timestamp>)>&& callback),
        (override));
};
}

#endif //MIR_TEST_DOUBLES_MOCK_SCREEN_SHOOTER_H
