/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_SCENE_DATA_EXCHANGE_H_
#define MIR_SCENE_DATA_EXCHANGE_H_

#include "mir/fd.h"

#include <vector>
#include <string>

namespace mir
{
namespace scene
{
/// Interface representing a data source (such as an offer by a client to transfer data). Implemented by the
/// frontend. Implementations must be threadsafe.
class DataExchangeSource
{
public:
    DataExchangeSource() = default;
    virtual ~DataExchangeSource() = default;

    /// Returns the list of supported mime types.
    virtual auto mime_types() const -> std::vector<std::string> const& = 0;

    /// Instructs the source to start sending it's data in the given mime type to the given fd.
    virtual void initiate_send(std::string const& mime_type, Fd const& target_fd) = 0;

private:
    DataExchangeSource(DataExchangeSource const&) = delete;
    DataExchangeSource& operator=(DataExchangeSource const&) = delete;
};
}
}
#endif //MIR_SCENE_DATA_EXCHANGE_H_
