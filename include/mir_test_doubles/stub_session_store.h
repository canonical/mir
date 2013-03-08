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

#ifndef MIR_TEST_DOUBLES_STUB_SESSION_STORE_H_
#define MIR_TEST_DOUBLES_STUB_SESSION_STORE_H_

#include "mir/shell/session_store.h"

namespace mir
{
namespace test
{
namespace doubles
{

class StubSessionStore : public shell::SessionStore
{
    std::shared_ptr<shell::Session> open_session(std::string const& /* name */)
    {
        return std::shared_ptr<shell::Session>();
    }
    void close_session(std::shared_ptr<shell::Session> const& /* session */)
    {
    }
    void tag_session_with_lightdm_id(std::shared_ptr<shell::Session> const& /* session */, int /* id */)
    {
    }
    void focus_session_with_lightdm_id(int /* id */)
    {
    }
    void shutdown()
    {
    }
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_STUB_SESSION_STORE_H_
