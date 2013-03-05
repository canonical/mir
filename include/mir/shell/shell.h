/*
 * Mir shell interface PUBLIC HEADER
 *
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_SHELL_SHELL_H_
#define MIR_SHELL_SHELL_H_

#include <mir/surface.h>

namespace mir
{

class Shell
{
public:
    virtual ~Shell() {}

    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool supports(MirSurfaceAttrib attrib) const = 0;
    virtual bool supports(MirSurfaceAttrib attrib, int value) const = 0;

    // TODO: Lots more will go here I guess.
};

} // namespace mir

#endif
