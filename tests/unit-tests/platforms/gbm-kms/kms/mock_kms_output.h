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

#ifndef MOCK_KMS_OUTPUT_H_
#define MOCK_KMS_OUTPUT_H_

#include "src/platforms/gbm-kms/server/kms/kms_output.h"
#include <gmock/gmock.h>

namespace mir
{

namespace test
{

struct MockKMSOutput : public graphics::gbm::KMSOutput
{
    MOCK_METHOD(uint32_t, id, (), (const));
    MOCK_METHOD(void, reset, (), ());
    MOCK_METHOD(void, configure, (geometry::Displacement, size_t), ());
    MOCK_METHOD(geometry::Size, size, (), (const));
    MOCK_METHOD(int, max_refresh_rate, (), (const));

    bool set_crtc(graphics::FBHandle const& fb) override
    {
        return set_crtc_thunk(&fb);
    }

    MOCK_METHOD(bool, has_crtc_mismatch, (), ());
    MOCK_METHOD(bool, set_crtc_thunk, (graphics::FBHandle const*), ());
    MOCK_METHOD(void, clear_crtc, (), ());

    bool schedule_page_flip(graphics::FBHandle const& fb) override
    {
        return schedule_page_flip_thunk(&fb);
    }
    MOCK_METHOD(bool, schedule_page_flip_thunk, (graphics::FBHandle const*), ());
    MOCK_METHOD(void, wait_for_page_flip, (), ());

    MOCK_METHOD(graphics::Frame, last_frame, (), (const));

    MOCK_METHOD(bool, set_cursor_image, (gbm_bo*), ());
    MOCK_METHOD(void, move_cursor, (geometry::Point), ());
    MOCK_METHOD(bool, clear_cursor, (), ());
    MOCK_METHOD(bool, has_cursor_image, (), (const));

    MOCK_METHOD(void, set_power_mode, (MirPowerMode), ());
    MOCK_METHOD(void, set_gamma, (mir::graphics::GammaCurves const&), ());

    MOCK_METHOD(void, refresh_hardware_state, (), ());
    MOCK_METHOD(void, update_from_hardware_state, (graphics::DisplayConfigurationOutput&), (const));

    MOCK_METHOD(std::shared_ptr<graphics::FBHandle const>, fb_for, (gbm_bo*), (const));
    MOCK_METHOD(std::shared_ptr<graphics::FBHandle const>, fb_for, (graphics::DMABufBuffer const&), (const));
    MOCK_METHOD(bool, buffer_requires_migration, (gbm_bo*), (const));
    MOCK_METHOD(int, drm_fd, (), (const));
    MOCK_METHOD(bool, connected, (), (const));
};

} // namespace test
} // namespace mir

#endif // MOCK_KMS_OUTPUT_H_
