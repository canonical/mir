/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by:
 * Eleni Maria Stea <elenimaria.stea@canonical.com>
 * Alan Griffiths <alan@octopull.co.uk>
 */

#include "native_gbm_platform.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mgg = mg::gbm;

std::shared_ptr<mg::GraphicBufferAllocator> mgg::NativeGBMPlatform::create_buffer_allocator(
        std::shared_ptr<mg::BufferInitializer> const& /*buffer_initializer*/)
{
    BOOST_THROW_EXCEPTION(std::runtime_error("Mir NativeGBMPlatform::create_buffer_allocator is not implemented yet!"));
}

std::shared_ptr<mg::PlatformIPCPackage> mgg::NativeGBMPlatform::get_ipc_package()
{
    BOOST_THROW_EXCEPTION(std::runtime_error("Mir NativeGBMPlatform::get_ipc_package is not implemented yet!"));
}

std::shared_ptr<mg::InternalClient> mgg::NativeGBMPlatform::create_internal_client()
{
    BOOST_THROW_EXCEPTION(std::runtime_error("Mir NativeGBMPlatform::create_internal_client is not implemented yet!"));
}

void mgg::NativeGBMPlatform::fill_ipc_package(std::shared_ptr<mg::BufferIPCPacker> const& /*packer*/,
        std::shared_ptr<mg::Buffer> const& /*buffer*/) const
{
    BOOST_THROW_EXCEPTION(std::runtime_error("Mir NativeGBMPlatform::fill_ipc_package is not implemented yet!"));
}

extern "C" std::shared_ptr<mg::NativePlatform> create_native_platform()
{
    return std::make_shared<mgg::NativeGBMPlatform>();
}
