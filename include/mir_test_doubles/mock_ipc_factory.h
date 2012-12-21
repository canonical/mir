/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 *              Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_IPC_FACTORY_H_
#define MIR_TEST_DOUBLES_MOCK_IPC_FACTORY_H_

#include "mir_test/empty_deleter.h"
#include "mir/frontend/protobuf_ipc_factory.h"
#include "mir/frontend/resource_cache.h"

#include <gmock/gmock.h>

namespace mf = mir::frontend;

namespace mir
{
namespace test
{
namespace doubles
{

class MockIpcFactory : public mf::ProtobufIpcFactory
{
public:
    MockIpcFactory(mir::protobuf::DisplayServer& server) :
        server(&server, EmptyDeleter()),
        cache(std::make_shared<mf::ResourceCache>())
    {
        using namespace testing;

        ON_CALL(*this, make_ipc_server()).WillByDefault(Return(this->server));

        // called during socket comms initialisation:
        // there's always a server awaiting the next connection
        EXPECT_CALL(*this, make_ipc_server()).Times(AtMost(1));
    }

    MOCK_METHOD0(make_ipc_server, std::shared_ptr<mir::protobuf::DisplayServer>());

private:
    virtual std::shared_ptr<mf::ResourceCache> resource_cache()
    {
        return cache;
    }

    std::shared_ptr<mir::protobuf::DisplayServer> server;
    std::shared_ptr<mf::ResourceCache> const cache;
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_IPC_FACTORY_H_ */
