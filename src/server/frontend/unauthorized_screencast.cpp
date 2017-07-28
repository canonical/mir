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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "unauthorized_screencast.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mf = mir::frontend;

mf::ScreencastSessionId mf::UnauthorizedScreencast::create_session(
    mir::geometry::Rectangle const&,
    mir::geometry::Size const&,
    MirPixelFormat, int, MirMirrorMode)
{
    BOOST_THROW_EXCEPTION(
        std::runtime_error("Process is not authorized to capture screencasts"));
}

void mf::UnauthorizedScreencast::destroy_session(mf::ScreencastSessionId)
{
    BOOST_THROW_EXCEPTION(
        std::runtime_error("Process is not authorized to capture screencasts"));
}

std::shared_ptr<mir::graphics::Buffer> mf::UnauthorizedScreencast::capture(
    mf::ScreencastSessionId)
{
    BOOST_THROW_EXCEPTION(
        std::runtime_error("Process is not authorized to capture screencasts"));
}

void mf::UnauthorizedScreencast::capture(mf::ScreencastSessionId, std::shared_ptr<mir::graphics::Buffer> const&)
{
    BOOST_THROW_EXCEPTION(
        std::runtime_error("Process is not authorized to capture screencasts"));
}
