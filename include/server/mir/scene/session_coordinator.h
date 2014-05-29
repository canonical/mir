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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_SCENE_SESSION_COORDINATOR_H_
#define MIR_SCENE_SESSION_COORDINATOR_H_


#include "mir/frontend/shell.h"
#include "mir/shell/focus_controller.h"

namespace mir
{
namespace scene
{

class SessionCoordinator : public frontend::Shell, public shell::FocusController
{
};

}
}

#endif /* MIR_SCENE_SESSION_COORDINATOR_H_ */
