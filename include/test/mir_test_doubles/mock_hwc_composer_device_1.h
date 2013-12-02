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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_HWC_COMPOSER_DEVICE_1_H_
#define MIR_TEST_DOUBLES_MOCK_HWC_COMPOSER_DEVICE_1_H_

#include <hardware/hwcomposer.h>
#include <gmock/gmock.h>
#include <vector>
#include <memory>

namespace mir
{
namespace test
{
namespace doubles
{

class MockHWCComposerDevice1 : public hwc_composer_device_1
{
public:
    MockHWCComposerDevice1()
    {
        using namespace testing;

        common.version = HWC_DEVICE_API_VERSION_1_1;

        registerProcs = hook_registerProcs;
        eventControl = hook_eventControl;
        set = hook_set;
        prepare = hook_prepare;
        blank = hook_blank;
        getDisplayConfigs = hook_getDisplayConfigs;
        getDisplayAttributes = hook_getDisplayAttributes;

        ON_CALL(*this, set_interface(_,_,_))
            .WillByDefault(Invoke(this, &MockHWCComposerDevice1::save_last_set_arguments));
        ON_CALL(*this, prepare_interface(_,_,_))
            .WillByDefault(Invoke(this, &MockHWCComposerDevice1::save_last_prepare_arguments));
    }

    int save_args(hwc_display_contents_1_t* out, hwc_display_contents_1_t** in)
    {
        if ((nullptr == in) || (nullptr == *in))
            return -1;

        hwc_display_contents_1_t* primary_display = *in;
        memcpy(out, primary_display, sizeof(hwc_display_contents_1_t));
        return 0;
    }

    void hwc_set_return_fence(int fence)
    {
        fb_fence = fence;
    }

    int save_last_prepare_arguments(struct hwc_composer_device_1 *, size_t, hwc_display_contents_1_t** displays)
    {
        return save_args(&display0_prepare_content, displays);
    }

    int save_last_set_arguments(
        struct hwc_composer_device_1 *, size_t, hwc_display_contents_1_t** displays)
    {
        hwc_display_contents_1_t* primary_display = *displays;
        if (!primary_display)
            return -1;

        for(auto i = 0u; i < primary_display->numHwLayers; i++)
        {
            set_layerlist.push_back(primary_display->hwLayers[i]);
            set_layerlist.back().visibleRegionScreen = {0, nullptr};
        }

        if (primary_display->numHwLayers >= 2)
        {
            primary_display->hwLayers[1].releaseFenceFd = fb_fence;
        }

        return save_args(&display0_set_content, displays);
    }

    static void hook_registerProcs(struct hwc_composer_device_1* mock_hwc, hwc_procs_t const* procs)
    {
        MockHWCComposerDevice1* mocker = static_cast<MockHWCComposerDevice1*>(mock_hwc);
        return mocker->registerProcs_interface(mock_hwc, procs);
    }
    static int hook_eventControl(struct hwc_composer_device_1* mock_hwc, int disp, int event, int enabled)
    {
        MockHWCComposerDevice1* mocker = static_cast<MockHWCComposerDevice1*>(mock_hwc);
        return mocker->eventControl_interface(mock_hwc, disp, event, enabled);
    }
    static int hook_set(struct hwc_composer_device_1 *mock_hwc, size_t numDisplays, hwc_display_contents_1_t** displays)
    {
        MockHWCComposerDevice1* mocker = static_cast<MockHWCComposerDevice1*>(mock_hwc);
        return mocker->set_interface(mock_hwc, numDisplays, displays);
    }
    static int hook_prepare(struct hwc_composer_device_1 *mock_hwc, size_t numDisplays, hwc_display_contents_1_t** displays)
    {
        MockHWCComposerDevice1* mocker = static_cast<MockHWCComposerDevice1*>(mock_hwc);
        return mocker->prepare_interface(mock_hwc, numDisplays, displays);
    }
    static int hook_blank(struct hwc_composer_device_1 *mock_hwc, int disp, int blank)
    {
        MockHWCComposerDevice1* mocker = static_cast<MockHWCComposerDevice1*>(mock_hwc);
        return mocker->blank_interface(mock_hwc, disp, blank);
    }

    static int hook_getDisplayConfigs(struct hwc_composer_device_1* mock_hwc, int disp, uint32_t* configs, size_t* numConfigs)
    {
        MockHWCComposerDevice1* mocker = static_cast<MockHWCComposerDevice1*>(mock_hwc);
        return mocker->getDisplayConfigs_interface(mock_hwc, disp, configs, numConfigs);
    }

    static int hook_getDisplayAttributes(struct hwc_composer_device_1* mock_hwc, int disp, uint32_t config, const uint32_t* attributes, int32_t* values)
    {
        MockHWCComposerDevice1* mocker = static_cast<MockHWCComposerDevice1*>(mock_hwc);
        return mocker->getDisplayAttributes_interface(mock_hwc, disp, config, attributes, values);
    }

    MOCK_METHOD2(registerProcs_interface, void(struct hwc_composer_device_1*, hwc_procs_t const*));
    MOCK_METHOD4(eventControl_interface, int(struct hwc_composer_device_1* dev, int disp, int event, int enabled));
    MOCK_METHOD3(set_interface, int(struct hwc_composer_device_1 *, size_t, hwc_display_contents_1_t**));
    MOCK_METHOD3(prepare_interface, int(struct hwc_composer_device_1 *, size_t, hwc_display_contents_1_t**));
    MOCK_METHOD3(blank_interface, int(struct hwc_composer_device_1 *, int, int));
    MOCK_METHOD4(getDisplayConfigs_interface, int(struct hwc_composer_device_1*, int, uint32_t*, size_t*));
    MOCK_METHOD5(getDisplayAttributes_interface, int(struct hwc_composer_device_1*, int, uint32_t, const uint32_t*, int32_t*));

    hwc_display_contents_1_t display0_set_content;
    std::vector<hwc_layer_1> set_layerlist;
    hwc_display_contents_1_t display0_prepare_content;
    int fb_fence;
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_HWC_COMPOSER_DEVICE_1_H_ */
