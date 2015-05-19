/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "src/platforms/android/server/hwc_loggers.h"
#include <memory>
#include <iostream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mga=mir::graphics::android;

namespace
{
struct HwcLogger : public ::testing::Test
{
    void fill_display_list(hwc_display_contents_1_t* display_list)
    {
        display_list->numHwLayers = num_layers;

        display_list->hwLayers[0].compositionType = HWC_OVERLAY;
        display_list->hwLayers[0].flags = 0; 
        display_list->hwLayers[0].handle = &native_handle1; 
        display_list->hwLayers[0].transform = HWC_TRANSFORM_ROT_90;
        display_list->hwLayers[0].blending = HWC_BLENDING_NONE;
        display_list->hwLayers[0].displayFrame = {1, 1, 2, 1}; 
        display_list->hwLayers[0].sourceCrop = {3, 2, 5, 3}; 
        display_list->hwLayers[0].acquireFenceFd = fake_fence[0];
        display_list->hwLayers[0].releaseFenceFd = fake_fence[1];

        display_list->hwLayers[1].compositionType = HWC_FRAMEBUFFER; 
        display_list->hwLayers[1].flags = 0;
        display_list->hwLayers[1].handle = &native_handle2;
        display_list->hwLayers[1].transform = HWC_TRANSFORM_ROT_180;
        display_list->hwLayers[1].blending = HWC_BLENDING_PREMULT;
        display_list->hwLayers[1].displayFrame = {8, 5, 13, 8}; 
        display_list->hwLayers[1].sourceCrop = {21, 13, 34, 21}; 
        display_list->hwLayers[1].acquireFenceFd = fake_fence[2];
        display_list->hwLayers[1].releaseFenceFd = fake_fence[3];

        display_list->hwLayers[2].compositionType = HWC_FRAMEBUFFER; 
        display_list->hwLayers[2].flags = HWC_SKIP_LAYER;
        display_list->hwLayers[2].handle = &native_handle3; 
        display_list->hwLayers[2].transform = HWC_TRANSFORM_ROT_270;
        display_list->hwLayers[2].blending = HWC_BLENDING_COVERAGE; 
        display_list->hwLayers[2].displayFrame = {55, 34, 89, 55};  
        display_list->hwLayers[2].sourceCrop = {144, 89, 233, 144}; 
        display_list->hwLayers[2].acquireFenceFd = fake_fence[4];
        display_list->hwLayers[2].releaseFenceFd = fake_fence[5];

        display_list->hwLayers[3].compositionType = HWC_FRAMEBUFFER_TARGET; 
        display_list->hwLayers[3].flags = 0; 
        display_list->hwLayers[3].handle = &native_handle4; 
        display_list->hwLayers[3].transform = 0;
        display_list->hwLayers[3].blending = HWC_BLENDING_NONE; 
        display_list->hwLayers[3].displayFrame = {377, 233, 610, 337}; 
        display_list->hwLayers[3].sourceCrop = {987, 610, 1597, 987};
        display_list->hwLayers[3].acquireFenceFd = fake_fence[6];
        display_list->hwLayers[3].releaseFenceFd = fake_fence[7];
    }

    HwcLogger()
     : num_layers{4},
       primary_list{std::shared_ptr<hwc_display_contents_1_t>(
            static_cast<hwc_display_contents_1_t*>(
           ::operator new(sizeof(hwc_display_contents_1_t) + (num_layers * sizeof(hwc_layer_1_t)))))},
       external_list{std::shared_ptr<hwc_display_contents_1_t>(
            static_cast<hwc_display_contents_1_t*>(
           ::operator new(sizeof(hwc_display_contents_1_t) + (num_layers * sizeof(hwc_layer_1_t)))))}
    {
        default_cout_buffer = std::cout.rdbuf();
        std::cout.rdbuf(test_stream.rdbuf());

        fill_display_list(primary_list.get());
        fill_display_list(external_list.get());
        display_list[HWC_DISPLAY_PRIMARY] = primary_list.get();
        display_list[HWC_DISPLAY_EXTERNAL] = external_list.get();
        display_list[HWC_DISPLAY_VIRTUAL] = nullptr;
    };

    virtual ~HwcLogger()
    {
        std::cout.rdbuf(default_cout_buffer);
    }

    decltype(std::cout.rdbuf()) default_cout_buffer;
    std::ostringstream test_stream;
    size_t const num_layers;
    std::shared_ptr<hwc_display_contents_1_t> const primary_list;
    std::shared_ptr<hwc_display_contents_1_t> const external_list;
    std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> display_list;
    std::array<int, 8> const fake_fence{ {4,5,6,7,8,9,10,11} };
    native_handle_t native_handle1;
    native_handle_t native_handle2;
    native_handle_t native_handle3;
    native_handle_t native_handle4;
};
}

TEST_F(HwcLogger, report_pre_prepare)
{
    std::stringstream str;
    str << "before prepare():" << std::endl
        << " # | display  | Type      | pos {l,t,r,b}         | crop {l,t,r,b}        | transform | blending | " << std::endl
        << " 0 | primary  | OVERLAY   | {   1,   1,   2,   1} | {   3,   2,   5,   3} | ROT_90    | NONE     | " << std::endl
        << " 1 | primary  | GL_RENDER | {   8,   5,  13,   8} | {  21,  13,  34,  21} | ROT_180   | PREMULT  | " << std::endl
        << " 2 | primary  | FORCE_GL  | {  55,  34,  89,  55} | { 144,  89, 233, 144} | ROT_270   | COVERAGE | " << std::endl
        << " 3 | primary  | FB_TARGET | { 377, 233, 610, 337} | { 987, 610,1597, 987} | NONE      | NONE     | " << std::endl
        << " 0 | external | OVERLAY   | {   1,   1,   2,   1} | {   3,   2,   5,   3} | ROT_90    | NONE     | " << std::endl
        << " 1 | external | GL_RENDER | {   8,   5,  13,   8} | {  21,  13,  34,  21} | ROT_180   | PREMULT  | " << std::endl
        << " 2 | external | FORCE_GL  | {  55,  34,  89,  55} | { 144,  89, 233, 144} | ROT_270   | COVERAGE | " << std::endl
        << " 3 | external | FB_TARGET | { 377, 233, 610, 337} | { 987, 610,1597, 987} | NONE      | NONE     | " << std::endl;
    mga::HwcFormattedLogger logger;
    logger.set_version(mga::HwcVersion::hwc12);
    logger.report_list_submitted_to_prepare(display_list);
    EXPECT_EQ(str.str(), test_stream.str()); 
}

TEST_F(HwcLogger, report_post_prepare)
{
    std::stringstream str;
    str << "after prepare():" << std::endl 
        << " # | display  | Type      | " << std::endl
        << " 0 | primary  | OVERLAY   | " << std::endl
        << " 1 | primary  | GL_RENDER | " << std::endl
        << " 2 | primary  | FORCE_GL  | " << std::endl
        << " 3 | primary  | FB_TARGET | " << std::endl
        << " 0 | external | OVERLAY   | " << std::endl
        << " 1 | external | GL_RENDER | " << std::endl
        << " 2 | external | FORCE_GL  | " << std::endl
        << " 3 | external | FB_TARGET | " << std::endl;
    mga::HwcFormattedLogger logger;
    logger.report_prepare_done(display_list);
    EXPECT_EQ(str.str(), test_stream.str()); 
}

TEST_F(HwcLogger, report_set)
{
    std::stringstream str;
    str << "set list():" << std::endl
        << " # | display  | Type      | handle | acquireFenceFd" << std::endl
        << " 0 | primary  | OVERLAY   | " << &native_handle1 << " | " << fake_fence[0] << std::endl
        << " 1 | primary  | GL_RENDER | " << &native_handle2 << " | " << fake_fence[2] << std::endl
        << " 2 | primary  | FORCE_GL  | " << &native_handle3 << " | " << fake_fence[4] << std::endl
        << " 3 | primary  | FB_TARGET | " << &native_handle4 << " | " << fake_fence[6] << std::endl
        << " 0 | external | OVERLAY   | " << &native_handle1 << " | " << fake_fence[0] << std::endl
        << " 1 | external | GL_RENDER | " << &native_handle2 << " | " << fake_fence[2] << std::endl
        << " 2 | external | FORCE_GL  | " << &native_handle3 << " | " << fake_fence[4] << std::endl
        << " 3 | external | FB_TARGET | " << &native_handle4 << " | " << fake_fence[6] << std::endl;

    mga::HwcFormattedLogger logger;
    logger.report_set_list(display_list);
    EXPECT_EQ(str.str(), test_stream.str()); 
}

TEST_F(HwcLogger, report_post_set)
{
    std::stringstream str;
    str << "after set():" << std::endl
        << " # | display  | releaseFenceFd" << std::endl
        << " 0 | primary  | " << fake_fence[1] << std::endl 
        << " 1 | primary  | " << fake_fence[3] << std::endl 
        << " 2 | primary  | " << fake_fence[5] << std::endl 
        << " 3 | primary  | " << fake_fence[7] << std::endl
        << " 0 | external | " << fake_fence[1] << std::endl 
        << " 1 | external | " << fake_fence[3] << std::endl 
        << " 2 | external | " << fake_fence[5] << std::endl 
        << " 3 | external | " << fake_fence[7] << std::endl; 

    mga::HwcFormattedLogger logger;
    logger.report_set_done(display_list);
    EXPECT_EQ(str.str(), test_stream.str()); 
}

TEST_F(HwcLogger, report_optimization)
{
    std::stringstream enabled_str;
    std::stringstream disabled_str;
    enabled_str << "HWC overlay optimizations are ON" << std::endl;
    disabled_str << "HWC overlay optimizations are OFF" << std::endl;

    mga::HwcFormattedLogger logger;
    logger.report_overlay_optimization(mga::OverlayOptimization::enabled);
    EXPECT_EQ(enabled_str.str(), test_stream.str()); 
    test_stream.str("");
    test_stream.clear();
    logger.report_overlay_optimization(mga::OverlayOptimization::disabled);
    EXPECT_EQ(disabled_str.str(), test_stream.str()); 
}

TEST_F(HwcLogger, report_vsync_on)
{
    std::stringstream str;
    str << "HWC: vsync signal on" << std::endl;

    mga::HwcFormattedLogger logger;
    logger.report_vsync_on();
    EXPECT_EQ(str.str(), test_stream.str()); 
}

TEST_F(HwcLogger, report_vsync_off)
{
    std::stringstream str;
    str << "HWC: vsync signal off" << std::endl;

    mga::HwcFormattedLogger logger;
    logger.report_vsync_off();
    EXPECT_EQ(str.str(), test_stream.str()); 
}

TEST_F(HwcLogger, report_display_off)
{
    std::stringstream str;
    str << "HWC: display off" << std::endl;

    mga::HwcFormattedLogger logger;
    logger.report_display_off();
    EXPECT_EQ(str.str(), test_stream.str()); 
}

TEST_F(HwcLogger, report_display_on)
{
    std::stringstream str;
    str << "HWC: display on" << std::endl;

    mga::HwcFormattedLogger logger;
    logger.report_display_on();
    EXPECT_EQ(str.str(), test_stream.str()); 
}

TEST_F(HwcLogger, report_legacy_fb)
{
    std::stringstream str;
    str << "Legacy FB module" << std::endl;

    mga::HwcFormattedLogger logger;
    logger.report_legacy_fb_module();
    EXPECT_EQ(str.str(), test_stream.str()); 
}

TEST_F(HwcLogger, report_hwc_version)
{
    std::stringstream str;
    str << "HWC version 1.0" << std::endl
        << "HWC version 1.1" << std::endl
        << "HWC version 1.2" << std::endl
        << "HWC version 1.3" << std::endl
        << "HWC version 1.4" << std::endl;

    mga::HwcFormattedLogger logger;
    logger.report_hwc_version(mga::HwcVersion::hwc10);
    logger.report_hwc_version(mga::HwcVersion::hwc11);
    logger.report_hwc_version(mga::HwcVersion::hwc12);
    logger.report_hwc_version(mga::HwcVersion::hwc13);
    logger.report_hwc_version(mga::HwcVersion::hwc14);
    EXPECT_EQ(str.str(), test_stream.str()); 
}

TEST_F(HwcLogger, report_power_mode)
{
    std::stringstream str;
    str << "HWC: power mode: off" << std::endl
        << "HWC: power mode: doze" << std::endl
        << "HWC: power mode: doze(suspend)" << std::endl
        << "HWC: power mode: on(normal)" << std::endl;

    mga::HwcFormattedLogger logger;
    logger.report_power_mode(mga::PowerMode::off);
    logger.report_power_mode(mga::PowerMode::doze);
    logger.report_power_mode(mga::PowerMode::doze_suspend);
    logger.report_power_mode(mga::PowerMode::normal);

    EXPECT_EQ(str.str(), test_stream.str());
}
