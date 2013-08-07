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

#ifndef MIR_TEST_DOUBLES_MOCK_FOCUS_SETTER_H_
#define MIR_TEST_DOUBLES_MOCK_FOCUS_SETTER_H_

#include "mir/shell/focus_setter.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockFocusSetter : public shell::FocusSetter
{
    MOCK_METHOD1(session_opened, void(std::shared_ptr<shell::Session> const&));
    MOCK_METHOD1(session_closed, void(std::shared_ptr<shell::Session> const&));
    MOCK_METHOD1(surface_created_for, void(std::shared_ptr<shell::Session> const&));
    MOCK_CONST_METHOD0(focused_session, std::weak_ptr<shell::Session>());
    MOCK_METHOD0(focus_next, void());
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_MOCK_FOCUS_SETTER_H_
