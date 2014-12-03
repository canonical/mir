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

#include "src/platform/graphics/android/hwc_loggers.h"
#include <memory>
#include <iostream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mga=mir::graphics::android;

namespace
{
struct HwcLogger : public ::testing::Test
{
    HwcLogger()
     : num_layers{4},
       display_list{std::shared_ptr<hwc_display_contents_1_t>(
            static_cast<hwc_display_contents_1_t*>(
           ::operator new(sizeof(hwc_display_contents_1_t) + (num_layers * sizeof(hwc_layer_1_t)))))}
    {
        default_cout_buffer = std::cout.rdbuf();
        std::cout.rdbuf(test_stream.rdbuf());

        display_list->numHwLayers = num_layers;

        display_list->hwLayers[0].compositionType = HWC_OVERLAY;
        display_list->hwLayers[0].flags = 0; 
        display_list->hwLayers[0].handle = &native_handle1; 
        display_list->hwLayers[0].transform = HWC_TRANSFORM_ROT_90;
        display_list->hwLayers[0].blending = HWC_BLENDING_NONE;
        display_list->hwLayers[0].displayFrame = {1, 1, 2, 1}; 
        display_list->hwLayers[0].sourceCrop = {3, 2, 5, 3}; 

        display_list->hwLayers[1].compositionType = HWC_FRAMEBUFFER; 
        display_list->hwLayers[1].flags = 0;
        display_list->hwLayers[1].handle = &native_handle2;
        display_list->hwLayers[1].transform = HWC_TRANSFORM_ROT_180;
        display_list->hwLayers[1].blending = HWC_BLENDING_PREMULT;
        display_list->hwLayers[1].displayFrame = {8, 5, 13, 8}; 
        display_list->hwLayers[1].sourceCrop = {21, 13, 34, 21}; 

        display_list->hwLayers[2].compositionType = HWC_FRAMEBUFFER; 
        display_list->hwLayers[2].flags = HWC_SKIP_LAYER;
        display_list->hwLayers[2].handle = &native_handle3; 
        display_list->hwLayers[2].transform = HWC_TRANSFORM_ROT_270;
        display_list->hwLayers[2].blending = HWC_BLENDING_COVERAGE; 
        display_list->hwLayers[2].displayFrame = {55, 34, 89, 55};  
        display_list->hwLayers[2].sourceCrop = {144, 89, 233, 144}; 

        display_list->hwLayers[3].compositionType = HWC_FRAMEBUFFER_TARGET; 
        display_list->hwLayers[3].flags = 0; 
        display_list->hwLayers[3].handle = &native_handle4; 
        display_list->hwLayers[3].transform = 0;
        display_list->hwLayers[3].blending = HWC_BLENDING_NONE; 
        display_list->hwLayers[3].displayFrame = {377, 233, 610, 337}; 
        display_list->hwLayers[3].sourceCrop = {987, 610, 1597, 987};  
    };

    virtual ~HwcLogger()
    {
        std::cout.rdbuf(default_cout_buffer);
    }

    decltype(std::cout.rdbuf()) default_cout_buffer;
    std::ostringstream test_stream;
    size_t const num_layers;
    std::shared_ptr<hwc_display_contents_1_t> const display_list;
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
        << " # | pos {l,t,r,b}         | crop {l,t,r,b}        | transform | blending | " << std::endl
        << " 0 | {   1,   1,   2,   1} | {   3,   2,   5,   3} | ROT_90    | NONE     | " << std::endl
        << " 1 | {   8,   5,  13,   8} | {  21,  13,  34,  21} | ROT_180   | PREMULT  | " << std::endl
        << " 2 | {  55,  34,  89,  55} | { 144,  89, 233, 144} | ROT_270   | COVERAGE | " << std::endl
        << " 3 | { 377, 233, 610, 337} | { 987, 610,1597, 987} | NONE      | NONE     | " << std::endl;
    mga::HwcFormattedLogger logger;
    logger.report_list_submitted_to_prepare(*display_list);
    EXPECT_EQ(str.str(), test_stream.str()); 
}

TEST_F(HwcLogger, report_post_prepare)
{
    std::stringstream str;
    str << "after prepare():" << std::endl 
        << " # | Type      | " << std::endl
        << " 0 | OVERLAY   | " << std::endl
        << " 1 | GL_RENDER | " << std::endl
        << " 2 | FORCE_GL  | " << std::endl
        << " 3 | FB_TARGET | " << std::endl;
    mga::HwcFormattedLogger logger;
    logger.report_prepare_done(*display_list);
    EXPECT_EQ(str.str(), test_stream.str()); 
}

TEST_F(HwcLogger, report_set)
{
    std::stringstream str;
    str << "set list():" << std::endl
        << " # | handle" << std::endl
        << " 0 | " << &native_handle1 << std::endl 
        << " 1 | " << &native_handle2 << std::endl 
        << " 2 | " << &native_handle3 << std::endl 
        << " 3 | " << &native_handle4 << std::endl; 

    mga::HwcFormattedLogger logger;
    logger.report_set_list(*display_list);
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
        << "HWC version unknown (0x33)" << std::endl;

    mga::HwcFormattedLogger logger;
    logger.report_hwc_version(HWC_DEVICE_API_VERSION_1_0);
    logger.report_hwc_version(HWC_DEVICE_API_VERSION_1_1);
    logger.report_hwc_version(HWC_DEVICE_API_VERSION_1_2);
    logger.report_hwc_version(HWC_DEVICE_API_VERSION_1_3);
    logger.report_hwc_version(51);
    EXPECT_EQ(str.str(), test_stream.str()); 
}
