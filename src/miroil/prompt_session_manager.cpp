/*
 * Copyright © 2021 Canonical Ltd.
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

#include "miroil/prompt_session_manager.h"
#include "mir/scene/prompt_session_manager.h"

miroil::PromptSessionManager::PromptSessionManager(std::shared_ptr<mir::scene::PromptSessionManager> const& promptSessionManager) 
:    m_promptSessionManager{promptSessionManager}
{
}

miroil::PromptSessionManager::PromptSessionManager(PromptSessionManager&& src) = default;

miroil::PromptSessionManager::PromptSessionManager(PromptSessionManager const& src) = default;

miroil::PromptSessionManager::~PromptSessionManager() = default;

bool miroil::PromptSessionManager::operator==(PromptSessionManager const& other)
{
    return m_promptSessionManager == other.m_promptSessionManager;
}

miral::Application miroil::PromptSessionManager::applicationFor(std::shared_ptr<mir::scene::PromptSession> const& promptSession) const
{
    return m_promptSessionManager->application_for(promptSession);
}

void miroil::PromptSessionManager::stopPromptSession(std::shared_ptr<mir::scene::PromptSession> const& promptSession) const
{
    m_promptSessionManager->stop_prompt_session(promptSession);
}

void miroil::PromptSessionManager::suspendPromptSession(std::shared_ptr<mir::scene::PromptSession> const& promptSession) const
{
    m_promptSessionManager->suspend_prompt_session(promptSession);
}

void miroil::PromptSessionManager::resumePromptSession(std::shared_ptr<mir::scene::PromptSession> const& promptSession) const
{
    m_promptSessionManager->resume_prompt_session(promptSession);
}
