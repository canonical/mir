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
 * Authored by: Nick Dedekind <nick.dedekind@gmail.com>
 */

#ifndef MIR_TRUSTED_SESSION_CONTROL_H_
#define MIR_TRUSTED_SESSION_CONTROL_H_

#include "mir_toolkit/common.h"

#include <functional>
#include <mutex>
#include <map>

namespace mir
{
namespace client
{
class TrustedSessionControl
{
public:
    TrustedSessionControl();
    ~TrustedSessionControl();

    int add_trusted_session_event_handler(std::function<void(uint32_t, MirTrustedSessionState)> const&);
    void remove_trusted_session_event_handler(int id);

    void call_trusted_session_event_handler(int32_t id, uint32_t state);

private:
    int next_id();

    std::mutex mutable guard;
    std::map<int, std::function<void(uint32_t, MirTrustedSessionState)>> handle_trusted_session_events;
    int next_fn_id;
};
}
}

#endif /* MIR_TRUSTED_SESSION_CONTROL_H_ */
