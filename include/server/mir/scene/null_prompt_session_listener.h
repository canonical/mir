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

#ifndef MIR_SCENE_NULL_PROMPT_SESSION_LISTENER_H_
#define MIR_SCENE_NULL_PROMPT_SESSION_LISTENER_H_

#include "mir/scene/prompt_session_listener.h"

namespace mir
{
namespace scene
{
class NullPromptSessionListener : public PromptSessionListener
{
public:
    void starting(std::shared_ptr<PromptSession> const&) override {}
    void stopping(std::shared_ptr<PromptSession> const&) override {}

    void prompt_provider_added(PromptSession const&, std::shared_ptr<Session> const&) override {}
    void prompt_provider_removed(PromptSession const&, std::shared_ptr<Session> const&) override {}
};
}
}

#endif // MIR_SHELL_NULL_PROMPT_SESSION_LISTENER_H_
