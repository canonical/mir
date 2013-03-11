/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_SESSION_H_
#define MIR_TEST_DOUBLES_STUB_SESSION_H_

#include "mir/shell/session.h"

namespace mir
{
namespace test
{
namespace doubles
{

struct StubSession : public shell::Session
{
    shell::SurfaceId create_surface(shell::SurfaceCreationParameters const& /* params */)
    {
        return shell::SurfaceId{0};
    }
    void destroy_surface(shell::SurfaceId /* surface */)
    {
    }
    std::shared_ptr<shell::Surface> get_surface(shell::SurfaceId /* surface */) const
    {
        return std::shared_ptr<shell::Surface>();
    }
    std::string name()
    {
        return std::string();
    }
    void shutdown()
    {
    }
    void hide()
    {
    }
    void show()
    {
    }
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_STUB_SESSION_H_
