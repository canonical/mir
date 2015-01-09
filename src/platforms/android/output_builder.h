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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_OUTPUT_BUILDER_H_
#define MIR_GRAPHICS_ANDROID_OUTPUT_BUILDER_H_

#include "display_buffer_builder.h"
#include "display_resource_factory.h"
#include "overlay_optimization.h"
#include "hardware/hwcomposer.h"
#include "hardware/fb.h"

namespace mir
{
namespace graphics
{
class DisplayReport;
namespace android
{
class FramebufferBundle;
class DisplayResourceFactory;
class GraphicBufferAllocator;
class DisplayDevice;
class HwcWrapper;
class HwcReport;

class OutputBuilder : public DisplayBufferBuilder
{
public:
    OutputBuilder(
        std::shared_ptr<GraphicBufferAllocator> const& buffer_allocator,
        std::shared_ptr<DisplayResourceFactory> const& res_factory,
        OverlayOptimization overlay_option,
        std::shared_ptr<HwcReport> const& hwc_report);

    MirPixelFormat display_format() override;
    std::unique_ptr<ConfigurableDisplayBuffer> create_display_buffer(
        GLProgramFactory const& gl_program_factory,
        GLContext const& gl_context) override;
    std::unique_ptr<HwcConfiguration> create_hwc_configuration() override;

private:
    std::shared_ptr<GraphicBufferAllocator> const buffer_allocator;
    std::shared_ptr<DisplayResourceFactory> const res_factory;
    std::shared_ptr<HwcReport> const hwc_report;

    std::shared_ptr<FramebufferBundle> framebuffers;
    bool force_backup_display;

    std::shared_ptr<HwcWrapper> hwc_wrapper;
    std::shared_ptr<framebuffer_device_t> fb_native;
    OverlayOptimization overlay_optimization;
    HwcVersion hwc_version;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_OUTPUT_BUILDER_H_ */
