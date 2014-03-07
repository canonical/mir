/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */
#ifndef MIR_CLIENT_TRUST_SESSION_H_
#define MIR_CLIENT_TRUST_SESSION_H_

#include "mir_toolkit/mir_client_library.h"

namespace mir
{
namespace client
{

/**
 * Interface to client-side platform specific support for graphics operations.
 * \ingroup platform_enablement
 */
class TrustSession
{
public:
    virtual MirWaitHandle* start(mir_trust_session_callback callback, void * context) = 0;
    virtual MirWaitHandle* stop(mir_trust_session_callback callback, void * context) = 0;

protected:
    TrustSession() {}
    virtual ~TrustSession() {}

    TrustSession(const TrustSession&) = delete;
    TrustSession& operator=(const TrustSession&) = delete;
};

}
}

#endif /* MIR_CLIENT_TRUST_SESSION_H_ */
