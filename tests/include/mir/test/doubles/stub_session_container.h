/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_SESSION_CONTAINER_H
#define MIR_TEST_DOUBLES_STUB_SESSION_CONTAINER_H

#include "mir/scene/session_container.h"
#include <memory>
#include <vector>

namespace mir
{
namespace test
{
namespace doubles
{

class StubSessionContainer : public scene::SessionContainer
{
public:
    void insert_session(std::shared_ptr<scene::Session> const& session)
    {
        sessions.push_back(session);
    }

    void remove_session(std::shared_ptr<scene::Session> const&)
    {
    }

    void for_each(std::function<void(std::shared_ptr<scene::Session> const&)> f) const
    {
        for (auto const& session : sessions)
            f(session);
    }

    std::shared_ptr<scene::Session> successor_of(std::shared_ptr<scene::Session> const&) const
    {
        return {};
    }

private:
    std::vector<std::shared_ptr<scene::Session>> sessions;
};

}
}
}
#endif
