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
 *              Thomas Guest <thomas.guest@canonical.com>
 *              Alan Griffiths <alan@octopull.co.uk>
 */

#include "display_server_test_fixture.h"

#include <boost/asio.hpp>
#include <boost/signals2.hpp>

#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mp = mir::process;

namespace
{
namespace detail
{
namespace ba = boost::asio;
namespace bal = boost::asio::local;
namespace bs = boost::system;

struct SurfaceState
{
    virtual bool is_valid() const = 0;
    virtual bs::error_code disconnect() = 0;
    virtual ~SurfaceState() {}
protected:
    SurfaceState() = default;
    SurfaceState(SurfaceState const&) = delete;
    SurfaceState& operator=(SurfaceState const&) = delete;
};
struct InvalidSurfaceState : public SurfaceState
{
    bs::error_code disconnect()
    {
        return bs::error_code();
    }

    bool is_valid() const
    {
        return false;
    }
};
struct ValidSurfaceState : public SurfaceState
{
    ba::io_service io_service;
    bal::stream_protocol::endpoint endpoint;
    bal::stream_protocol::socket socket;

    ValidSurfaceState()
    : endpoint(mir::test_socket_file()), socket(io_service)
    {
        socket.connect(endpoint);
    }

    bs::error_code disconnect()
    {
        bs::error_code error;
        ba::write(socket, ba::buffer(std::string("disconnect\n")), error);
        return error;
    }

    bool is_valid() const
    {
        return true;
    }
};
}

class Surface
{
public:
    Surface() : body(new detail::InvalidSurfaceState()) {}
    Surface(int /*width*/, int /*height*/, int /*pix_format*/)
        : body(new detail::ValidSurfaceState()) {}
    ~Surface()
    {
        if (body)
        {
            body->disconnect();
        }
        delete body;
    }

    bool is_valid() const
    {
        return body->is_valid();
    }
    Surface(Surface&& that) : body(that.body)
    {
        that.body = 0;
    }
    Surface& operator=(Surface&& that)
    {
        delete body;
        body = that.body;
        that.body = 0;
        return *this;
    }

private:
    detail::SurfaceState* body;
    Surface(Surface const&) = delete;
    Surface& operator=(Surface const&) = delete;
};
}

TEST_F(DefaultDisplayServerTestFixture, client_connects_and_disconnects)
{
    struct Client : TestingClientConfiguration
    {
        void exec()
        {
            // Default surface is not connected
            Surface mysurface;
            EXPECT_FALSE(mysurface.is_valid());

            // connect surface
            EXPECT_NO_THROW(mysurface = Surface(640, 480, 0));
            EXPECT_TRUE(mysurface.is_valid());

            // disconnect surface
            EXPECT_NO_THROW(mysurface = Surface());
            EXPECT_FALSE(mysurface.is_valid());
        }
    } client_connects_and_disconnects;

    launch_client_process(client_connects_and_disconnects);
}
