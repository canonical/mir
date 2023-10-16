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

#ifndef MIR_GRAPHICS_GBM_DISPLAY_H_
#define MIR_GRAPHICS_GBM_DISPLAY_H_

#include "mir/graphics/display.h"
#include "mir/renderer/gl/context_source.h"
#include "real_kms_output_container.h"
#include "real_kms_display_configuration.h"
#include "display_helpers.h"
#include "egl_helper.h"
#include "platform_common.h"
#include "mir/graphics/platform.h"

#include <atomic>
#include <mutex>
#include <vector>

namespace mir
{
namespace graphics
{

class DisplayTarget;
class DisplayReport;
class DisplayBuffer;
class DisplayConfigurationPolicy;
class EventHandlerRegister;
class GLConfig;

namespace gbm
{

namespace helpers
{
class DRMHelper;
class GBMHelper;
}

class DisplayBuffer;
class KMSOutput;
class Cursor;

class Display : public graphics::Display
{
public:
    Display(
        std::shared_ptr<DisplayTarget> target,
        mir::Fd drm_fd,
        BypassOption bypass_option,
        std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
        std::shared_ptr<DisplayReport> const& listener);
    ~Display();

    geometry::Rectangle view_area() const;
    void for_each_display_sync_group(
        std::function<void(graphics::DisplaySyncGroup&)> const& f) override;

    std::unique_ptr<DisplayConfiguration> configuration() const override;
    bool apply_if_configuration_preserves_display_buffers(DisplayConfiguration const& conf) override;
    void configure(DisplayConfiguration const& conf) override;

    void register_configuration_change_handler(
        EventHandlerRegister& handlers,
        DisplayConfigurationChangeHandler const& conf_change_handler) override;

    void pause() override;
    void resume() override;

    std::shared_ptr<graphics::Cursor> create_hardware_cursor() override;

private:
    void clear_connected_unused_outputs();

    std::shared_ptr<DisplayTarget> const target;
    mutable std::mutex configuration_mutex;
    mir::Fd const drm_fd;
    std::shared_ptr<DisplayReport> const listener;
    mir::udev::Monitor monitor;
    std::shared_ptr<KMSOutputContainer> const output_container;
    std::vector<std::unique_ptr<DisplayBuffer>> display_buffers;
    mutable RealKMSDisplayConfiguration current_display_configuration;
    mutable std::atomic<bool> dirty_configuration;

    void configure_locked(
        RealKMSDisplayConfiguration const& conf,
        std::lock_guard<decltype(configuration_mutex)> const&);

    BypassOption bypass_option;
    std::weak_ptr<Cursor> cursor;
};

class CPUAddressableDisplayProvider : public graphics::CPUAddressableDisplayProvider
{
public:
    explicit CPUAddressableDisplayProvider(mir::Fd drm_fd);

    auto supported_formats() const
        -> std::vector<DRMFormat> override;

    auto alloc_fb(geometry::Size pixel_size, DRMFormat format)
        -> std::unique_ptr<MappableFB> override;

private:
    mir::Fd const drm_fd;
    bool const supports_modifiers;
};

class GBMDisplayProvider : public graphics::GBMDisplayProvider
{
public:
    GBMDisplayProvider(mir::Fd drm_fd);
    
    auto is_same_device(mir::udev::Device const& render_device) const -> bool override;
    
    auto gbm_device() const -> std::shared_ptr<struct gbm_device> override;

    auto supported_formats() const -> std::vector<DRMFormat> override;
    
    auto modifiers_for_format(DRMFormat format) const -> std::vector<uint64_t> override;
    
    auto make_surface(geometry::Size size, DRMFormat format, std::span<uint64_t> modifier) -> std::unique_ptr<GBMSurface> override;    
private:
    mir::Fd const fd;
    std::shared_ptr<struct gbm_device> const gbm;
};
}
}
}

#endif /* MIR_GRAPHICS_GBM_DISPLAY_H_ */
