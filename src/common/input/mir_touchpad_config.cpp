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

#include "mir/input/mir_touchpad_config.h"
#include <ostream>

struct MirTouchpadConfig::Implementation
{
    Implementation() = default;
    Implementation(MirTouchpadClickModes click_mode,
                   MirTouchpadScrollModes scroll_mode,
                   int button_down_scroll_button,
                   bool tap_to_click,
                   bool middle_mouse_button_emulation,
                   bool disable_with_mouse,
                   bool disable_while_typing)
        : click_mode{click_mode},
          scroll_mode{scroll_mode},
          button_down_scroll_button{button_down_scroll_button},
          tap_to_click{tap_to_click},
          middle_mouse_button_emulation{middle_mouse_button_emulation},
          disable_with_mouse{disable_with_mouse},
          disable_while_typing{disable_while_typing}
    {
    }
    MirTouchpadClickModes click_mode{mir_touchpad_click_mode_finger_count};
    MirTouchpadScrollModes scroll_mode{mir_touchpad_scroll_mode_two_finger_scroll};
    int button_down_scroll_button{0};
    bool tap_to_click{true};
    bool middle_mouse_button_emulation{true};
    bool disable_with_mouse{false};
    bool disable_while_typing{false};
};

MirTouchpadConfig::MirTouchpadConfig()
    : impl{std::make_unique<Implementation>()}
{
}

MirTouchpadConfig::MirTouchpadConfig(MirTouchpadConfig && other)
    : impl(std::move(other.impl))
{
}

MirTouchpadConfig::MirTouchpadConfig(MirTouchpadConfig const& other)
    : impl(std::make_unique<Implementation>(*other.impl))
{
}

MirTouchpadConfig& MirTouchpadConfig::operator=(MirTouchpadConfig const& other)
{
    *impl = *other.impl;
    return *this;
}

MirTouchpadConfig::~MirTouchpadConfig() = default;

MirTouchpadConfig::MirTouchpadConfig(MirTouchpadClickModes click_mode,
                                                 MirTouchpadScrollModes scroll_mode,
                                                 int button_down_scroll_button,
                                                 bool tap_to_click,
                                                 bool disable_while_typing,
                                                 bool disable_with_mouse,
                                                 bool middle_mouse_button_emulation)
    : impl{std::make_unique<Implementation>(
            click_mode,
            scroll_mode,
            button_down_scroll_button,
            tap_to_click,
            middle_mouse_button_emulation,
            disable_with_mouse,
            disable_while_typing
            )}
{
}

MirTouchpadClickModes MirTouchpadConfig::click_mode() const
{
    return impl->click_mode;
}

void MirTouchpadConfig::click_mode(MirTouchpadClickModes modes)
{
    impl->click_mode = modes;
}

MirTouchpadScrollModes MirTouchpadConfig::scroll_mode() const
{
    return impl->scroll_mode;
}

void MirTouchpadConfig::scroll_mode(MirTouchpadScrollModes modes)
{
    impl->scroll_mode = modes;
}

int MirTouchpadConfig::button_down_scroll_button() const
{
    return impl->button_down_scroll_button;
}

void MirTouchpadConfig::button_down_scroll_button(int button)
{
    impl->button_down_scroll_button = button;
}

bool MirTouchpadConfig::tap_to_click() const
{
    return impl->tap_to_click;
}

void MirTouchpadConfig::tap_to_click(bool enabled)
{
    impl->tap_to_click = enabled;
}

bool MirTouchpadConfig::middle_mouse_button_emulation() const
{
    return impl->middle_mouse_button_emulation;
}

void MirTouchpadConfig::middle_mouse_button_emulation(bool enabled)
{
    impl->middle_mouse_button_emulation = enabled;
}

bool MirTouchpadConfig::disable_with_mouse() const
{
    return impl->disable_with_mouse;
}

void MirTouchpadConfig::disable_with_mouse(bool enabled)
{
    impl->disable_with_mouse = enabled;
}

bool MirTouchpadConfig::disable_while_typing() const
{
    return impl->disable_while_typing;
}

void MirTouchpadConfig::disable_while_typing(bool enabled)
{
    impl->disable_while_typing = enabled;
}

bool MirTouchpadConfig::operator==(MirTouchpadConfig const& rhs) const
{
    return
        impl->click_mode == rhs.impl->click_mode &&
        impl->scroll_mode == rhs.impl->scroll_mode &&
        impl->button_down_scroll_button == rhs.impl->button_down_scroll_button &&
        impl->tap_to_click == rhs.impl->tap_to_click &&
        impl->middle_mouse_button_emulation == rhs.impl->middle_mouse_button_emulation &&
        impl->disable_with_mouse == rhs.impl->disable_with_mouse &&
        impl->disable_while_typing == rhs.impl->disable_while_typing;
}

bool MirTouchpadConfig::operator!=(MirTouchpadConfig const& rhs) const
{
    return !(*this == rhs);
}

std::ostream& operator<<(std::ostream& out, MirTouchpadConfig const& conf)
{
    return out << "click_mode:" << conf.click_mode()
        << " scroll_mode:" << conf.scroll_mode()
        << " btn_down:" << conf.button_down_scroll_button()
        << " tap_to_click:" << conf.tap_to_click()
        << " middle_btn_emulation:" << conf.middle_mouse_button_emulation()
        << " disable_with_mouse:" << conf.disable_with_mouse()
        << " disable_while_typing:" << conf.disable_while_typing();
}
