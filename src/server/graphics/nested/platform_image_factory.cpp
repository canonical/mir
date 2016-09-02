/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "platform_image_factory.h"

namespace mgn = mir::graphics::nested;

std::unique_ptr<EGLImageKHR> mgn::AndroidImageFactory::create_egl_image_from(
    mgn::NativeBuffer& buffer, EGLDisplay display, EGLint const* attrs) const
{
    (void)buffer;(void)display;(void)attrs;
    return nullptr;
}
