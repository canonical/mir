/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "android_client_buffer_depository.h"
#include "android_client_buffer.h"

#include <boost/throw_exception.hpp>

namespace mcl=mir::client;
namespace mcla=mir::client::android;
namespace geom=mir::geometry;

mcla::AndroidClientBufferDepository::AndroidClientBufferDepository(const std::shared_ptr<AndroidRegistrar>& buffer_registrar)
 : registrar(buffer_registrar)
{
}

void mcla::AndroidClientBufferDepository::deposit_package(std::shared_ptr<MirBufferPackage>&& package, int id, geom::Size size, geom::PixelFormat pf)
{
    auto find_it = buffer_depository.find(id);
    if (find_it == buffer_depository.end())
    {
        auto buffer = std::make_shared<mcla::AndroidClientBuffer>(registrar, std::move(package), size, pf);
        buffer_depository[id] = buffer;
    }
}

std::shared_ptr<mcl::ClientBuffer> mcla::AndroidClientBufferDepository::access_buffer(int id)
{
    auto find_it = buffer_depository.find(id);
    if (find_it == buffer_depository.end())
        BOOST_THROW_EXCEPTION(std::runtime_error("server told client to use buffer before"));

    return buffer_depository[id];
}
