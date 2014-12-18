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

#ifndef MIR_SCENE_NULL_PROMPT_SESSION_MANAGER_H_
#define MIR_SCENE_NULL_PROMPT_SESSION_MANAGER_H_

#include "mir/scene/prompt_session_manager.h"

namespace mir
{
namespace test
{
namespace doubles
{

class NullPromptSessionManager: public scene::PromptSessionManager
{
public:
    std::shared_ptr<scene::PromptSession> start_prompt_session_for(std::shared_ptr<scene::Session> const&,
                                                                   scene::PromptSessionCreationParameters const&) const
    {
        return std::shared_ptr<scene::PromptSession>();
    }

    void stop_prompt_session(std::shared_ptr<scene::PromptSession> const&) const
    {
    }

    void suspend_prompt_session(std::shared_ptr<scene::PromptSession> const&) const override
    {
    }

    void resume_prompt_session(std::shared_ptr<scene::PromptSession> const&) const override
    {
    }

    void add_prompt_provider(std::shared_ptr<scene::PromptSession> const&,
                             std::shared_ptr<scene::Session> const&) const
    {
    }

    void remove_session(std::shared_ptr<scene::Session> const&) const
    {
    }

    std::shared_ptr<scene::Session> application_for(std::shared_ptr<scene::PromptSession> const&) const
    {
        return std::shared_ptr<scene::Session>();
    }

    std::shared_ptr<scene::Session> helper_for(std::shared_ptr<scene::PromptSession> const&) const
    {
        return std::shared_ptr<scene::Session>();
    }

    void for_each_provider_in(std::shared_ptr<scene::PromptSession> const&,
                              std::function<void(std::shared_ptr<scene::Session> const&)> const&) const
    {
    }
};

}
}
}

#endif // MIR_SCENE_NULL_PROMPT_SESSION_MANAGER_H_
