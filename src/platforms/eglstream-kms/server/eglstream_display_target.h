/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_GRAPHICS_EGLSTREAM_DISPLAY_TARGET_H_
#define MIR_GRAPHICS_EGLSTREAM_DISPLAY_TARGET_H_

#include "mir/graphics/platform.h"

#include <optional>

namespace mir::graphics::eglstream
{
class EGLStreamDisplayTarget : public DisplayTarget
{
public:
    EGLStreamDisplayTarget(EGLDisplay dpy);
    EGLStreamDisplayTarget(EGLStreamDisplayTarget const& from, EGLStreamKHR with_stream);

protected:
    auto maybe_create_interface(DisplayProvider::Tag const& tag)
        -> std::shared_ptr<DisplayProvider> override;

private:
    EGLDisplay dpy;
    std::optional<EGLStreamKHR> stream;
};

;
}

#endif // MIR_GRAPHICS_EGLSTREAM_DISPLAY_TARGET_H_
