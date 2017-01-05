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
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "egl_sync_fence.h"
#include "android_native_buffer.h"
#include "sync_fence.h"
#include "android_format_conversion-inl.h"
#include "gralloc_module.h"
#include "device_quirks.h"
#include "cmdstream_sync_factory.h"

#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <stdexcept>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

namespace
{
struct AndroidBufferHandleDeleter
{
    AndroidBufferHandleDeleter(std::shared_ptr<alloc_device_t> const& alloc_dev)
        : alloc_device(alloc_dev)
    {}

    void operator()(native_handle_t const* t)
    {
        alloc_device->free(alloc_device.get(), t);
    }
private:
    std::shared_ptr<alloc_device_t> const alloc_device;
};
}

mga::GrallocModule::GrallocModule(
    std::shared_ptr<struct alloc_device_t> const& alloc_device,
    std::shared_ptr<CommandStreamSyncFactory> const& sync_factory,
    std::shared_ptr<DeviceQuirks> const& quirks) :
    alloc_dev(alloc_device),
    sync_factory(sync_factory),
    quirks(quirks)
{
}

std::shared_ptr<mga::NativeBuffer> mga::GrallocModule::alloc_buffer(
    geometry::Size size, uint32_t format, uint32_t usage_flag)
{
    buffer_handle_t buf_handle = NULL;
    auto stride = 0;
    auto width = static_cast<int>(size.width.as_uint32_t());
    auto height = static_cast<int>(size.height.as_uint32_t());
    auto ret = alloc_dev->alloc(alloc_dev.get(), quirks->aligned_width(width), height,
                           format, usage_flag, &buf_handle, &stride);

    if (( ret ) || (buf_handle == NULL) || (stride == 0))
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(std::runtime_error("buffer allocation failed\n"))
            << boost::errinfo_errno(-ret));
    }

    AndroidBufferHandleDeleter del1(alloc_dev);
    std::shared_ptr<native_handle_t const> handle(buf_handle, del1);

    auto ops = std::make_shared<mga::RealSyncFileOps>();
    auto fence = std::make_shared<mga::SyncFence>(ops, mir::Fd());

    auto anwb = std::shared_ptr<mga::RefCountedNativeBuffer>(
        new mga::RefCountedNativeBuffer(handle),
        [](mga::RefCountedNativeBuffer* buffer)
        {
            buffer->mir_dereference();
        });

    anwb->width = width;
    anwb->height = height;
    anwb->stride = stride;
    anwb->handle = buf_handle;
    anwb->format = format;
    anwb->usage = usage_flag;

    return std::make_shared<mga::AndroidNativeBuffer>(anwb,
        sync_factory->create_command_stream_sync(),
        fence, mga::BufferAccess::read);
}
