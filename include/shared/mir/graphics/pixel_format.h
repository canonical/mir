/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_GRAPHICS_PIXEL_FORMAT_UTILS_H_
#define MIR_GRAPHICS_PIXEL_FORMAT_UTILS_H_

#include "mir_toolkit/common.h"

namespace mir
{
namespace graphics
{

/*!
 * \brief PixelFormat is a wrapper class to query details of MirPixelFormat
 */
class PixelFormat
{
public:
    /*!
     * \brief Not explicit on purpose to allow transparent wrapping of MirPixelFormat.
     */
    PixelFormat(MirPixelFormat format = mir_pixel_format_invalid);

    /// \brief returns true if there is an alpha channel
    bool contains_alpha() const;

    /// \brief returns the number of bits in red channel
    int red_channel_depth() const;

    /// \brief returns the number of bits in blue channel
    int blue_channel_depth() const;

    /// \brief returns the number of bits in green channel
    int green_channel_depth() const;

    /// \brief returns the number of bits in alpha channel
    int alpha_channel_depth() const;

    /// \brief true when PixelFormat wraps a valid MirPixelFormat enum value
    explicit operator bool() const;

    operator MirPixelFormat() const;
private:
    MirPixelFormat format;
};


}
}

#endif
