/*
 * Copyright © 2017 Canonical Ltd.
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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_SURFACE_OBSERVER_H_
#define MIR_TEST_DOUBLES_MOCK_SURFACE_OBSERVER_H_

#include "mir/scene/surface_observer.h"
namespace mir
{
namespace test
{
namespace doubles
{

class MockSurfaceObserver : public scene::SurfaceObserver
{
public:
    MOCK_METHOD2(attrib_changed, void(MirWindowAttrib attrib, int value));
    MOCK_METHOD1(resized_to, void(geometry::Size const& size));
    MOCK_METHOD1(moved_to, void(geometry::Point const& top_left));
    MOCK_METHOD1(hidden_set_to, void(bool hide));
    MOCK_METHOD2(frame_posted, void(int frames_available, geometry::Size const& size));
    MOCK_METHOD1(alpha_set_to, void(float alpha));
    MOCK_METHOD1(orientation_set_to, void(MirOrientation orientation));
    MOCK_METHOD1(transformation_set_to, void(glm::mat4 const& t));
    MOCK_METHOD1(reception_mode_set_to, void(input::InputReceptionMode mode));
    MOCK_METHOD1(cursor_image_set_to, void(graphics::CursorImage const& image));
    MOCK_METHOD0(client_surface_close_requested, void());
    MOCK_METHOD5(keymap_changed, void(
        MirInputDeviceId id,
        std::string const& model,
        std::string const& layout,
        std::string const& variant,
        std::string const& options));
    MOCK_METHOD1(renamed, void(char const* name));
    MOCK_METHOD0(cursor_image_removed, void());
    MOCK_METHOD1(placed_relative, void(geometry::Rectangle const& placement));
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_SURFACE_OBSERVER_H_ */
