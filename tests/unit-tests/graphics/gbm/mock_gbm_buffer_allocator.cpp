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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include <stdexcept>
#include <cstring>
#include <gbm.h>
#include "gbmint.h"
#include "mock_gbm_buffer_allocator.h"

namespace mg=mir::graphics;
namespace mgg=mir::graphics::gbm;
namespace mc=mir::compositor;
namespace geom=mir::geometry;


static mgg::MockGBMDeviceCreator *thunk = NULL;

extern "C" {
static gbm_bo *mock_bo_create(gbm_device *gbm,
                            uint32_t width, uint32_t height,
                            uint32_t format,
                            uint32_t usage);

static void mock_bo_destroy(gbm_bo *bo);
}

mgg::MockGBMDeviceCreator::MockGBMDeviceCreator()
{
    using namespace testing;

    thunk = this;

    fake_device = std::shared_ptr<gbm_device>(new gbm_device);
    // Make it easy to find calls to unmocked functions.
    memset(fake_device.get(), 0xDEADBEEF, sizeof(fake_device));

    fake_device->bo_create = mock_bo_create;
    fake_device->bo_destroy = mock_bo_destroy;

    ON_CALL(*this, bo_create(_,_,_,_,_))
            .WillByDefault(Invoke(this, &MockGBMDeviceCreator::bo_create_impl));
}

std::unique_ptr<mgg::GBMBufferAllocator> mgg::MockGBMDeviceCreator::create_gbm_allocator()
{
    return std::unique_ptr<mgg::GBMBufferAllocator>(new GBMBufferAllocator(fake_device.get()));
}

gbm_bo *mgg::MockGBMDeviceCreator::bo_create_impl(gbm_device *dev,
                                                      uint32_t w, uint32_t h, uint32_t format, uint32_t usage)
{
    gbm_bo *bo = new gbm_bo();
    (void)format;
    (void)usage;
    (void)dev;
    bo->gbm = fake_device.get();
    bo->width = w;
    bo->height = h;
    switch (format) {
    case GBM_BO_FORMAT_ARGB8888:
        bo->stride = w * 4;
        break;
    default:
        throw std::runtime_error("Attempted to create buffer with unknown format");
    }
    return bo;
}

std::shared_ptr<gbm_device> mgg::MockGBMDeviceCreator::get_device()
{
    return fake_device;
}

extern "C" {
static gbm_bo *mock_bo_create(gbm_device *gbm,
                            uint32_t width, uint32_t height,
                            uint32_t format,
                            uint32_t usage)
{
    return thunk->bo_create(gbm, width, height, format, usage);
}

static void mock_bo_destroy(gbm_bo *bo)
{
    return thunk->bo_destroy(bo);
}
}
