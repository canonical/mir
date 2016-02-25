/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_CMDSTREAM_SYNC_FACTORY_H_
#define MIR_GRAPHICS_ANDROID_CMDSTREAM_SYNC_FACTORY_H_

#include <memory>
namespace mir
{
namespace graphics
{
class CommandStreamSync;
namespace android
{
class CommandStreamSyncFactory
{
public:
    virtual ~CommandStreamSyncFactory() = default;
    virtual std::unique_ptr<CommandStreamSync> create_command_stream_sync() = 0;
protected:
    CommandStreamSyncFactory() = default;
    CommandStreamSyncFactory(CommandStreamSyncFactory const&) = delete;
    CommandStreamSyncFactory& operator=(CommandStreamSyncFactory const&) = delete;
};

class EGLSyncFactory : public CommandStreamSyncFactory
{
    std::unique_ptr<CommandStreamSync> create_command_stream_sync() override;
};

class NullCommandStreamSyncFactory : public CommandStreamSyncFactory
{
    std::unique_ptr<CommandStreamSync> create_command_stream_sync() override;
};

}
}
}
#endif /* MIR_GRAPHICS_ANDROID_CMDSTREAM_SYNC_FACTORY_H_ */
