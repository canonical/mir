/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_CMDSTREAM_SYNC_FACTORY_H_
#define MIR_TEST_DOUBLES_STUB_CMDSTREAM_SYNC_FACTORY_H_

#include "src/platforms/android/server/cmdstream_sync_factory.h"

namespace mir
{
namespace test
{
namespace doubles
{
struct StubCmdStreamSyncFactory : graphics::android::CommandStreamSyncFactory
{
    std::unique_ptr<graphics::CommandStreamSync> create_command_stream_sync() override
    {
        return std::make_unique<graphics::NullCommandSync>();
    }
};
}
}
}

#endif /* MIR_TEST_DOUBLES_STUB_CMDSTREAM_SYNC_FACTORY_H_ */
