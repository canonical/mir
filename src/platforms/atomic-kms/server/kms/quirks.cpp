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

#include <boost/algorithm/string.hpp>

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

        auto const process_one_option = [&](auto option_value)
        {
            std::vector<std::string> tokens;
            boost::split(tokens, option_value, boost::is_any_of(":"));

            // <option>:{devnode,driver}:<specifier value>
            auto const structure = validate_structure(tokens);
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
            else if (option == "force-runtime-quirk")
            {
                if (tokens.size() < 4)
                    return false;

                auto const runtime_quirk = tokens[3];
                if (runtime_quirk == "default" || runtime_quirk == "nvidia")
                {
                    if (specifier == "devnode")
                        force_runtime_quirk.add_devnode(specifier_value, runtime_quirk);
                    else if (specifier == "driver")
                        force_runtime_quirk.add_driver(specifier_value, runtime_quirk);

                    return true;
                }
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
                    "or “force-runtime-quirk:{driver,devnode}:<driver or devnode>:{default,nvidia}”)",
                    quirks_option_name,
                    quirk.c_str());
            }
        }
    }

    auto should_skip(udev::Device const& device) const -> bool
    {
        auto const devnode = value_or(device.devnode(), "");
        auto const parent_device = device.parent();
        auto const driver = get_device_driver(parent_device.get());

        mir::log_debug("Quirks(skip/allow): checking device with devnode: %s, driver %s", device.devnode(), driver);

        // Devnodes have higher precedence
        //
        //  ? = unspecified
        //
        //  devnode driver  outcome
        //  0: ?       skip    skip
        //  1: ?       ?       no skip
        //  2: ?       allow   no skip
        //
        //  3: skip    ?       skip
        //  4: skip    skip    skip
        //  5: skip    allow   skip
        //
        //  6: allow   ?       no skip
        //  7: allow   skip    no skip
        //  8: allow   allow   no skip
        //
        //  Cases 3-5 are the complete opposite of cases 6-8
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
        auto const driver = get_device_driver(parent_device.get());
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

    auto runtime_quirks_for(udev::Device const& device) -> std::shared_ptr<GbmQuirks>
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

        auto const driver = get_device_driver(device.parent().get());
        auto const devnode = device.devnode();
        mir::log_debug("Quirks(force-runtime-quirk): checking device with devnode: %s, driver %s", devnode, driver);

        auto const chosen_quirk = [&] -> std::optional<std::string>
        {
            if (auto const iter = force_runtime_quirk.devnodes.find(devnode); iter != force_runtime_quirk.devnodes.end())
            {
                return {iter->second};
            }

            if (auto const iter = force_runtime_quirk.drivers.find(driver); iter != force_runtime_quirk.drivers.end())
            {
                return {iter->second};
            }

            return std::nullopt;
        }();

        chosen_quirk
            .transform(
                [](auto fq)
                {
                    mir::log_debug("Quirks(force-runtime-quirk): forcing runtime quirk %s", fq.c_str());
                    return fq;
                })
            .or_else(
                [&driver] -> std::optional<std::string>
                {
                    mir::log_debug("Quirks(force-runtime-quirk): using runtime quirk for “%s” driver", driver);
                    return {driver};
                });

        if (chosen_quirk == "nvidia")
            return std::make_shared<NvidiaGbmQuirks>();

        return std::make_shared<DefaultGbmQuirks>();
    }

private:
    static auto get_device_driver(mir::udev::Device const* parent_device) -> const char*
    {
        if (parent_device)
        {
            return value_or(parent_device->driver(), "");
        }
        mir::log_warning("udev device has no parent! Unable to determine driver for quirks.");
        return "<UNKNOWN>";
    }

    static auto validate_structure(std::vector<std::string> const& tokens) -> std::optional<std::tuple<std::string, std::string, std::string>>
    {
        if (tokens.size() < 1)
            return {};
        auto const option = tokens[0];

        auto const available_options =
            std::set<std::string>{"skip", "allow", "disable-kms-probe", "force-runtime-quirk"};
        if (!available_options.contains(option))
            return {};

        if (tokens.size() < 3)
            return {};
        auto const specifier = tokens[1];
        auto const specifier_value = tokens[2];

        if (specifier != "driver" || specifier != "devnode")
            return {};

        return {{option, specifier, specifier_value}};
    }

    struct AllowList
    {
        AllowList(std::unordered_set<std::string>&& drivers_to_skip) :
            skipped_drivers{drivers_to_skip}
        {
        }

        void allow(std::string specifier, std::string specifier_value)
        {
            if (specifier == "devnode")
                skipped_devnodes.erase(specifier_value);
            else if (specifier == "driver")
                skipped_drivers.erase(specifier_value);
        }

        void skip(std::string specifier, std::string specifier_value)
        {
            if (specifier == "devnode")
                skipped_devnodes.insert(specifier_value);
            else if (specifier == "driver")
                skipped_drivers.insert(specifier_value);
        }

        std::unordered_set<std::string> skipped_drivers;
        std::unordered_set<std::string> skipped_devnodes;
    };

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
    AllowList completely_skip{{"ast", "simple-framebuffer"}};

    // We know this is currently useful for virtio_gpu, vc4-drm and v3d
    AllowList disable_kms_probe{{"virtio_gpu", "vc4-drm", "v3d"}};

    struct ValuedOption
    {
        void add_devnode(std::string const& devnode, std::string const& quirk)
        {
            devnodes.insert_or_assign(devnode, quirk);
        }
        void add_driver(std::string const& driver, std::string const& quirk)
        {
            drivers.insert_or_assign(driver, quirk);
        }

        std::unordered_map<std::string, std::string> drivers;
        std::unordered_map<std::string, std::string> devnodes;
    };

    ValuedOption force_runtime_quirk;
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
auto mir::graphics::atomic::Quirks::runtime_quirks_for(udev::Device const& device)
    -> std::shared_ptr<GbmQuirks>
{
    return impl->runtime_quirks_for(device);
}

