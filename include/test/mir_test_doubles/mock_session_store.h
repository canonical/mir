/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
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

#ifndef MIR_TEST_DOUBLES_MOCK_SESSION_STORE_H_
#define MIR_TEST_DOUBLES_MOCK_SESSION_STORE_H_

#include "mir/shell/session_store.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockSessionStore : public shell::SessionStore
{
    MOCK_METHOD1(open_session, std::shared_ptr<shell::Session>(std::string const&));
    MOCK_METHOD1(close_session, void(std::shared_ptr<shell::Session> const&));

    MOCK_METHOD2(tag_session_with_lightdm_id, void(std::shared_ptr<shell::Session> const&, int));
    MOCK_METHOD1(focus_session_with_lightdm_id, void(int));

    MOCK_METHOD0(shutdown, void());
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_MOCK_SESSION_STORE_H_
