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
     */
    void tap_to_click(bool enabled);
    bool tap_to_click() const;
    /*!
     */
    void middle_mouse_button_emulation(bool enabled);
    bool middle_mouse_button_emulation() const;
    /*!
     */
    void disable_with_mouse(bool enabled);
    bool disable_with_mouse() const;
    /*!
     */
    void disable_while_typing(bool enabled);
    bool disable_while_typing() const;
    /*!
     */
    void click_mode(MirTouchpadClickModes click_mode);
    MirTouchpadClickModes click_mode() const;

    /*!
     */
    void scroll_mode(MirTouchpadScrollModes scroll_mode);
    MirTouchpadScrollModes scroll_mode() const;
    /*!
     * Configure scroll mode via button down.
     */
    void scroll_mode(int scroll_button);
    int scroll_button() const;
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
