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
#include "quirk_common.h"

#include "mir/log.h"
#include "mir/options/option.h"
#include "mir/udev/wrapper.h"

#include <boost/algorithm/string.hpp>

#include <vector>
#include <unordered_set>

namespace mga = mir::graphics::atomic;
namespace mo = mir::options;
namespace mgc = mir::graphics::common;

namespace
{
char const* quirks_option_name = "driver-quirks";
}

namespace
{
auto value_or(char const* maybe_null_string, char const* value_if_null) -> char const*
{
    if (maybe_null_string)
    {
        return maybe_null_string;
    }
    else
    {
        return value_if_null;
    }
}
}

class mga::Quirks::Impl
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

            // <option>:{devnode,driver}:<specifier value>

            auto static const available_options = std::set<std::string>{
                "skip", "allow", "disable-kms-probe", "gbm-create-surface-flags", "gbm-surface-has-free-buffers"};
            auto const structure = mgc::validate_structure(tokens, available_options);
            if(!structure)
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
            else if (mgc::matches(tokens, "gbm-create-surface-flags", {"default", "no-flags"}))
            {
                auto const chosen_value = tokens[3];
                if (specifier == "devnode")
                    gbm_create_surface_flags.add_devnode(specifier_value, chosen_value);
                else if (specifier == "driver")
                    gbm_create_surface_flags.add_driver(specifier_value, chosen_value);

                return true;
            }
            else if (mgc::matches(tokens, "gbm-surface-has-free-buffers", {"default", "skip"}))
            {
                auto const chosen_value = tokens[3];
                if (specifier == "devnode")
                    gbm_surface_has_free_buffers.add_devnode(specifier_value, chosen_value);
                else if (specifier == "driver")
                    gbm_surface_has_free_buffers.add_driver(specifier_value, chosen_value);

                return true;
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
                    "(expects value of the form “{skip, allow}:{driver,devnode}:<driver or devnode>”"
                    ", “disable-kms-probe:{driver,devnode}:<driver or devnode>” "
                    "or “{gbm-create-surface-flags}:{driver,devnode}:<driver or devnode>:{default,no-flags}” or "
                    "“{gbm-surface-has-free-buffers}:{driver,devnode}:<driver or devnode>:{default,skip}”)",
                    quirks_option_name,
                    quirk.c_str());
            }
        }
    }

    auto should_skip(udev::Device const& device) const -> bool
    {
        auto const devnode = value_or(device.devnode(), "");
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
        auto const devnode = value_or(device.devnode(), "");
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
        class GbmCreateSurfaceNoFlags : public GbmQuirks::CreateSurfaceFlagsQuirk
        {
            auto gbm_create_surface_flags() const -> uint32_t override
            {
                return 0;
            }
        };

        class GbmSurfaceHasFreeBuffersAlwaysTrue : public GbmQuirks::SurfaceHasFreeBuffersQuirk
        {
            auto gbm_surface_has_free_buffers(gbm_surface*) const -> int override
            {
                return 1;
            }
        };

        class DefaultGbmCreateSurfaceFlags : public GbmQuirks::CreateSurfaceFlagsQuirk
        {
            auto gbm_create_surface_flags() const -> uint32_t override
            {
                return GBM_BO_USE_RENDERING | GBM_BO_USE_SCANOUT;
            }
        };

        class DefaultGbmSurfaceHasFreeBuffers : public GbmQuirks::SurfaceHasFreeBuffersQuirk
        {
            auto gbm_surface_has_free_buffers(gbm_surface* gbm_surface) const -> int override
            {
                return ::gbm_surface_has_free_buffers(gbm_surface);
            }
        };

        auto const driver = mgc::get_device_driver(device.parent().get());
        auto const devnode = device.devnode();
        mir::log_debug("Quirks(gbm-create-surface-flags + gbm-surface-has-free-buffers): checking device with devnode: %s, driver %s", devnode, driver);

        auto const devnode_or_driver =
            [](auto devnode, auto driver, auto devnodes, auto drivers) -> std::optional<std::string>
        {
            if (devnodes.contains(devnode))
                return devnodes.at(devnode);

            if (drivers.contains(driver))
                return drivers.at(driver);

            return std::nullopt;
        };

        auto create_surface_impl_name =
            devnode_or_driver(devnode, driver, gbm_create_surface_flags.devnodes, gbm_create_surface_flags.drivers)
                .transform(
                    [](auto impl_name)
                    {
                        mir::log_debug(
                            "Quirks(gbm-create-surface-flags): forcing %s implementation", impl_name.c_str());
                        return impl_name;
                    })
                .or_else(
                    [&driver] -> std::optional<std::string>

                    {
                        mir::log_debug(
                            "Quirks(gbm-create-surface-flags): using default implementation for %s driver", driver);
                        // Not specified
                        return driver;
                    })
                .value();

        auto create_surface_impl = [&]() -> std::unique_ptr<GbmQuirks::CreateSurfaceFlagsQuirk>
        {
            if (create_surface_impl_name == "no-flags" || create_surface_impl_name == "nvidia")
                return std::make_unique<GbmCreateSurfaceNoFlags>();

            return std::make_unique<DefaultGbmCreateSurfaceFlags>();
        }();

        auto surface_has_free_buffers_impl_name =
            devnode_or_driver(
                devnode, driver, gbm_surface_has_free_buffers.devnodes, gbm_surface_has_free_buffers.drivers)
                .transform(
                    [](auto impl_name)
                    {
                        mir::log_debug(
                            "Quirks(gbm-surface-has-free-buffers): forcing %s implementation", impl_name.c_str());
                        return impl_name;
                    })
                .or_else(
                    [&driver] -> std::optional<std::string>

                    {
                        mir::log_debug(
                            "Quirks(gbm-surface-has-free-buffers): using default implementation for %s driver", driver);
                        // Not specified
                        return driver;
                    })
                .value();

        auto surface_has_free_buffers_impl = [&]() -> std::unique_ptr<GbmQuirks::SurfaceHasFreeBuffersQuirk>
        {
            if (surface_has_free_buffers_impl_name == "skip" || surface_has_free_buffers_impl_name == "nvidia")
                return std::make_unique<GbmSurfaceHasFreeBuffersAlwaysTrue>();
            return std::make_unique<DefaultGbmSurfaceHasFreeBuffers>();
        }();

        return std::make_shared<GbmQuirks>(std::move(create_surface_impl), std::move(surface_has_free_buffers_impl));
    }

private:
    /* AST is a simple 2D output device, built into some motherboards.
     * They do not have any 3D engine associated, so were quirked off to avoid https://github.com/canonical/mir/issues/2678
     *
     * At least as of drivers ≤ version 550, the NVIDIA atomic implementation is buggy in a way that prevents
     * Mir from working. Quirk off atomic-kms on NVIDIA.
     *
     * Simple-framebuffer should be evicted before Mir even gets there.
     * https://bugs.launchpad.net/ubuntu/+source/linux/+bug/2084046
     * https://github.com/canonical/mir/issues/3710
     */
    mgc::AllowList completely_skip{{"ast", "simple-framebuffer"}};

    // We know this is currently useful for virtio_gpu, vc4-drm and v3d
    mgc::AllowList disable_kms_probe{{"virtio_gpu", "vc4-drm", "v3d"}};


    mgc::ValuedOption gbm_create_surface_flags;
    mgc::ValuedOption gbm_surface_has_free_buffers;
};

mga::Quirks::Quirks(const options::Option& options)
    : impl{std::make_unique<Impl>(options)}
{
}

mga::Quirks::~Quirks() = default;

auto mga::Quirks::should_skip(udev::Device const& device) const -> bool
{
    return impl->should_skip(device);
}

void mga::Quirks::add_quirks_option(boost::program_options::options_description& config)
{
    config.add_options()(
        quirks_option_name,
        boost::program_options::value<std::vector<std::string>>(),
        "[platform-specific] Driver quirks to apply (may be specified multiple times; multiple quirks are combined)");
}

auto mir::graphics::atomic::Quirks::require_modesetting_support(mir::udev::Device const& device) const -> bool
{
    if (getenv("MIR_MESA_KMS_DISABLE_MODESET_PROBE") != nullptr)
    {
        mir::log_debug("MIR_MESA_KMS_DISABLE_MODESET_PROBE is set");
        return false;
    }
    else if (getenv("MIR_GBM_KMS_DISABLE_MODESET_PROBE")  != nullptr)
    {
        mir::log_debug("MIR_GBM_KMS_DISABLE_MODESET_PROBE is set");
        return false;
    }
    else
    {
        return impl->require_modesetting_support(device);
    }
}
auto mir::graphics::atomic::Quirks::gbm_quirks_for(udev::Device const& device)
    -> std::shared_ptr<GbmQuirks>
{
    return impl->gbm_quirks_for(device);
}

mir::graphics::atomic::GbmQuirks::GbmQuirks(
    std::unique_ptr<CreateSurfaceFlagsQuirk> create_surface_flags,
    std::unique_ptr<SurfaceHasFreeBuffersQuirk> surface_has_free_buffers) :
    create_surface_flags{std::move(create_surface_flags)},
    surface_has_free_buffers{std::move(surface_has_free_buffers)}
{
}

auto mir::graphics::atomic::GbmQuirks::gbm_create_surface_flags() const -> uint32_t
{
    return create_surface_flags->gbm_create_surface_flags();
}

auto mir::graphics::atomic::GbmQuirks::gbm_surface_has_free_buffers(gbm_surface* gbm_surface) const -> int
{
    return surface_has_free_buffers->gbm_surface_has_free_buffers(gbm_surface);
}

