/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "client_platform_factory.h"
#include "android_client_platform.h"

namespace mcl = mir::client;
namespace mcla = mcl::android;

std::shared_ptr<mcl::ClientPlatform>
mcla::ClientPlatformFactory::create_client_platform(mcl::ClientContext* /*context*/)
{
    return std::make_shared<mcla::AndroidClientPlatform>();
}

extern "C" std::shared_ptr<mcl::ClientPlatformFactory> mcl::create_client_platform_factory()
{
    return std::make_shared<mcla::ClientPlatformFactory>();
}
