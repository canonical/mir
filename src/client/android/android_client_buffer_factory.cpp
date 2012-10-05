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
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "android/android_client_buffer_factory.h"
#include "android/android_client_buffer.h"

//GO AWAY
#include "mir_test/empty_deleter.h"

namespace mcl=mir::client;

mcl::AndroidClientBufferFactory::AndroidClientBufferFactory()
{
}

std::shared_ptr<mcl::ClientBuffer> mcl::AndroidClientBufferFactory::create_buffer_from_ipc_message(const MirBufferPackage&)
{
    mcl::AndroidClientBuffer* null = NULL;
   
    auto test = std::shared_ptr<mcl::AndroidClientBuffer>(null, mir::EmptyDeleter()); 
    return test;
} 
