/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "android_pointer_controller.h"

namespace mia = mir::input::android;


mia::PointerController::PointerController() : 
    state(0),
    x(0), 
    y(0)
{
}

bool mia::PointerController::getBounds(float* out_min_x, float* out_min_y, float* out_max_x, float* out_max_y) const
{
    *out_min_x = 0;
    *out_min_y = 0;
    *out_max_x = 2048;
    *out_max_y = 2048;
}
void mia::PointerController::move(float delta_x, float delta_y)
{
    std::lock_guard<std::mutex> lg(guard);
    x += delta_x;
    y += delta_y;
}
void mia::PointerController::setButtonState(int32_t button_state)
{
    std::lock_guard<std::mutex> lg(guard);
    state = button_state;
}
int32_t mia::PointerController::getButtonState()
{
    std::lock_guard<std::mutex> lg(guard);
    return state;
}
void mia::PointerController::setPosition(float new_x, float new_y)
{
    std::lock_guard<std::mutex> lg(guard);
    x = new_x;
    y = new_y;
}
void mia::PointerController::getPosition(float *out_x, float *out_y)
{
    std::lock_guard<std::mutex> lg(guard);
    *out_x = x;
    *out_y = y;
}
