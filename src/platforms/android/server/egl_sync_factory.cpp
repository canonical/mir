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

#include "cmdstream_sync_factory.h"
#include "egl_sync_fence.h"

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;

std::unique_ptr<mg::CommandStreamSync> mga::EGLSyncFactory::create_command_stream_sync()
{
    try
    {
        return std::make_unique<EGLSyncFence>(std::make_shared<mg::EGLSyncExtensions>());
    }
    catch (std::runtime_error&)
    {
        return std::make_unique<NullCommandSync>();
    }
}

std::unique_ptr<mg::CommandStreamSync> mga::NullCommandStreamSyncFactory::create_command_stream_sync()
{
    return std::make_unique<NullCommandSync>();
}
