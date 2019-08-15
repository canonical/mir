/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_FRONTEND_NO_PROMPT_SHELL_H_
#define MIR_FRONTEND_NO_PROMPT_SHELL_H_

#include "shell_wrapper.h"

namespace mir
{
namespace frontend
{
class NoPromptShell : public ShellWrapper
{
public:

    using ShellWrapper::ShellWrapper;

    std::shared_ptr<PromptSession> start_prompt_session_for(
        std::shared_ptr<scene::Session> const& session,
        scene::PromptSessionCreationParameters const& params) override;

    void add_prompt_provider_for(
        std::shared_ptr<PromptSession> const& prompt_session,
        std::shared_ptr<scene::Session> const& session) override;

    void stop_prompt_session(
        std::shared_ptr<PromptSession> const& prompt_session) override;
};
}
}

#endif /* MIR_FRONTEND_NO_PROMPT_SHELL_H_ */
