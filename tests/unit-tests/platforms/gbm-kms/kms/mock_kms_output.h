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
    MOCK_CONST_METHOD0(id, uint32_t());
    MOCK_METHOD0(reset, void());
    MOCK_METHOD2(configure, void(geometry::Displacement, size_t));
    MOCK_CONST_METHOD0(size, geometry::Size());
    MOCK_CONST_METHOD0(max_refresh_rate, int());

    bool set_crtc(graphics::FBHandle const& fb) override
    {
        return set_crtc_thunk(&fb);
    }

    MOCK_METHOD0(has_crtc_mismatch, bool());
    MOCK_METHOD1(set_crtc_thunk, bool(graphics::FBHandle const*));
    MOCK_METHOD0(clear_crtc, void());

    bool schedule_page_flip(graphics::FBHandle const& fb) override
    {
        return schedule_page_flip_thunk(&fb);
    }
    MOCK_METHOD1(schedule_page_flip_thunk, bool(graphics::FBHandle const*));
    MOCK_METHOD0(wait_for_page_flip, void());

    MOCK_CONST_METHOD0(last_frame, graphics::Frame());

    MOCK_METHOD1(set_cursor_image, bool(gbm_bo*));
    MOCK_METHOD1(move_cursor, void(geometry::Point));
    MOCK_METHOD0(clear_cursor, bool());
    MOCK_CONST_METHOD0(has_cursor_image, bool());

    MOCK_METHOD1(set_power_mode, void(MirPowerMode));
    MOCK_METHOD1(set_gamma, void(mir::graphics::GammaCurves const&));

    MOCK_METHOD0(refresh_hardware_state, void());
    MOCK_CONST_METHOD1(update_from_hardware_state, void(graphics::DisplayConfigurationOutput&));

    MOCK_CONST_METHOD1(fb_for, std::shared_ptr<graphics::FBHandle const>(gbm_bo*));
    MOCK_CONST_METHOD1(fb_for, std::shared_ptr<graphics::FBHandle const>(graphics::DMABufBuffer const&));
    MOCK_CONST_METHOD1(buffer_requires_migration, bool(gbm_bo*));
    MOCK_CONST_METHOD0(drm_fd, int());
};

} // namespace test
} // namespace mir

#endif // MOCK_KMS_OUTPUT_H_
