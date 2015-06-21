/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Nick Dedekind <nick.dedekind@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_PROMPT_SESSION_LISTENER_H_
#define MIR_TEST_DOUBLES_MOCK_PROMPT_SESSION_LISTENER_H_

#include "mir/scene/prompt_session_listener.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockPromptSessionListener : public scene::PromptSessionListener
{
    virtual ~MockPromptSessionListener() noexcept(true) {}

    MOCK_METHOD1(starting, void(std::shared_ptr<scene::PromptSession> const&));
    MOCK_METHOD1(stopping, void(std::shared_ptr<scene::PromptSession> const&));
    MOCK_METHOD1(suspending, void(std::shared_ptr<scene::PromptSession> const&));
    MOCK_METHOD1(resuming, void(std::shared_ptr<scene::PromptSession> const&));

    MOCK_METHOD2(prompt_provider_added, void(scene::PromptSession const&, std::shared_ptr<scene::Session> const&));
    MOCK_METHOD2(prompt_provider_removed, void(scene::PromptSession const&, std::shared_ptr<scene::Session> const&));
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_MOCK_PROMPT_SESSION_LISTENER_H_
