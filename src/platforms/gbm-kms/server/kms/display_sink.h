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

#ifndef MIR_GRAPHICS_GBM_DISPLAY_SINK_H_
#define MIR_GRAPHICS_GBM_DISPLAY_SINK_H_

#include "mir/graphics/display_sink.h"
#include "mir/graphics/display.h"
#include "display_helpers.h"
#include "egl_helper.h"
#include "mir/graphics/platform.h"
#include "platform_common.h"
#include "kms_framebuffer.h"

#include <vector>
#include <memory>
#include <atomic>

namespace mir
{
namespace graphics
{

class DisplayReport;
class GLConfig;

namespace gbm
{

class Platform;
class KMSOutput;

class DisplaySink : public graphics::DisplaySink,
                      public graphics::DisplaySyncGroup
{
public:
    DisplaySink(
        mir::Fd drm_fd,
        std::shared_ptr<struct gbm_device> gbm,
        BypassOption bypass_options,
        std::shared_ptr<DisplayReport> const& listener,
        std::vector<std::shared_ptr<KMSOutput>> const& outputs,
        geometry::Rectangle const& area,
        glm::mat2 const& transformation);
    ~DisplaySink();

    geometry::Rectangle view_area() const override;

    void set_next_image(std::unique_ptr<Framebuffer> content) override;

    bool overlay(std::vector<DisplayElement> const& renderlist) override;

    void for_each_display_sink(
        std::function<void(graphics::DisplaySink&)> const& f) override;
    void post() override;
    std::chrono::milliseconds recommended_sleep() const override;

    glm::mat2 transformation() const override;

    void set_transformation(glm::mat2 const& t, geometry::Rectangle const& a);
    void schedule_set_crtc();
    void wait_for_page_flip();

    auto drm_fd() const -> mir::Fd;

    auto gbm_device() const -> std::shared_ptr<struct gbm_device>;

protected:
    auto maybe_create_allocator(DisplayAllocator::Tag const& type_tag) -> DisplayAllocator* override;

private:
    bool schedule_page_flip(FBHandle const& bufobj);
    void set_crtc(FBHandle const&);

    std::shared_ptr<struct gbm_device> const gbm;
    bool holding_client_buffers{false};
    std::shared_ptr<FBHandle const> bypass_bufobj{nullptr};
    std::shared_ptr<DisplayReport> const listener;

    std::vector<std::shared_ptr<KMSOutput>> outputs;

    std::shared_ptr<CPUAddressableDisplayAllocator> kms_allocator;
    std::unique_ptr<GBMDisplayAllocator> gbm_allocator;

    // Framebuffer handling
    // KMS does not take a reference to submitted framebuffers; if you destroy a framebuffer while
    // it's in use, KMS treat that as submitting a null framebuffer and turn off the display.
    std::shared_ptr<FBHandle const> next_swap{nullptr};    //< Next frame to submit to the hardware
    std::shared_ptr<FBHandle const> scheduled_fb{nullptr}; //< Frame currently submitted to the hardware, not yet on-screen
    std::shared_ptr<FBHandle const> visible_fb{nullptr};   //< Frame currently onscreen

    geometry::Rectangle area;
    glm::mat2 transform;
    std::atomic<bool> needs_set_crtc;
    std::chrono::milliseconds recommend_sleep{0};
    bool page_flips_pending;
};

}
}
}

#endif /* MIR_GRAPHICS_GBM_DISPLAY_SINK_H_ */
