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

#ifndef MIRAL_LOGGING_CONFIG_H_
#define MIRAL_LOGGING_CONFIG_H_

#include <memory>

namespace miral
{
namespace live_config
{
class Store;
}

/// Hook up log filtering configuration to a `live_config::Store`
///
/// This is a convenience function that registers a log filtering
/// option on the live config store and monitors it for changes.
/// \remark Since MirAL 5.8
void register_log_filtering_config(live_config::Store& config_store);
}


#endif // MIRAL_LOGGING_CONFIG_H_
