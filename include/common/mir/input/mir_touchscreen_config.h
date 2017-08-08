/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
 * Authored by:
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_TOUCH_SCREEN_CONFIGURATION_H_
#define MIR_INPUT_TOUCH_SCREEN_CONFIGURATION_H_

#include "mir_toolkit/common.h"
#include "mir_toolkit/mir_input_device_types.h"
#include <memory>
#include <iosfwd>

struct MirTouchscreenConfig
{
    MirTouchscreenConfig();
    ~MirTouchscreenConfig();
    MirTouchscreenConfig(MirTouchscreenConfig const&);
    MirTouchscreenConfig(MirTouchscreenConfig &&);
    MirTouchscreenConfig& operator=(MirTouchscreenConfig const&);
    MirTouchscreenConfig(uint32_t output_id, MirTouchscreenMappingMode mode);

    /**
     * Configures the output the device coordinates should be aligned to.
     *
     * This element is only relevant when mapping_mode is set to
     * mir_touchscreen_mapping_mode_to_output.
     */
    uint32_t output_id() const;
    void output_id(uint32_t);

    /**
     * Configure the type of coordinate mapping to be used for this input
     * device.
     */
    MirTouchscreenMappingMode mapping_mode() const;
    void mapping_mode(MirTouchscreenMappingMode);

    bool operator==(MirTouchscreenConfig const& other) const;
    bool operator!=(MirTouchscreenConfig const& other) const;
private:
    struct Implementation;
    std::unique_ptr<Implementation> impl;
};

std::ostream& operator<<(std::ostream& out, MirTouchscreenConfig const& conf);

#endif
