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

#ifndef MIR_GRAPHICS_GBM_KMS_QUIRKS_H_
#define MIR_GRAPHICS_GBM_KMS_QUIRKS_H_

#include <memory>
#include <boost/program_options.hpp>

namespace mir
{
namespace options
{
class Option;
}
namespace udev
{
class Device;
}

namespace graphics::atomic
{
/**
 * Interface for querying device-specific quirks
 */
class Quirks
{
public:
    explicit Quirks(options::Option const& options);
    ~Quirks();

    /**
     * Should this device be skipped entirely from use and probing?
     */
    [[nodiscard]]
    auto should_skip(udev::Device const& device) const -> bool;

    /**
     * Should we require this device to have modesetting support?
     */
    [[nodiscard]]
    auto require_modesetting_support(udev::Device const& device) const -> bool;

    static void add_quirks_option(boost::program_options::options_description& config);

private:
    class Impl;
    std::unique_ptr<Impl> const impl;
};
}
}


#endif //MIR_GRAPHICS_GBM_KMS_QUIRKS_H_
