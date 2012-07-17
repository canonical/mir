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
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 */

#ifndef MIR_DISPLAY_SERVER_TEST_ENVIRONMENT
#define MIR_DISPLAY_SERVER_TEST_ENVIRONMENT

#include "mir/process/process.h"

#include <gtest/gtest.h>
#include <memory>

namespace mir
{
class DisplayServer;
namespace compositor
{
class BufferAllocationStrategy;
}
namespace graphics
{
class Renderer;
}
}

// The test fixture sets up and tears down a display server for use
// in display server tests.
class DisplayServerTestEnvironment : public testing::Test
{
public:
    DisplayServerTestEnvironment();
    ~DisplayServerTestEnvironment();
    void startServer(std::function<void()>&& functor);
    void SetUp();
    void TearDown();
    mir::DisplayServer* displayServer() const;

private:
    std::unique_ptr<mir::DisplayServer> server;
    std::shared_ptr<mir::process::Process> server_process;
    virtual std::shared_ptr<mir::graphics::Renderer> makeRenderer();
    virtual std::shared_ptr<mir::compositor::BufferAllocationStrategy> makeBufferAllocationStrategy();
};

// Helper function which converts a gtest result into a Process exit status.
int test_exit();

#endif // MIR_DISPLAY_SERVER_TEST_ENVIRONMENT
