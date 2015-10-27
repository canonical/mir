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

#ifndef MIR_INPUT_TOUCH_PAD_CONFIGURATION_H_
#define MIR_INPUT_TOUCH_PAD_CONFIGURATION_H_

#include "mir_toolkit/common.h"
#include "mir_toolkit/mir_input_device.h"

namespace mir
{
namespace input
{

struct TouchpadConfiguration
{
    TouchpadConfiguration(MirTouchpadClickModes click_mode,
                          MirTouchpadScrollModes scroll_mode,
                          int button_down_scroll_button,
                          bool tap_to_click,
                          bool disable_while_typing,
                          bool disable_with_mouse,
                          bool middle_mouse_button_emulation);
    TouchpadConfiguration();
    ~TouchpadConfiguration();

    /*!
     * When tap to click is enabled the system will interpret short finger touch down/up sequences as button clicks.
     * \{
     */
    void tap_to_click(bool enabled);
    bool tap_to_click() const;
    /// \}
    /*!
     * Emulates a middle mouse button press when the left and right buttons on a touchpad are pressed.
     * \{
     */
    void middle_mouse_button_emulation(bool enabled);
    bool middle_mouse_button_emulation() const;
    /// \}
    /*!
     * When disable-with-mouse is enabled the touchpad will stop to emit user input events when another pointing device is plugged in.
     * \{
     */
    void disable_with_mouse(bool enabled);
    bool disable_with_mouse() const;
    /// \}
    /*!
     * When disable-with-mouse is enabled the touchpad will stop to emit user input events when the user starts to use a keyboard and a short period after.
     * \{
     */
    void disable_while_typing(bool enabled);
    bool disable_while_typing() const;
    /// \}
    /*!
     * The click mode defines when the touchpad generates software emulated button events.
     * \{
     */
    void click_mode(MirTouchpadClickModes click_mode);
    MirTouchpadClickModes click_mode() const;
    /// \}

    /*!
     * The scroll mode defines when the touchpad generates scroll events instead of pointer motion events.
     * \{
     */
    void scroll_mode(MirTouchpadScrollModes scroll_mode);
    MirTouchpadScrollModes scroll_mode() const;
    /*!
     * Configures the button used for the on-button-down scroll mode
     */
    void scroll_button(int scroll_button);
    /*!
     * Returns button used to enable the on-button-down scroll mode
     */
    int scroll_button() const;
    /// \}
    private:
    MirTouchpadClickModes click_mode_;
    MirTouchpadScrollModes scroll_mode_;
    int button_down_scroll_button_;
    bool tap_to_click_;
    bool disable_while_typing_;
    bool disable_with_mouse_;
    bool middle_mouse_button_emulation_;
};

}
}

#endif
