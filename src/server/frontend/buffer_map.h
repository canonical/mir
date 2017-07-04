/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_COMPOSITOR_BUFFER_MAP_H_
#define MIR_COMPOSITOR_BUFFER_MAP_H_

#include <mutex>
#include <map>

namespace mir
{
namespace frontend
{
class BufferMap
{
public:
    BufferMap();

    graphics::BufferID add_buffer(std::shared_ptr<graphics::Buffer> const& buffer);

    void remove_buffer(graphics::BufferID id);

    std::shared_ptr<graphics::Buffer> get(graphics::BufferID) const;

private:
    std::mutex mutable mutex;

    enum class Owner;
    struct MapEntry
    {
        std::shared_ptr<graphics::Buffer> buffer;
        Owner owner;
    };
    typedef std::map<graphics::BufferID, MapEntry> Map;
    //used to keep strong reference
    Map buffers;

    Map::iterator checked_buffers_find(graphics::BufferID, std::unique_lock<std::mutex> const&);

    Map::const_iterator checked_buffers_find(graphics::BufferID, std::unique_lock<std::mutex> const&) const;
};
}
}
#endif /* MIR_COMPOSITOR_BUFFER_MAP_H_ */
