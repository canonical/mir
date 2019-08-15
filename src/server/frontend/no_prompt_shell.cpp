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

#include "no_prompt_shell.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mf = mir::frontend;

namespace
{
char const* const prompt_sessions_disabled = "Prompt sessions disabled";
}

std::shared_ptr<mf::PromptSession> mf::NoPromptShell::start_prompt_session_for(
    std::shared_ptr<scene::Session> const& /*session*/,
    scene::PromptSessionCreationParameters const& /*params*/)
{
    BOOST_THROW_EXCEPTION(std::runtime_error(prompt_sessions_disabled));
}

void mf::NoPromptShell::add_prompt_provider_for(
    std::shared_ptr<PromptSession> const& /*prompt_session*/,
    std::shared_ptr<scene::Session> const& /*session*/)
{
    BOOST_THROW_EXCEPTION(std::runtime_error(prompt_sessions_disabled));
}

void mf::NoPromptShell::stop_prompt_session(
    std::shared_ptr<PromptSession> const& /*prompt_session*/)
{
    BOOST_THROW_EXCEPTION(std::runtime_error(prompt_sessions_disabled));
}
