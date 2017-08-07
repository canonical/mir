/*
 * Copyright © 2015 Canonical Ltd.
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

#ifndef MIR_INPUT_POINTER_CONFIGURATION_H_
#define MIR_INPUT_POINTER_CONFIGURATION_H_

#include "mir_toolkit/common.h"
#include "mir_toolkit/mir_input_device_types.h"

#include <memory>
#include <iosfwd>

struct MirPointerConfig
{
    MirPointerConfig();
    ~MirPointerConfig();
    MirPointerConfig(MirPointerConfig const& cp);
    MirPointerConfig(MirPointerConfig && cp);
    MirPointerConfig& operator=(MirPointerConfig const& cp);
    MirPointerConfig(MirPointerHandedness handedness, MirPointerAcceleration acceleration, double acceleration_bias,
                         double horizontal_scroll_scale, double vertical_scroll_scale);
    /*!
     * Configure which button shall be used as primary button. That way the input device is configured to be either
     * right or left handed.
     */
    MirPointerHandedness handedness() const;
    void handedness(MirPointerHandedness);

    /*!
     * Configure cursor acceleration profile
     */
    MirPointerAcceleration acceleration() const;
    void acceleration(MirPointerAcceleration);

    /*!
     * Configures the intensity of the cursor acceleration. Values within the range of [-1, 1] are allowed.
     *   - 0: default acceleration
     *   - [-1, 0): reduced acceleration
     *   - (0, 1]: increased acceleration
     */
    double cursor_acceleration_bias() const;
    void cursor_acceleration_bias(double);

    /*!
     * Configures a signed scale of the horizontal scrolling. Use negative values to configure 'natural scrolling'
     */
    double horizontal_scroll_scale() const;
    void horizontal_scroll_scale(double);

    /*!
     * Configures a signed scale of the vertical scrolling. Use negative values to configure 'natural scrolling'
     */
    double vertical_scroll_scale() const;
    void vertical_scroll_scale(double);

    bool operator==(MirPointerConfig const& rhs) const;
    bool operator!=(MirPointerConfig const& rhs) const;
private:
    struct Implementation;
    std::unique_ptr<Implementation> impl;
};

std::ostream& operator<<(std::ostream& out, MirPointerConfig const& rhs);


#endif
