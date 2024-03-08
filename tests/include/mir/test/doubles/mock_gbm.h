/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_DOUBLES_MOCK_GBM_H_
#define MIR_TEST_DOUBLES_MOCK_GBM_H_

#include <gmock/gmock.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wall"
#include <gbm.h>
#pragma GCC diagnostic pop

namespace mir
{
namespace test
{
namespace doubles
{

class FakeGBMResources
{
public:
    FakeGBMResources();
    ~FakeGBMResources() = default;

    gbm_device  *device;
    gbm_surface *surface;
    gbm_bo *bo;
    gbm_bo_handle bo_handle;
};

class MockGBM
{
public:
    MockGBM();
    ~MockGBM();

    MOCK_METHOD(struct gbm_device*, gbm_create_device, (int fd));
    MOCK_METHOD(void, gbm_device_destroy, (struct gbm_device *gbm));
    MOCK_METHOD(int, gbm_device_get_fd, (struct gbm_device *gbm));
    MOCK_METHOD(int, gbm_device_is_format_supported, (struct gbm_device *gbm, uint32_t format, uint32_t usage));

    MOCK_METHOD(struct gbm_surface*, gbm_surface_create,
        (struct gbm_device *gbm, uint32_t width, uint32_t height, uint32_t format, uint32_t flags));
    MOCK_METHOD(void, gbm_surface_destroy, (struct gbm_surface *surface));
    MOCK_METHOD(struct gbm_bo*, gbm_surface_lock_front_buffer, (struct gbm_surface *surface));
    MOCK_METHOD(void, gbm_surface_release_buffer, (struct gbm_surface *surface, struct gbm_bo *bo));

    MOCK_METHOD(struct gbm_bo*, gbm_bo_create,
        (struct gbm_device *gbm, uint32_t width, uint32_t height, uint32_t format, uint32_t flags));
    MOCK_METHOD(struct gbm_device*, gbm_bo_get_device, (struct gbm_bo *bo));
    MOCK_METHOD(uint32_t, gbm_bo_get_width, (struct gbm_bo *bo));
    MOCK_METHOD(uint32_t, gbm_bo_get_height, (struct gbm_bo *bo));
    MOCK_METHOD(uint32_t, gbm_bo_get_stride, (struct gbm_bo *bo));
    MOCK_METHOD(uint32_t, gbm_bo_get_format, (struct gbm_bo *bo));
    MOCK_METHOD(union gbm_bo_handle, gbm_bo_get_handle, (struct gbm_bo *bo));
    MOCK_METHOD(void, gbm_bo_set_user_data,
                (struct gbm_bo *bo, void *data, void (*destroy_user_data)(struct gbm_bo *, void *)));
    MOCK_METHOD(void*, gbm_bo_get_user_data, (struct gbm_bo *bo));
    MOCK_METHOD(bool, gbm_bo_write, (struct gbm_bo *bo, const void *buf, size_t count));
    MOCK_METHOD(void, gbm_bo_destroy, (struct gbm_bo *bo));
    MOCK_METHOD(struct gbm_bo*, gbm_bo_import, (struct gbm_device*, uint32_t, void*, uint32_t));
    MOCK_METHOD(int, gbm_bo_get_fd, (gbm_bo*));

    FakeGBMResources fake_gbm;

private:
    void on_gbm_bo_set_user_data(struct gbm_bo *bo, void *data,
                                            void (*destroy_user_data)(struct gbm_bo *, void *))
    {
        destroyers.push_back(Destroyer{bo, data, destroy_user_data});
    }

    struct Destroyer
    {
        struct gbm_bo *bo;
        void *data;
        void (*destroy_user_data)(struct gbm_bo *, void *);

        void operator()() const { destroy_user_data(bo, data); }
    };

    std::vector<Destroyer> destroyers;
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_GBM_H_ */
