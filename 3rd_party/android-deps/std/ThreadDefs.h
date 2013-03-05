/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_ANDROID_UBUNTU_THREADDEFS_H_
#define MIR_ANDROID_UBUNTU_THREADDEFS_H_

namespace mir_input
{
enum
{
    PRIORITY_DEFAULT            = 0,
    PRIORITY_URGENT_DISPLAY     = -8
};
}

namespace android
{
using ::mir_input::PRIORITY_DEFAULT;
using ::mir_input::PRIORITY_URGENT_DISPLAY;
}

#endif /* MIR_ANDROID_UBUNTU_THREADDEFS_H_ */
