/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRAL_OUTPUT_CONFIGURATION_H
#define MIRAL_OUTPUT_CONFIGURATION_H

#include <mir/graphics/display_configuration.h>

#include <memory>
#include <span>
#include <utility>

namespace mir { class Server; }

namespace miral
{

/// Provides customization of the output configuration.
///
/// \remark Since MirAL 6.1
class OutputConfiguration
{
public:
    class Strategy;
    class NullStrategy;
    void operator()(mir::Server& server) const;

    /// Creates an output configuration with a default strategy that does nothing.
    OutputConfiguration();

    /// Creates an output configuration with the specified strategy.
    OutputConfiguration(std::shared_ptr<Strategy> strategy);

    /// Updates the strategy used to customize the output configuration and applies it.
    void update_strategy(std::shared_ptr<Strategy> strategy);

    ~OutputConfiguration();
    OutputConfiguration(OutputConfiguration const&);
    auto operator=(OutputConfiguration const&) -> OutputConfiguration&;

private:
    struct Self;
    std::shared_ptr<Self> self;
};

/// Interface for customization of the output configuration applied to the server.
///
/// The default implementation does nothing.
///
/// \remark Since MirAL 6.1
class OutputConfiguration::Strategy
{
public:
    Strategy();

    /// Called when the server is about to apply a new configuration. The strategy can modify the
    /// configuration before it is applied
    virtual void apply_configuration(std::span<mir::graphics::UserDisplayConfigurationOutput> outputs) = 0;

    /// Called after the server has applied a new configuration. The strategy can record the configuration
    /// that was applied, for example to write it to a log or a file.
    virtual void confirm_configuration(std::span<mir::graphics::UserDisplayConfigurationOutput const> outputs) = 0;
    virtual ~Strategy();

private:
    Strategy(Strategy const&) = delete;
    Strategy& operator=(Strategy const&) = delete;
};

/// A strategy that does nothing.
/// \remark Since MirAL 6.1
class OutputConfiguration::NullStrategy : public Strategy
{
public:
    void apply_configuration(std::span<mir::graphics::UserDisplayConfigurationOutput> outputs) override;
    void confirm_configuration(std::span<mir::graphics::UserDisplayConfigurationOutput const> outputs) override;
    ~NullStrategy() override;
};

}

#endif //MIRAL_OUTPUT_CONFIGURATION_H
