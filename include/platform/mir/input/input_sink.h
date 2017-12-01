/*
 * Copyright © 2014-2015 Canonical Ltd.
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

#ifndef MIR_INPUT_INPUT_SINK_H_
#define MIR_INPUT_INPUT_SINK_H_

#include "mir_toolkit/event.h"
#include "mir/geometry/rectangle.h"
#include "mir/geometry/point.h"

#include <vector>
#include <array>

namespace mir
{
namespace input
{

struct OutputInfo
{
    using Matrix = std::array<float,6>; // 2x3 row major matrix
    OutputInfo() {}
    OutputInfo(bool active, geometry::Size size, Matrix const& transformation)
        : active{active}, output_size{size}, output_to_scene(transformation)
    {}

    bool active{false};
    geometry::Size output_size;
    Matrix output_to_scene{{1,0,0,
                            0,1,0}};

    inline void transform_to_scene(float& x, float& y) const
    {
        auto original_x = x;
        auto original_y = y;
        auto const& mat = output_to_scene;

        x = mat[0]*original_x + mat[1]*original_y + mat[2]*1;
        y = mat[3]*original_x + mat[4]*original_y + mat[5]*1;
    }
};

class InputSink
{
public:
    InputSink() = default;
    virtual ~InputSink() = default;
    virtual void handle_input(std::shared_ptr<MirEvent> const& event) = 0;
    /**!
     * Obtain the bounding rectangle of the destination area for this input sink
     */
    virtual mir::geometry::Rectangle bounding_rectangle() const = 0;

    /**!
     * Obtain the output information for a specific ouput.
     *
     * \param[in] output_id the id of the output
     */
    virtual OutputInfo output_info(uint32_t output_id) const = 0;

    /**
     * \name Device State interface of InputSink
     *
     * In scenarios in which the system is not capable of receiving all changes as they occur,
     * these method should be used to update the input device state as needed
     * \{
     */
    /**
     * Set all pressed scan codes.
     * \param scan_codes currently pressed
     */
    virtual void key_state(std::vector<uint32_t> const& scan_codes) = 0;
    /**
     * Set button state of a pointing device.
     * \param buttons mask of the buttons currently pressed
     */
    virtual void pointer_state(MirPointerButtons buttons) = 0;
    /**
     * \}
     */
private:
    InputSink(InputSink const&) = delete;
    InputSink& operator=(InputSink const&) = delete;
};
}
}

#endif
