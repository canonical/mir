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

namespace mcl=mir::client;
namespace geom=mir::geometry;

mcl::AndroidClientBufferFactory::AndroidClientBufferFactory(const std::shared_ptr<AndroidRegistrar>& buffer_registrar)
 : registrar(buffer_registrar)
{
}

std::shared_ptr<mcl::ClientBuffer> mcl::AndroidClientBufferFactory::create_buffer_from_ipc_message(std::shared_ptr<MirBufferPackage>&& package, geometry::Width w, geometry::Height h, geometry::PixelFormat pf)
{
    return std::make_shared<mcl::AndroidClientBuffer>(registrar, std::move(package), w, h, pf );
} 
