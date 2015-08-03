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
 * Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/test/doubles/mock_gbm.h"
#include <gtest/gtest.h>

namespace mtd=mir::test::doubles;

namespace
{
mtd::MockGBM* global_mock = NULL;
}

mtd::FakeGBMResources::FakeGBMResources()
    : device(reinterpret_cast<gbm_device*>(0x12345678)),
      surface(reinterpret_cast<gbm_surface*>(0x1234abcd)),
      bo(reinterpret_cast<gbm_bo*>(0xabcdef12))
{
    bo_handle.u32 = 0x0987;
}

mtd::MockGBM::MockGBM()
{
    using namespace testing;
    assert(global_mock == NULL && "Only one mock object per process is allowed");

    global_mock = this;

    ON_CALL(*this, gbm_create_device(_))
    .WillByDefault(Return(fake_gbm.device));

    ON_CALL(*this, gbm_device_is_format_supported(_,_,_))
    .WillByDefault(Return(1));

    ON_CALL(*this, gbm_surface_create(fake_gbm.device,_,_,_,_))
    .WillByDefault(Return(fake_gbm.surface));

    ON_CALL(*this, gbm_surface_lock_front_buffer(fake_gbm.surface))
    .WillByDefault(Return(fake_gbm.bo));

    ON_CALL(*this, gbm_bo_create(fake_gbm.device,_,_,_,_))
    .WillByDefault(Return(fake_gbm.bo));

    ON_CALL(*this, gbm_bo_get_device(_))
    .WillByDefault(Return(fake_gbm.device));

    ON_CALL(*this, gbm_bo_get_handle(fake_gbm.bo))
    .WillByDefault(Return(fake_gbm.bo_handle));

    ON_CALL(*this, gbm_bo_set_user_data(_,_,_))
    .WillByDefault(Invoke(this, &MockGBM::on_gbm_bo_set_user_data));

    ON_CALL(*this, gbm_bo_write(_,_,_))
    .WillByDefault(Return(0));
}

mtd::MockGBM::~MockGBM()
{
    // this is probably later than optimal, but at least ensures memory freed
    for (auto i = destroyers.begin(); i != destroyers.end(); ++i) (*i)();
    global_mock = NULL;
}

struct gbm_device* gbm_create_device(int fd)
{
    return global_mock->gbm_create_device(fd);
}

void gbm_device_destroy(struct gbm_device *gbm)
{
    return global_mock->gbm_device_destroy(gbm);
}

int gbm_device_get_fd(struct gbm_device *gbm)
{
    return global_mock->gbm_device_get_fd(gbm);
}

int gbm_device_is_format_supported(struct gbm_device *gbm,
                                   uint32_t format, uint32_t usage)
{
    return global_mock->gbm_device_is_format_supported(gbm, format, usage);
}

struct gbm_surface *gbm_surface_create(struct gbm_device *gbm,
                                       uint32_t width, uint32_t height,
                                       uint32_t format, uint32_t flags)
{
    return global_mock->gbm_surface_create(gbm, width, height, format, flags);
}

void gbm_surface_destroy(struct gbm_surface *surface)
{
    return global_mock->gbm_surface_destroy(surface);
}

struct gbm_bo *gbm_surface_lock_front_buffer(struct gbm_surface *surface)
{
    return global_mock->gbm_surface_lock_front_buffer(surface);
}

void gbm_surface_release_buffer(struct gbm_surface *surface, struct gbm_bo *bo)
{
    global_mock->gbm_surface_release_buffer(surface, bo);
}

struct gbm_bo *gbm_bo_create(struct gbm_device *gbm,
                             uint32_t width, uint32_t height,
                             uint32_t format, uint32_t flags)
{
    return global_mock->gbm_bo_create(gbm, width, height, format,flags);
}

struct gbm_device *gbm_bo_get_device(struct gbm_bo *bo)
{
    return global_mock->gbm_bo_get_device(bo);
}

uint32_t gbm_bo_get_width(struct gbm_bo *bo)
{
    return global_mock->gbm_bo_get_width(bo);
}

uint32_t gbm_bo_get_height(struct gbm_bo *bo)
{
    return global_mock->gbm_bo_get_height(bo);
}

uint32_t gbm_bo_get_stride(struct gbm_bo *bo)
{
    return global_mock->gbm_bo_get_stride(bo);
}

uint32_t gbm_bo_get_format(struct gbm_bo *bo)
{
    return global_mock->gbm_bo_get_format(bo);
}

union gbm_bo_handle gbm_bo_get_handle(struct gbm_bo *bo)
{
    return global_mock->gbm_bo_get_handle(bo);
}

void gbm_bo_set_user_data(struct gbm_bo *bo, void *data,
                          void (*destroy_user_data)(struct gbm_bo *, void *))
{
    global_mock->gbm_bo_set_user_data(bo, data, destroy_user_data);
}

void *gbm_bo_get_user_data(struct gbm_bo *bo)
{
    return global_mock->gbm_bo_get_user_data(bo);
}

int gbm_bo_write(struct gbm_bo *bo, const void *buf, size_t count)
{
    return global_mock->gbm_bo_write(bo, buf, count);
}

void gbm_bo_destroy(struct gbm_bo *bo)
{
    return global_mock->gbm_bo_destroy(bo);
}

struct gbm_bo* gbm_bo_import(struct gbm_device* device, uint32_t type, void* data, uint32_t flags)
{
    return global_mock->gbm_bo_import(device, type, data, flags);
}
