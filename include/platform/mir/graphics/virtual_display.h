/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#ifndef MIR_GRAPHICS_VIRTUAL_DISPLAY_H_
#define MIR_GRAPHICS_VIRTUAL_DISPLAY_H_

namespace mir
{

namespace graphics
{

/**
 * Interface to a virtual display
 */
class VirtualDisplay
{
public:

    /** Enables the virtual display.
     *
     *  This will initiate the same response as when a physical display is
     *  hotplugged into a system. A configuration change notification will be
     *  generated and the DisplayConfiguration will contain one new output.
    **/
    virtual void enable() = 0;

    /** Disable the virtual display.
     *
     *  This will initiate the same response as when a physical display is
     *  unplugged from a system. A configuration change notification will be
     *  generated and the DisplayConfiguration will no longer contain an output
     *  associated with the virtual display.
    **/
    virtual void disable() = 0;

    VirtualDisplay() = default;
    virtual ~VirtualDisplay() = default;
private:
    VirtualDisplay(VirtualDisplay const&) = delete;
    VirtualDisplay& operator=(VirtualDisplay const&) = delete;
};
}
}

#endif /* MIR_GRAPHICS_VIRTUAL_DISPLAY_H_ */
