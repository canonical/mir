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

#include "mir/shell/trust_session_creation_parameters.h"

namespace msh = mir::shell;

msh::TrustSessionCreationParameters::TrustSessionCreationParameters()
    : base_process_id(0)
{
}

msh::TrustSessionCreationParameters& msh::TrustSessionCreationParameters::set_base_process_id(pid_t process_id)
{
    base_process_id = process_id;
    return *this;
}

bool msh::operator==(
    const TrustSessionCreationParameters& lhs,
    const msh::TrustSessionCreationParameters& rhs)
{
    return lhs.base_process_id == rhs.base_process_id;
}

bool msh::operator!=(
    const TrustSessionCreationParameters& lhs,
    const msh::TrustSessionCreationParameters& rhs)
{
    return !(lhs == rhs);
}

msh::TrustSessionCreationParameters msh::a_trust_session()
{
    return TrustSessionCreationParameters();
}
