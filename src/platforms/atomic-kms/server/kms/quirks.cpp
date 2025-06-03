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

#include "mir/log.h"
#include "mir/options/option.h"
#include "mir/udev/wrapper.h"


#include <vector>
#include <unordered_set>

namespace mga = mir::graphics::atomic;
namespace mo = mir::options;

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

        for (auto const& quirk : options.get<std::vector<std::string>>(quirks_option_name))
        {
            auto const disable_kms_probe = "disable-kms-probe:";
            auto const skip_devnode = "skip:devnode:";
            auto const skip_driver = "skip:driver:";
            auto const allow_devnode = "allow:devnode:";
            auto const allow_driver = "allow:driver:";

            if (quirk.starts_with(skip_devnode))
            {
                devnodes_to_skip.insert(quirk.substr(strlen(skip_devnode)));
                continue;
            }
            else if (quirk.starts_with(skip_driver))
            {
                drivers_to_skip.insert(quirk.substr(strlen(skip_driver)));
                continue;
            }
            else if (quirk.starts_with(allow_devnode))
            {
                devnodes_to_skip.erase(quirk.substr(strlen(allow_devnode)));
                continue;
            }
            else if (quirk.starts_with(allow_driver))
            {
                drivers_to_skip.erase(quirk.substr(strlen(allow_driver)));
                continue;
            }
            else if (quirk.starts_with(disable_kms_probe))
            {
                // Quirk format is disable-kms-probe:value
                skip_modesetting_support.emplace(quirk.substr(strlen(disable_kms_probe)));
                continue;
            }

            // If we didn't `continue` above, we're ignoring...
            mir::log_warning(
                "Ignoring unexpected value for %s option: %s "
                "(expects value of the form “skip:<type>:<value>”, “allow:<type>:<value>” or ”disable-kms-probe:<value>”)",
                quirks_option_name,
                quirk.c_str());
        }
    }

    auto should_skip(udev::Device const& device) const -> bool
    {
        auto const devnode = value_or(device.devnode(), "");
        auto const parent_device = device.parent();
        auto const driver = get_device_driver(parent_device.get());
        mir::log_debug("Quirks: checking device with devnode: %s, driver %s", device.devnode(), driver);
        bool const should_skip_driver = drivers_to_skip.count(driver);
        bool const should_skip_devnode = devnodes_to_skip.count(devnode);
        if (should_skip_driver)
        {
            mir::log_info("Quirks: skipping device %s (matches driver quirk %s)", devnode, driver);
        }
        if (should_skip_devnode)
        {
            mir::log_info("Quirks: skipping device %s (matches devnode quirk %s)", devnode, devnode);
        }
        return should_skip_driver || should_skip_devnode;
    }

    auto require_modesetting_support(mir::udev::Device const& device) const -> bool
    {
        auto const devnode = value_or(device.devnode(), "");
        auto const parent_device = device.parent();
        auto const driver = get_device_driver(parent_device.get());
        mir::log_debug("Quirks: checking device with devnode: %s, driver %s", device.devnode(), driver);

        bool const should_skip_modesetting_support = skip_modesetting_support.count(driver);
        if (should_skip_modesetting_support)
        {
            mir::log_info("Quirks: skipping modesetting check %s (matches driver quirk %s)", devnode, driver);
        }
        return !should_skip_modesetting_support;
    }

    auto runtime_quirks_for(udev::Device const& device) -> std::shared_ptr<GbmQuirks>
    {
        auto const driver = get_device_driver(device.parent().get());
        if(std::strcmp(driver, "nvidia") == 0)
        {
            class NvidiaGbmQuirks : public GbmQuirks
            {
                auto gbm_create_surface_flags() const -> uint32_t override
                {
                    return 0;
                }
                auto gbm_surface_has_free_buffers(gbm_surface*) const -> int override
                {
                    return 1;
                }
            };

            return std::make_shared<NvidiaGbmQuirks>();
        }

        class DefaultGbmQuirks : public GbmQuirks
        {
            auto gbm_create_surface_flags() const -> uint32_t override
            {
                return GBM_BO_USE_RENDERING | GBM_BO_USE_SCANOUT;
            }
            auto gbm_surface_has_free_buffers(gbm_surface* gbm_surface) const -> int override
            {
                return ::gbm_surface_has_free_buffers(gbm_surface);
            }
        };

        return std::make_shared<DefaultGbmQuirks>();
    }

private:
    auto get_device_driver(mir::udev::Device const* parent_device) const -> const char*
    {
        if (parent_device)
        {
            return value_or(parent_device->driver(), "");
        }
        mir::log_warning("udev device has no parent! Unable to determine driver for quirks.");
        return "<UNKNOWN>";
    }

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
    std::unordered_set<std::string> drivers_to_skip = { "ast", "simple-framebuffer"};
    std::unordered_set<std::string> devnodes_to_skip;
    // We know this is currently useful for virtio_gpu, vc4-drm and v3d
    std::unordered_set<std::string> skip_modesetting_support = { "virtio_gpu", "vc4-drm", "v3d" };
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
    config.add_options()
        (quirks_option_name,
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
auto mir::graphics::atomic::Quirks::runtime_quirks_for(udev::Device const& device)
    -> std::shared_ptr<GbmQuirks>
{
    return impl->runtime_quirks_for(device);
}

