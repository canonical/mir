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

#ifndef MIR_FRONTEND_TRUST_SESSION_H_
#define MIR_FRONTEND_TRUST_SESSION_H_

#include "mir_toolkit/common.h"

#include <sys/types.h>
#include <vector>

namespace mir
{

namespace frontend
{
class TrustSession
{
public:
    virtual ~TrustSession() = default;

    virtual std::vector<pid_t> get_applications() const = 0;

    virtual MirTrustSessionState get_state() const = 0;

    virtual void start() = 0;
    virtual void stop() = 0;

protected:
    TrustSession() = default;
    TrustSession(const TrustSession&) = delete;
    TrustSession& operator=(const TrustSession&) = delete;
};

}
}

#endif // MIR_FRONTEND_TRUST_SESSION_H_
