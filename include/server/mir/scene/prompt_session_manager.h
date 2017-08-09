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

#ifndef MIR_SCENE_PROMPT_SESSION_MANAGER_H_
#define MIR_SCENE_PROMPT_SESSION_MANAGER_H_

#include <sys/types.h>
#include <memory>
#include <functional>

namespace mir
{
namespace scene
{
class Session;
class PromptSession;
struct PromptSessionCreationParameters;

class PromptSessionManager
{
public:
    virtual ~PromptSessionManager() = default;

    /**
     * Start a new prompt session
     *   \param [in] session  The prompt helper session
     *   \param [in] params   The creation parameters for constructing the prompt session
     */
    virtual std::shared_ptr<PromptSession> start_prompt_session_for(std::shared_ptr<Session> const& session,
                                                                    PromptSessionCreationParameters const& params) const = 0;

    /**
     * Stop a started prompt session
     *   \param [in] prompt_session  The prompt session
     */
    virtual void stop_prompt_session(std::shared_ptr<PromptSession> const& prompt_session) const = 0;

    /**
     * Suspend a prompt session
     *   \param [in] prompt_session  The prompt session
     */
    virtual void suspend_prompt_session(std::shared_ptr<PromptSession> const& prompt_session) const = 0;

    /**
     * Resume a suspended prompt session
     *   \param [in] prompt_session  The prompt session
     */
    virtual void resume_prompt_session(std::shared_ptr<PromptSession> const& prompt_session) const = 0;

    /**
     * Add a prompt provider to an existing prompt session
     *   \param [in] prompt_session  The prompt session
     *   \param [in] prompt_provider The prompt provider to add to the prompt session
     */
    virtual void add_prompt_provider(std::shared_ptr<PromptSession> const& prompt_session,
                                     std::shared_ptr<Session> const& prompt_provider) const = 0;

    /**
     * Remove a session from all associated prompt sessions
     *   \param [in] session  The new session that is to be removed
     */
    virtual void remove_session(std::shared_ptr<Session> const& session) const = 0;

    /**
     * Retrieve the application session for a prompt session
     *   \param [in] prompt_session  The prompt session
     */
    virtual std::shared_ptr<Session> application_for(std::shared_ptr<PromptSession> const& prompt_session) const = 0;

    /**
     * Retrieve the helper session for a prompt session
     *   \param [in] prompt_session  The prompt session
     */
    virtual std::shared_ptr<Session> helper_for(std::shared_ptr<PromptSession> const& prompt_session) const = 0;

    /**
     * Iterate over all the prompt providers associated with a prompt session
     *   \param [in] prompt_session  The prompt session
     *   \param [in] f               The callback function to call for each provider
     */
    virtual void for_each_provider_in(std::shared_ptr<PromptSession> const& prompt_session,
                                      std::function<void(std::shared_ptr<Session> const& prompt_provider)> const& f) const = 0;

protected:
    PromptSessionManager() = default;
    PromptSessionManager(const PromptSessionManager&) = delete;
    PromptSessionManager& operator=(const PromptSessionManager&) = delete;
};

}
}

#endif // MIR_SCENE_PROMPT_SESSION_MANAGER_H_
