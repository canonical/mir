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

#ifndef MIR_FRONTEND_RESOURCE_CACHE_H_
#define MIR_FRONTEND_RESOURCE_CACHE_H_

#include "mir_protobuf.pb.h"
#include "mir/fd.h"

#include <map>
#include <memory>
#include <mutex>

namespace mir
{
namespace frontend
{

class MessageResourceCache
{
public:
    virtual void save_resource(google::protobuf::Message* key, std::shared_ptr<void> const& value) = 0;
    virtual void free_resource(google::protobuf::Message* key) = 0;
    virtual void save_fd(google::protobuf::Message* key, Fd const& fd) = 0;

    virtual ~MessageResourceCache() = default;
    MessageResourceCache() = default;
    MessageResourceCache(MessageResourceCache const&) = delete;
    MessageResourceCache& operator=(MessageResourceCache const&) = delete;
};

// Used to save resources that must be retained until a call completes.
class ResourceCache : public MessageResourceCache
{
public:
    void save_resource(google::protobuf::Message* key, std::shared_ptr<void> const& value);
    void save_fd(google::protobuf::Message* key, Fd const& fd);
    void free_resource(google::protobuf::Message* key);

private:
    typedef std::map<google::protobuf::Message*, std::shared_ptr<void>> Resources;
    typedef std::multimap<google::protobuf::Message*, mir::Fd> FdResources;

    std::mutex guard;
    Resources resources;
    FdResources fd_resources;
};

}
}

#endif /* MIR_FRONTEND_RESOURCE_CACHE_H_ */
