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

#include "quirks.h"
#include <mir/graphics/quirk_common.h>

#include <mir/log.h>
#include <mir/options/option.h>
#include <mir/udev/wrapper.h>

#include <boost/algorithm/string.hpp>

#include <gbm.h>

#include <algorithm>
#include <cctype>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace mgg = mir::graphics::gbm;
namespace mo = mir::options;
namespace mgc = mir::graphics::common;

namespace
{
char const* quirks_option_name = "driver-quirks";

auto contains_case_insensitive(std::string_view haystack, std::string_view needle) -> bool
{
    auto it = std::search(
        haystack.begin(),
        haystack.end(),
        needle.begin(),
        needle.end(),
        [](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); });
    return it != haystack.end();
}

auto is_nvidia(std::string_view vendor) -> bool
{
    return contains_case_insensitive(vendor, "nvidia");
}

auto is_intel(std::string_view vendor) -> bool
{
    return contains_case_insensitive(vendor, "intel") || contains_case_insensitive(vendor, "i915");
}

struct VendorPairTransferStrategy
{
    void add(std::string_view source_vendor, std::string_view dest_vendor, std::string_view strategy);
    auto get_strategy(std::string_view source_vendor, std::string_view dest_vendor) const
        -> std::optional<mgg::GbmQuirks::BufferTransferStrategy>;

    // Map from "source:dest" to strategy
    std::unordered_map<std::string, mgg::GbmQuirks::BufferTransferStrategy> vendor_pairs;
};

void VendorPairTransferStrategy::add(
    std::string_view source_vendor, std::string_view dest_vendor, std::string_view strategy)
{
    auto key = std::string{source_vendor} + ":" + std::string{dest_vendor};

    if (strategy == "cpu")
    {
        vendor_pairs[key] = mgg::GbmQuirks::BufferTransferStrategy::cpu;
        mir::log_info(
            "Quirks: Configured %s -> %s to use CPU transfer strategy",
            std::string{source_vendor}.c_str(),
            std::string{dest_vendor}.c_str());
    }
    else if (strategy == "dma")
    {
        vendor_pairs[key] = mgg::GbmQuirks::BufferTransferStrategy::dma;
        mir::log_info(
            "Quirks: Configured %s -> %s to use DMA transfer strategy",
            std::string{source_vendor}.c_str(),
            std::string{dest_vendor}.c_str());
    }
    else
    {
        mir::log_warning(
            "Quirks: Unknown strategy '%s' for %s -> %s, ignoring",
            std::string{strategy}.c_str(),
            std::string{source_vendor}.c_str(),
            std::string{dest_vendor}.c_str());
    }
}

auto VendorPairTransferStrategy::get_strategy(std::string_view source_vendor, std::string_view dest_vendor) const
    -> std::optional<mgg::GbmQuirks::BufferTransferStrategy>
{
    auto key = std::string{source_vendor} + ":" + std::string{dest_vendor};
    auto it = vendor_pairs.find(key);
    if (it != vendor_pairs.end())
    {
        return it->second;
    }
    return std::nullopt;
}

auto parse_vendor_pair_quirk(
    std::vector<std::string> const& tokens, VendorPairTransferStrategy& vendor_pair_transfer_strategy) -> bool
{
    if (tokens.size() != 4)
    {
        mir::log_debug(
            "Invalid number of tokens (%zu) for gbm-buffer-transfer-strategy quirk (expected 4)", tokens.size());
        return false;
    }

    auto const& source_vendor = tokens[1];
    auto const& dest_vendor = tokens[2];
    auto const& strategy = tokens[3];

    if (strategy == "cpu" || strategy == "dma")
    {
        vendor_pair_transfer_strategy.add(source_vendor, dest_vendor, strategy);
        return true;
    }
    else
    {
        mir::log_warning(
            "Invalid strategy '%s' for gbm-buffer-transfer-strategy (expected 'cpu' or 'dma')", strategy.c_str());
        return false;
    }
}
}

class mgg::Quirks::Impl
{
public:
    explicit Impl(mo::Option const& options)
    {
        if (!options.is_set(quirks_option_name))
        {
            return;
        }

        auto const process_one_option = [&](auto option_value)
        {
            std::vector<std::string> tokens;
            boost::split(tokens, option_value, boost::is_any_of(":"));

            auto static const available_options = std::unordered_map<std::string, mgc::OptionStructure>{
                // option::{driver,devnode}:<specifier>[:value]
                {"skip", mgc::OptionStructure{}},
                {"allow", mgc::OptionStructure{}},
                {"disable-kms-probe", mgc::OptionStructure{}},
                {"egl-destroy-surface", mgc::OptionStructure{}},
                {"gbm-surface-has-free-buffers", mgc::OptionStructure{}},

                // option:source_vendor:dest_vendor:strategy
                {"gbm-buffer-transfer-strategy", mgc::OptionStructure::freeform(4)},
            };

            auto const structure = mgc::validate_structure(tokens, available_options);
            if (!structure)
                return false;

            auto const [option, specifier, specifier_value] = *structure;

            if (option == "skip")
            {
                completely_skip.skip(specifier, specifier_value);
                return true;
            }
            else if (option == "allow")
            {
                completely_skip.allow(specifier, specifier_value);
                return true;
            }
            else if (option == "disable-kms-probe")
            {
                disable_kms_probe.skip(specifier, specifier_value);
                return true;
            }
            else if (mgc::matches(tokens, "egl-destroy-surface", {"default", "leak"}))
            {
                egl_destroy_surface.add(specifier, specifier_value, tokens[3]);
                return true;
            }
            else if (mgc::matches(tokens, "gbm-surface-has-free-buffers", {"default", "skip"}))
            {
                gbm_surface_has_free_buffers.add(specifier, specifier_value, tokens[3]);
                return true;
            }
            else if (option == "gbm-buffer-transfer-strategy")
            {
                return parse_vendor_pair_quirk(tokens, vendor_pair_transfer_strategy);
            }

            return false;
        };

        for (auto const& quirk : options.get<std::vector<std::string>>(quirks_option_name))
        {
            if (!process_one_option(quirk))
            {
                // If we didn't process above, we're ignoring...
                // clangd really can't format this...
                mir::log_warning(
                    "Ignoring unexpected value for %s option: %s "
                    "(expects value of the form "
                    "\"{skip, allow}:{driver,devnode}:<driver or devnode>\", "
                    "\"disable-kms-probe:{driver,devnode}:<driver or devnode>\", "
                    "\"egl-destroy-surface:{driver,devnode}:{default:leak}\", "
                    "\"gbm-surface-has-free-buffers:{driver,devnode}:<driver or devnode>:{default,skip}\", "
                    "or \"gbm-buffer-transfer-strategy:<source_vendor>:<dest_vendor>:{cpu,dma}\")",
                    quirks_option_name,
                    quirk.c_str());
            }
        }
    }

    auto should_skip(udev::Device const& device) const -> bool
    {
        auto const devnode = mgc::value_or(device.devnode(), "");
        auto const parent_device = device.parent();
        auto const driver = mgc::get_device_driver(parent_device.get());

        mir::log_debug("Quirks(skip/allow): checking device with devnode: %s, driver %s", device.devnode(), driver);

        bool const should_skip_devnode = completely_skip.skipped_devnodes.contains(devnode);
        if (should_skip_devnode)
        {
            mir::log_info("Quirks(skip/allow): skipping device %s (matches devnode quirk %s)", devnode, devnode);
        }

        bool const should_skip_driver = completely_skip.skipped_drivers.contains(driver);
        if (should_skip_driver)
        {
            mir::log_info("Quirks(skip/allow): skipping device %s (matches driver quirk %s)", devnode, driver);
        }

        return should_skip_driver || should_skip_devnode;
    }

    auto require_modesetting_support(mir::udev::Device const& device) const -> bool
    {
        auto const devnode = mgc::value_or(device.devnode(), "");
        auto const parent_device = device.parent();
        auto const driver = mgc::get_device_driver(parent_device.get());
        mir::log_debug(
            "Quirks(disable-kms-probe): checking device with devnode: %s, driver %s", device.devnode(), driver);

        bool const should_skip_devnode = disable_kms_probe.skipped_devnodes.contains(devnode);
        if (should_skip_devnode)
        {
            mir::log_info("Quirks(disable-kms-probe): skipping device %s (matches devnode quirk %s)", devnode, devnode);
        }

        bool const should_skip_driver = disable_kms_probe.skipped_drivers.contains(driver);
        if (should_skip_driver)
        {
            mir::log_info("Quirks(disable-kms-probe): skipping device %s (matches driver quirk %s)", devnode, driver);
        }

        return !(should_skip_driver || should_skip_devnode);
    }

    auto gbm_quirks_for(udev::Device const& device) -> std::shared_ptr<GbmQuirks>
    {
        class DefaultEglDestroySurface : public GbmQuirks::EglDestroySurfaceQuirk
        {
            void egl_destroy_surface(EGLDisplay dpy, EGLSurface surf) const override
            {
                eglDestroySurface(dpy, surf);
            }
        };

        class LeakSurface : public GbmQuirks::EglDestroySurfaceQuirk
        {
            void egl_destroy_surface(EGLDisplay, EGLSurface) const override
            {
                // https://github.com/canonical/mir/pull/3979#issuecomment-2970797208
                mir::log_warning("egl_destroy_surface called on the leak implementation. Will not destroy surface.");
            }
        };

        auto const driver = mgc::get_device_driver(device.parent().get());
        auto const devnode = device.devnode();
        mir::log_debug("Quirks(egl-destroy-surface): checking device with devnode: %s, driver %s", devnode, driver);

        auto egl_destroy_surface_impl_name = mgc::apply_quirk(
            devnode, driver, egl_destroy_surface.devnodes, egl_destroy_surface.drivers, "egl-destroy-surface");

        auto egl_destroy_surface_impl = [&]() -> std::unique_ptr<GbmQuirks::EglDestroySurfaceQuirk>
        {
            if (egl_destroy_surface_leaking_options.contains(egl_destroy_surface_impl_name))
                return std::make_unique<LeakSurface>();

            return std::make_unique<DefaultEglDestroySurface>();
        }();

        class GbmSurfaceHasFreeBuffersAlwaysTrue : public GbmQuirks::SurfaceHasFreeBuffersQuirk
        {
            auto gbm_surface_has_free_buffers(gbm_surface*) const -> int override
            {
                return 1;
            }
        };

        class DefaultGbmSurfaceHasFreeBuffers : public GbmQuirks::SurfaceHasFreeBuffersQuirk
        {
            auto gbm_surface_has_free_buffers(gbm_surface* gbm_surface) const -> int override
            {
                return ::gbm_surface_has_free_buffers(gbm_surface);
            }
        };

        mir::log_debug(
            "Quirks(gbm-surface-has-free-buffers): checking device with devnode: %s, driver %s", devnode, driver);

        auto surface_has_free_buffers_impl_name = mgc::apply_quirk(
            devnode,
            driver,
            gbm_surface_has_free_buffers.devnodes,
            gbm_surface_has_free_buffers.drivers,
            "gbm-surface-has-free-buffers");

        auto surface_has_free_buffers_impl = [&]() -> std::unique_ptr<GbmQuirks::SurfaceHasFreeBuffersQuirk>
        {
            if (gbm_surface_has_free_buffers_always_true_options.contains(surface_has_free_buffers_impl_name))
                return std::make_unique<GbmSurfaceHasFreeBuffersAlwaysTrue>();
            return std::make_unique<DefaultGbmSurfaceHasFreeBuffers>();
        }();

        return std::make_shared<GbmQuirks>(
            std::move(egl_destroy_surface_impl),
            std::move(surface_has_free_buffers_impl),
            GbmQuirks::VendorPairConfig{vendor_pair_transfer_strategy.vendor_pairs});
    }

private:
    /* AST is a simple 2D output device, built into some motherboards.
     * They do not have any 3D engine associated, so were quirked off to avoid
     * https://github.com/canonical/mir/issues/2678
     *
     * At least as of drivers ≤ version 550, the NVIDIA gbm implementation is buggy in a way that prevents
     * Mir from working. Quirk off gbm-kms on NVIDIA.
     *
     * Simple-framebuffer should be evicted before Mir even gets there.
     * https://bugs.launchpad.net/ubuntu/+source/linux/+bug/2084046
     * https://github.com/canonical/mir/issues/3710
     */
    mgc::AllowList completely_skip{{"ast", "simple-framebuffer"}};
    // We know this is currently useful for virtio_gpu, vc4-drm and v3d
    mgc::AllowList disable_kms_probe{{"virtio_gpu", "vc4-drm", "v3d"}};

    inline static std::set<std::string_view> const egl_destroy_surface_leaking_options{"leak", "nvidia"};
    mgc::ValuedOption egl_destroy_surface;

    inline static std::set<std::string_view> const gbm_surface_has_free_buffers_always_true_options{"skip", "nvidia"};
    mgc::ValuedOption gbm_surface_has_free_buffers;

    VendorPairTransferStrategy vendor_pair_transfer_strategy;
};

mgg::Quirks::Quirks(options::Option const& options) :
    impl{std::make_unique<Impl>(options)}
{
}

mgg::Quirks::~Quirks() = default;

auto mgg::Quirks::should_skip(udev::Device const& device) const -> bool
{
    return impl->should_skip(device);
}

void mgg::Quirks::add_quirks_option(boost::program_options::options_description& config)
{
    config.add_options()(
        quirks_option_name,
        boost::program_options::value<std::vector<std::string>>(),
        "Driver quirks to apply. "
        "May be specified multiple times; multiple quirks are combined.");
}

auto mir::graphics::gbm::Quirks::require_modesetting_support(mir::udev::Device const& device) const -> bool
{
    if (getenv("MIR_MESA_KMS_DISABLE_MODESET_PROBE") != nullptr)
    {
        mir::log_debug("MIR_MESA_KMS_DISABLE_MODESET_PROBE is set");
        return false;
    }
    else if (getenv("MIR_GBM_KMS_DISABLE_MODESET_PROBE") != nullptr)
    {
        mir::log_debug("MIR_GBM_KMS_DISABLE_MODESET_PROBE is set");
        return false;
    }
    else
    {
        return impl->require_modesetting_support(device);
    }
}

auto mir::graphics::gbm::Quirks::gbm_quirks_for(udev::Device const& device) -> std::shared_ptr<GbmQuirks>
{
    return impl->gbm_quirks_for(device);
}

mir::graphics::gbm::GbmQuirks::GbmQuirks(
    std::unique_ptr<EglDestroySurfaceQuirk> egl_destroy_surface,
    std::unique_ptr<SurfaceHasFreeBuffersQuirk> surface_has_free_buffers,
    VendorPairConfig vendor_pair_config) :
    egl_destroy_surface_{std::move(egl_destroy_surface)},
    surface_has_free_buffers_{std::move(surface_has_free_buffers)},
    vendor_pair_config_{std::move(vendor_pair_config)}
{
}

void mir::graphics::gbm::GbmQuirks::egl_destroy_surface(EGLDisplay dpy, EGLSurface surf) const
{
    egl_destroy_surface_->egl_destroy_surface(dpy, surf);
}

auto mir::graphics::gbm::GbmQuirks::gbm_surface_has_free_buffers(gbm_surface* gbm_surface) const -> int
{
    return surface_has_free_buffers_->gbm_surface_has_free_buffers(gbm_surface);
}

auto mir::graphics::gbm::GbmQuirks::make_transfer_strategy_selector() const
    -> DMABufEGLProvider::TransferStrategySelector
{
    // Capture vendor pair config and default strategy
    auto config = vendor_pair_config_;

    return [config](std::string_view source_vendor, std::string_view dest_vendor) -> DMABufEGLProvider::BufferTransferStrategy
    {
        auto constexpr default_strategy = DMABufEGLProvider::BufferTransferStrategy::dma;

        // If either vendor is unknown, use default
        if (source_vendor.empty() || dest_vendor.empty())
        {
            return default_strategy;
        }

        std::string_view source_view{source_vendor};
        std::string_view dest_view{dest_vendor};

        // Check if user configured this specific vendor pair
        auto key = std::string{source_view} + ":" + std::string{dest_view};
        auto it = config.vendor_pairs.find(key);
        if (it != config.vendor_pairs.end())
        {
            mir::log_debug(
                "Using configured %s transfer for %s -> %s import",
                it->second == DMABufEGLProvider::BufferTransferStrategy::cpu ? "CPU" : "DMA",
                source_vendor.data(),
                dest_vendor.data());
            return it->second;
        }

        // If vendors are the same, use default
        if (source_view == dest_view)
        {
            return default_strategy;
        }


        // Fallback: Intel -> NVIDIA: Use CPU transfer (known bug)
        if (is_intel(source_view) && is_nvidia(dest_view))
        {
            mir::log_info("Using CPU transfer for Intel -> NVIDIA import (workaround for known issue)");
            return DMABufEGLProvider::BufferTransferStrategy::cpu;
        }

        // All other combinations: use default
        return default_strategy;
    };
}
