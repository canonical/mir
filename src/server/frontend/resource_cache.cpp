/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "resource_cache.h"

void mir::frontend::ResourceCache::save_resource(
    google::protobuf::MessageLite* key,
    std::shared_ptr<void> const& value)
{
    std::lock_guard<std::mutex> lock(guard);
    resources[key] = value;
}

void mir::frontend::ResourceCache::save_fd(
    google::protobuf::MessageLite* key,
    mir::Fd const& fd)
{
    std::lock_guard<std::mutex> lock(guard);
    fd_resources.emplace(key, fd);
}

void mir::frontend::ResourceCache::free_resource(google::protobuf::MessageLite* key)
{
    std::shared_ptr<void> value;
    {
        std::lock_guard<std::mutex> lock(guard);

        auto const& p = resources.find(key);

        if (p != resources.end())
        {
            value = p->second;
        }

        resources.erase(key);
        fd_resources.erase(key);
    }

}
