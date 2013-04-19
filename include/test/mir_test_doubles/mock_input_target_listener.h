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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_INPUT_TARGET_LISTENER_H_
#define MIR_TEST_DOUBLES_MOCK_INPUT_TARGET_LISTENER_H_

#include "mir/shell/input_target_listener.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockInputTargetListener : public shell::InputTargetListener
{
    virtual ~MockInputTargetListener() noexcept(true) {}
    MOCK_METHOD1(input_application_opened, void(std::shared_ptr<input::SessionTarget> const& application));
    MOCK_METHOD1(input_application_closed, void(std::shared_ptr<input::SessionTarget> const& application));
    MOCK_METHOD2(input_surface_opened, void(std::shared_ptr<input::SessionTarget> const& application,
        std::shared_ptr<input::SurfaceTarget> const& opened_surface));
    MOCK_METHOD1(input_surface_closed, void(std::shared_ptr<input::SurfaceTarget> const& closed_surface));
    MOCK_METHOD1(focus_changed, void(std::shared_ptr<input::SurfaceTarget> const& focus_surface));
    MOCK_METHOD0(focus_cleared, void());
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_MOCK_INPUT_TARGET_LISTENER_H_
