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

#ifndef MIR_GRAPHICS_NESTED_PLATFORM_IMAGE_FACTORY_H_
#define MIR_GRAPHICS_NESTED_PLATFORM_IMAGE_FACTORY_H_

#include "mir/graphics/egl_extensions.h"
#include "egl_image_factory.h"

namespace mir
{
namespace graphics
{
namespace nested
{

class AndroidImageFactory : public EglImageFactory
{
public:
    std::unique_ptr<EGLImageKHR, std::function<void(EGLImageKHR*)>> create_egl_image_from(
        NativeBuffer& buffer, EGLDisplay display, EGLint const* attrs) const override;
private:
    std::shared_ptr<EGLExtensions> egl_extensions = std::make_shared<EGLExtensions>();
};

}
}
}
#endif /* MIR_GRAPHICS_NESTED_PLATFORM_IMAGE_FACTORY_H_ */
