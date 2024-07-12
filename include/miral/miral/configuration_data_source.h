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

#ifndef MIRAL_CONFIGURATION_SOURCE_H
#define MIRAL_CONFIGURATION_SOURCE_H

#include <functional>
#include <map>
#include <string>
#include <memory>

namespace mir { class Server; }

namespace miral
{

/// Used to define another source of configuration data. By default, configuration
/// options are parsed from command line parameters and the Mir configuration file.
/// Using this class, compositor authors can provide another way to set configuration
/// options. For example, one may parse a JSON file to the map and define configuration
/// variable in that way.
class ConfigurationDataSource
{
public:
    explicit ConfigurationDataSource(std::function<std::map<std::string, std::string>()> const&);
    void operator()(mir::Server& server) const;

    struct Self;
    std::shared_ptr<Self> self;
};
}

#endif //MIRAL_CONFIGURATION_SOURCE_H
