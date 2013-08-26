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

#ifndef MIR_TEST_DOUBLES_STUB_SHELL_H_
#define MIR_TEST_DOUBLES_STUB_SHELL_H_

#include "mir/frontend/shell.h"
#include "mir_test_doubles/stub_session.h"

namespace mir
{
namespace test
{
namespace doubles
{

struct StubShell : public frontend::Shell
{
    StubShell() : stub_session(std::make_shared<StubSession>())
    {
    }
    std::shared_ptr<frontend::Session> open_session(std::string const& /* name */, std::shared_ptr<frontend::EventSink> const& /* sink */) override
    {
        return stub_session;
    }
    void close_session(std::shared_ptr<frontend::Session> const& /* session */) override
    {
    }
    frontend::SurfaceId create_surface_for(std::shared_ptr<frontend::Session> const& /* session */,
                                        shell::SurfaceCreationParameters const& /* params */) override
    {
        return frontend::SurfaceId{0};
    }
    void handle_surface_created(std::shared_ptr<frontend::Session> const& /* session */) override
    {
    }
    std::shared_ptr<StubSession> const stub_session;
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_STUB_SHELL_H_
