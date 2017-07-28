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
 * Authored By: Nick Dedekind <nick.dedekind@canonical.com>
 */

#ifndef MIR_SCENE_PROMPT_SESSION_H_
#define MIR_SCENE_PROMPT_SESSION_H_

#include "mir/frontend/prompt_session.h"

namespace mir
{
namespace scene
{
class Session;

class PromptSession : public frontend::PromptSession
{
public:
    /**
     * Start a prompt session
     *   \param [in] helper_session  The prompt session helper session
     */
    virtual void start(std::shared_ptr<Session> const& helper_session) = 0;

    /**
     * Stop a prompt session
     *   \param [in] helper_session  The prompt session helper session
     */
    virtual void stop(std::shared_ptr<Session> const& helper_session) = 0;

    /**
     * Suspend a prompt session
     *   \param [in] helper_session  The prompt session helper session
     */
    virtual void suspend(std::shared_ptr<Session> const& helper_session) = 0;

    /**
     * Resume a prompt session
     *   \param [in] helper_session  The prompt session helper session
     */
    virtual void resume(std::shared_ptr<Session> const& helper_session) = 0;
};

}
}

#endif // MIR_SHELL_PROMPT_SESSION_H_
