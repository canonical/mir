/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_SHELL_H_
#define MIR_TEST_DOUBLES_SHELL_H_

#include "mir/shell/surface_creation_parameters.h"
#include "mir/frontend/shell.h"
#include "mir/frontend/surface_id.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockShell : public frontend::Shell
{
    MOCK_METHOD2(open_session, std::shared_ptr<frontend::Session>(
        std::string const&, std::shared_ptr<frontend::EventSink> const&));
    MOCK_METHOD1(close_session, void(std::shared_ptr<frontend::Session> const&));

    MOCK_METHOD2(create_surface_for, frontend::SurfaceId(std::shared_ptr<frontend::Session> const&, shell::SurfaceCreationParameters const&));
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_SHELL_H_
