/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIRAL_APPLICATION_H
#define MIRAL_APPLICATION_H

#include <mir_toolkit/common.h>

#include <memory>
#include <string>

namespace mir
{
namespace scene { class Session; }
}

namespace miral
{
using Application = std::shared_ptr<mir::scene::Session>;

void apply_lifecycle_state_to(Application const& application, MirLifecycleState state);
void kill(Application const& application, int sig);
auto name_of(Application const& application) -> std::string;
auto pid_of(Application const& application) -> pid_t;
}

#endif //MIRAL_APPLICATION_H
