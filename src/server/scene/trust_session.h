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

#ifndef MIR_SCENE_TRUST_SESSION_H_
#define MIR_SCENE_TRUST_SESSION_H_

#include "mir/shell/trust_session.h"

#include <vector>
#include <atomic>
#include <mutex>

namespace mir
{
namespace shell
{
class TrustSessionCreationParameters;
}

namespace scene
{
class SessionContainer;

class TrustSession : public shell::TrustSession
{
public:
    TrustSession(std::weak_ptr<shell::Session> const& session,
                 shell::TrustSessionCreationParameters const& parameters);
    virtual ~TrustSession();

    std::vector<pid_t> get_applications() const override;

    MirTrustSessionState get_state() const override;
    std::weak_ptr<shell::Session> get_trusted_helper() const override;
    void start() override;
    void stop() override;

    void add_trusted_child(std::shared_ptr<shell::Session> const& session);
    void remove_trusted_child(std::shared_ptr<shell::Session> const& session);
    void for_each_trusted_child(std::function<bool(std::shared_ptr<shell::Session> const&)> f, bool reverse) const;

protected:
    TrustSession(const TrustSession&) = delete;
    TrustSession& operator=(const TrustSession&) = delete;

private:
    std::weak_ptr<shell::Session> const trusted_helper;
    std::vector<pid_t> const applications;

    MirTrustSessionState state;

    std::mutex mutable mutex;
    std::vector<std::weak_ptr<shell::Session>> trusted_children;
    void clear_trusted_children();
};

}
}

#endif // MIR_SCENE_TRUST_SESSION_H_
