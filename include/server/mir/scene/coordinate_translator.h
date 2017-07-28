/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_SCENE_COORDINATE_TRANSLATOR_H_
#define MIR_SCENE_COORDINATE_TRANSLATOR_H_

#include "mir/geometry/point.h"
#include <memory>

namespace mir
{
namespace frontend
{
class Surface;
}

namespace scene
{

/**
 * Support for the debug "surface to screen" coordinate translation interface.
 * \note For shells which do surface transformations the default implementation
 *       will return incorrect results.
 */
class CoordinateTranslator
{
public:
    virtual ~CoordinateTranslator() = default;

    /**
     * \brief Translate a surface coordinate into the screen coordinate space
     * \param [in] surface  A frontend::Surface. This will need to be dynamic_cast into
     *                      the scene::Surface relevant for the shell.
     * \param [in] x, y     Coordinates to translate from the surface coordinate space
     * \return              The coordinates in the screen coordinate space.
     * \throws              A std::runtime_error if the translation cannot be performed
     *                      for any reason.
     *
     * \note It is acceptable for this call to unconditionally throw a std::runtime_error.
     *       It is not required for normal functioning of the server or clients; clients which
     *       use the debug extension will receive an appropriate failure notice.
     */
    virtual geometry::Point surface_to_screen(std::shared_ptr<frontend::Surface> surface,
                                              int32_t x, int32_t y) = 0;
    virtual bool translation_supported() const = 0;
};


}
}

#endif // MIR_SCENE_COORDINATE_TRANSLATOR_H_
