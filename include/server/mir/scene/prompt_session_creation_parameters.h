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

#ifndef MIR_SCENE_PROMPT_SESSION_CREATION_PARAMETERS_H_
#define MIR_SCENE_PROMPT_SESSION_CREATION_PARAMETERS_H_

#include <sys/types.h>

namespace mir
{
namespace scene
{

struct PromptSessionCreationParameters
{
    pid_t application_pid = 0;
};
}
}

#endif /* MIR_SCENE_PROMPT_SESSION_CREATION_PARAMETERS_H_ */
