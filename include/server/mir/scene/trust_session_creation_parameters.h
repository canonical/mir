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

#ifndef MIR_SCENE_TRUSTED_SESSION_CREATION_PARAMETERS_H_
#define MIR_SCENE_TRUSTED_SESSION_CREATION_PARAMETERS_H_

#include <sys/types.h>
#include <vector>

namespace mir
{
namespace scene
{

struct TrustSessionCreationParameters
{
    TrustSessionCreationParameters();

    TrustSessionCreationParameters& set_base_process_id(pid_t process_id);

    pid_t base_process_id;
};

bool operator==(const TrustSessionCreationParameters& lhs, const TrustSessionCreationParameters& rhs);
bool operator!=(const TrustSessionCreationParameters& lhs, const TrustSessionCreationParameters& rhs);

TrustSessionCreationParameters a_trust_session();
}
}

#endif /* MIR_SCENE_TRUSTED_SESSION_CREATION_PARAMETERS_H_ */
