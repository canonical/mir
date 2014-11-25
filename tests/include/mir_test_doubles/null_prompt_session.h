/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 * Authored by: Nick Dedekind <nick.dedekind@gmail.com>
 */

#ifndef MIR_TEST_DOUBLES_NULL_PROMPT_SESSION_H_
#define MIR_TEST_DOUBLES_NULL_PROMPT_SESSION_H_

#include "mir/scene/prompt_session.h"

namespace mir
{
namespace test
{
namespace doubles
{
class NullPromptSession : public scene::PromptSession
{
public:
    void start(std::shared_ptr<scene::Session> const&) override
    {
    }

    void stop(std::shared_ptr<scene::Session> const&) override
    {
    }

    void suspend(std::shared_ptr<scene::Session> const&) override
    {
    }

    void resume(std::shared_ptr<scene::Session> const&) override
    {
    }
};
}
}
}

#endif /* MIR_TEST_DOUBLES_NULL_PROMPT_SESSION_H_ */
