/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_INPUT_ANDROID_DUMMY_POINTER_CONTROLLER_H__
#define MIR_INPUT_ANDROID_DUMMY_POINTER_CONTROLLER_H__

#include <PointerController.h>
namespace droidinput = android;

namespace mir
{
namespace input
{
namespace android
{

class DummyPointerController : public droidinput::PointerControllerInterface
{
  public:

    // From PointerControllerInterface
    virtual bool getBounds(float* out_min_x, float* out_min_y, float* out_max_x, float* out_max_y) const
    {
        (void)out_min_x;
        (void)out_min_y;
        (void)out_max_x;
        (void)out_max_y;
        // The bounds could not be fetched
        return false;
    }
    virtual void move(float delta_x, float delta_y)
    {
        (void)delta_x;
        (void)delta_y;
    }
    virtual void setButtonState(int32_t button_state)
    {
        (void)button_state;
    }
    virtual int32_t getButtonState() const
    {
      return 0;
    }
    virtual void setPosition(float x, float y)
    {
        (void)x;
        (void)y;
    }
    virtual void getPosition(float* out_x, float* out_y) const
    {
        (void)out_x;
        (void)out_y;
    }
    virtual void fade(Transition transition)
    {
        (void)transition;
    }
    virtual void unfade(Transition transition)
    {
        (void)transition;
    }

    virtual void setPresentation(Presentation presentation)
    {
        (void)presentation;
    }
    virtual void setSpots(const droidinput::PointerCoords* spot_coords, uint32_t spot_count)
    {
        (void)spot_coords;
        (void)spot_count;
    }
    virtual void clearSpots()
    {
    }

    virtual void setDisplaySize(int32_t width, int32_t height)
    {
        (void)width;
        (void)height;
    }
    virtual void setDisplayOrientation(int32_t orientation)
    {
        (void)orientation;
    }
};

}
}
} // namespace mir

#endif // MIR_ANDROID_DUMMY_POINTER_CONTROLER_H__
