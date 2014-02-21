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

#ifndef MIR_SHELL_TRUSTED_SESSION_IMPL_H_
#define MIR_SHELL_TRUSTED_SESSION_IMPL_H_

#include "mir/shell/trusted_session.h"

#include <vector>
#include <atomic>

namespace mir
{
namespace shell
{
class TrustedSessionCreationParameters;
}

namespace scene
{
class SessionContainer;

class TrustedSessionImpl : public shell::TrustedSession
{
public:
    TrustedSessionImpl(shell::TrustedSessionCreationParameters const& parameters);
    virtual ~TrustedSessionImpl();

    frontend::SessionId id() const override;

    std::vector<pid_t> get_applications() const override;

    bool get_started() const override;
    std::shared_ptr<shell::Session> get_trusted_helper() const override;
    void stop() override;

    void add_child_session(std::shared_ptr<shell::Session> const& session) override;

    static std::shared_ptr<shell::TrustedSession> create_for(std::shared_ptr<shell::Session> const& session,
                                                            std::shared_ptr<SessionContainer> const& container,
                                                            shell::TrustedSessionCreationParameters const& parameters);

protected:
    TrustedSessionImpl(const TrustedSessionImpl&) = delete;
    TrustedSessionImpl& operator=(const TrustedSessionImpl&) = delete;

private:
    frontend::SessionId next_id();

    std::vector<pid_t> const applications;

    bool started;
    std::atomic<int> next_session_id;
    frontend::SessionId current_id;
    std::shared_ptr<shell::Session> trusted_helper;
};

}
}

#endif // MIR_SHELL_TRUSTED_PROMPT_SESSION_IMPL_H_
