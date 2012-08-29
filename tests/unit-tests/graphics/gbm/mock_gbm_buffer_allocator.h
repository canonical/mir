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

#ifndef MOCK_GBM_BUFFER_ALLOCATOR_H
#define MOCK_GBM_BUFFER_ALLOCATOR_H

#include <stdexcept>
#include <memory>
#include <gbm.h>
#include "gbmint.h"
#include <gmock/gmock.h>

#include "mir/graphics/gbm/gbm_buffer_allocator.h"

namespace mir
{
namespace graphics
{
namespace gbm
{

class MockGBMDeviceCreator
{
public:
    MockGBMDeviceCreator();

    MOCK_METHOD5(bo_create, gbm_bo *(gbm_device *, uint32_t, uint32_t, uint32_t, uint32_t));
    MOCK_METHOD1(bo_destroy, void(gbm_bo *));

    std::unique_ptr<GBMBufferAllocator> create_gbm_allocator();
    std::shared_ptr<gbm_device> get_device();

private:
    gbm_bo *bo_create_impl(gbm_device *dev, uint32_t w, uint32_t h, uint32_t format, uint32_t usage);

    std::shared_ptr<gbm_device> fake_device;
};

}
}
}
#endif // MOCK_GBM_BUFFER_ALLOCATOR_H
