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

#ifndef MIR_INPUT_TOUCH_PAD_CONFIGURATION_H_
#define MIR_INPUT_TOUCH_PAD_CONFIGURATION_H_

#include "mir_toolkit/common.h"
#include "mir_toolkit/mir_input_device_types.h"

#include <memory>
#include <iosfwd>

struct MirTouchpadConfig
{
    MirTouchpadConfig();
    MirTouchpadConfig(MirTouchpadConfig && other);
    MirTouchpadConfig(MirTouchpadConfig const& other);
    MirTouchpadConfig& operator=(MirTouchpadConfig const& other);
    ~MirTouchpadConfig();
    MirTouchpadConfig(MirTouchpadClickModes click_mode,
                          MirTouchpadScrollModes scroll_mode,
                          int button_down_scroll_button,
                          bool tap_to_click,
                          bool disable_while_typing,
                          bool disable_with_mouse,
                          bool middle_mouse_button_emulation);

    /*!
     * The click mode defines when the touchpad generates software emulated button events.
     */
    MirTouchpadClickModes click_mode() const;
    void click_mode(MirTouchpadClickModes) ;
    /*!
     * The scroll mode defines when the touchpad generates scroll events instead of pointer motion events.
     */
    MirTouchpadScrollModes scroll_mode() const;
    void scroll_mode(MirTouchpadScrollModes);

    /*!
     * Configures the button used for the on-button-down scroll mode
     */
    int button_down_scroll_button() const;
    void button_down_scroll_button(int);

    /*!
     * When tap to click is enabled the system will interpret short finger touch down/up sequences as button clicks.
     */
    bool tap_to_click() const;
    void tap_to_click(bool);

    /*!
     * Emulates a middle mouse button press when the left and right buttons on a touchpad are pressed.
     */
    bool middle_mouse_button_emulation() const;
    void middle_mouse_button_emulation(bool);

    /*!
     * When disable-with-mouse is enabled the touchpad will stop to emit user input events when another pointing device is plugged in.
     */
    bool disable_with_mouse() const;
    void disable_with_mouse(bool);

    /*!
     * When disable-with-mouse is enabled the touchpad will stop to emit user input events when the user starts to use a keyboard and a short period after.
     */
    bool disable_while_typing() const;
    void disable_while_typing(bool);

    bool operator==(MirTouchpadConfig const& rhs) const;
    bool operator!=(MirTouchpadConfig const& rhs) const;
private:
    struct Implementation;
    std::unique_ptr<Implementation> impl;
};

std::ostream& operator<<(std::ostream& out, MirTouchpadConfig const& conf);

#endif
