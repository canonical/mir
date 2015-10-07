/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by:
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_POINTER_CONFIGURATION_H_
#define MIR_INPUT_POINTER_CONFIGURATION_H_

#include "mir_toolkit/common.h"
#include "mir_toolkit/client_types.h"

namespace mir
{
namespace input
{

struct PointerConfiguration
{
    PointerConfiguration();
    PointerConfiguration(MirPointerButton primary, double cursor_speed, double horizontal_scroll_speed, double vertical_scroll_speed);
    ~PointerConfiguration();

    /*!
     * Configure which button shall be used as primary button. That way the input device is configured to be either
     * right or left handed.
     */
    void primary_button(MirPointerButton button);
    MirPointerButton primary_button() const;

    /*!
     * Configures the intensity of the cursor acceleration. Values within the range of [-1, 1] are allowed.
     *   - 0: default acceleration
     *   - [-1, 0[: reduced acceleration
     *   - ]0, 1]: increased acceleration
     */
    void cursor_speed(double speed);
    double cursor_speed() const;

    /*!
     * Configures a signed scale of the horizontal scrolling. Use negative values to configure 'natural scrolling'
     */
    void horizontal_scroll_speed(double horizontal);
    double horizontal_scroll_speed() const;

    /*!
     * Configures a signed scale of the vertical scrolling. Use negative values to configure 'natural scrolling'
     */
    void vertical_scroll_speed(double vertical);
    double vertical_scroll_speed() const;
private:
    MirPointerButton primary_button_;
    double cursor_speed_;
    double horizontal_scroll_speed_;
    double vertical_scroll_speed_;
};

}
}

#endif
