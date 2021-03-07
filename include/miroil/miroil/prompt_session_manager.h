/*
 * Copyright © 2016-2021 Canonical Ltd.
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
 */

#ifndef MIROIL_PROMPT_SESSION_MANAGER_H
#define MIROIL_PROMPT_SESSION_MANAGER_H

#include <miral/application.h>

#include <memory>

namespace mir { namespace scene { class PromptSessionManager; class PromptSession;} }

namespace miroil {
    
class PromptSessionManager
{
public:
    PromptSessionManager(std::shared_ptr<mir::scene::PromptSessionManager> const& promptSessionManager);
    PromptSessionManager(PromptSessionManager const& src);
    PromptSessionManager(PromptSessionManager&& src);
    ~PromptSessionManager();

    auto operator=(PromptSessionManager const& src) -> PromptSessionManager&;
    auto operator=(PromptSessionManager&& src) -> PromptSessionManager&;

    bool operator==(PromptSessionManager const& other);

    auto applicationFor(std::shared_ptr<mir::scene::PromptSession> const& promptSession) const -> miral::Application;
    void resumePromptSession(std::shared_ptr<mir::scene::PromptSession> const& promptSession) const;
    void stopPromptSession(std::shared_ptr<mir::scene::PromptSession> const& promptSession) const;
    void suspendPromptSession(std::shared_ptr<mir::scene::PromptSession> const& promptSession) const;    

private:
    std::shared_ptr<mir::scene::PromptSessionManager> m_promptSessionManager;
};
}

#endif //MIROIL_PROMPT_SESSION_MANAGER_H
