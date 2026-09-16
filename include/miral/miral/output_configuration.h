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
#include <utility>

namespace mir { class Server; }

namespace miral
{

class OutputConfiguration
{
public:
    class Strategy;
    class NullStrategy;
    void operator()(mir::Server& server) const;

    OutputConfiguration();
    OutputConfiguration(std::shared_ptr<Strategy> strategy);

    void update_strategy(std::shared_ptr<Strategy> strategy);

    ~OutputConfiguration();
    OutputConfiguration(OutputConfiguration const&);
    auto operator=(OutputConfiguration const&) -> OutputConfiguration&;

private:
    struct Self;
    std::shared_ptr<Self> self;
};

class OutputConfiguration::Strategy
{
public:

    virtual ~Strategy() = default;
    virtual void apply_configuration(std::span<mir::graphics::UserDisplayConfigurationOutput> outputs) = 0;
    virtual void confirm_configuration(std::span<mir::graphics::UserDisplayConfigurationOutput const> outputs) = 0;
};
class OutputConfiguration::NullStrategy : public Strategy
{
public:
    void apply_configuration(std::span<mir::graphics::UserDisplayConfigurationOutput> outputs) override;
    void confirm_configuration(std::span<mir::graphics::UserDisplayConfigurationOutput const> outputs) override;
};

}

#endif //MIRAL_OUTPUT_CONFIGURATION_H
