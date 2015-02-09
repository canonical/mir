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

#ifndef MIR_TEST_DOUBLES_MOCK_HWC_LAYERLIST_H_
#define MIR_TEST_DOUBLES_MOCK_HWC_LAYERLIST_H_

#include "src/platforms/android/server/hwc_layerlist.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockHWCLayerList : public graphics::android::HWCLayerList
{
    ~MockHWCLayerList() noexcept {}
    MOCK_CONST_METHOD0(native_list, hwc_display_contents_1_t*());
    MOCK_METHOD1(set_fb_target, void(std::shared_ptr<graphics::Buffer> const&));
};

}
}
}
#endif /* MIR_TEST_DOUBLES_MOCK_HWC_LAYERLIST_H_ */
