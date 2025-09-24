/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_FRONTEND_EXT_IMAGE_CAPTURE_V1_H
#define MIR_FRONTEND_EXT_IMAGE_CAPTURE_V1_H

#include "ext-image-capture-source-v1_wrapper.h"
#include "ext-image-copy-capture-v1_wrapper.h"

#include <memory>

namespace mir
{
class Executor;
namespace frontend
{
class OutputManager;
class SurfaceStack;

auto create_ext_output_image_capture_source_manager_v1(
    wl_display* display)
-> std::shared_ptr<wayland::OutputImageCaptureSourceManagerV1::Global>;

auto create_ext_image_copy_capture_manager_v1(
    wl_display* display,
    std::shared_ptr<Executor> const& wayland_executor)
-> std::shared_ptr<wayland::ImageCopyCaptureManagerV1::Global>;

}
}

#endif // MIR_FRONTEND_EXT_IMAGE_CAPTURE_V1_H
