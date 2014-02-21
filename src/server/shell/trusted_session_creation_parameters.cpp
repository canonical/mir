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

#include "mir/shell/trusted_session_creation_parameters.h"

namespace msh = mir::shell;

msh::TrustedSessionCreationParameters::TrustedSessionCreationParameters()
{
}

msh::TrustedSessionCreationParameters& msh::TrustedSessionCreationParameters::add_application(pid_t application_pid)
{
    applications.push_back(application_pid);
    return *this;
}

bool msh::operator==(
    const TrustedSessionCreationParameters& lhs,
    const msh::TrustedSessionCreationParameters& rhs)
{
    return lhs.applications == rhs.applications;
}

bool msh::operator!=(
    const TrustedSessionCreationParameters& lhs,
    const msh::TrustedSessionCreationParameters& rhs)
{
    return !(lhs == rhs);
}

msh::TrustedSessionCreationParameters msh::a_trusted_session()
{
    return TrustedSessionCreationParameters();
}
