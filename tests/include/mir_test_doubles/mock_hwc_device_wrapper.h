/*
 * Copyright © 2014 Canonical Ltd.
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

#ifndef MIR_TEST_DOUBLES_MOCK_HWC_DEVICE_WRAPPER_H_
#define MIR_TEST_DOUBLES_MOCK_HWC_DEVICE_WRAPPER_H_

#include "src/platforms/android/hwc_wrapper.h"
#include "mir_test/gmock_fixes.h"

namespace testing
{
//gmock has a hard time with nonmovable types
namespace internal
{
template<>
struct ActionResultHolder<mir::graphics::android::EventSubscription>
: UntypedActionResultHolderBase {
  explicit ActionResultHolder(mir::graphics::android::EventSubscription&& a_value) :
  value_(std::move(a_value)) {}
  mir::graphics::android::EventSubscription GetValueAndDelete() const {
      mir::graphics::android::EventSubscription retval(std::move(value_));
    delete this;
    return std::move(retval);
  }

  // Prints the held value as an action's result to os.
  virtual void PrintAsActionResult(::std::ostream* os) const {
    *os << "\n          Returns: ";
    // T may be a reference type, so we don't use UniversalPrint().
    UniversalPrinter<mir::graphics::android::EventSubscription>::Print(value_, os);
  }

  // Performs the given mock function's default action and returns the
  // result in a new-ed ActionResultHolder.
  template <typename F>
  static ActionResultHolder* PerformDefaultAction(
      const FunctionMockerBase<F>* func_mocker,
      const typename Function<F>::ArgumentTuple& args,
      const string& call_description) {
    return new ActionResultHolder(
        func_mocker->PerformDefaultAction(args, call_description));
  }

  // Performs the given action and returns the result in a new-ed
  // ActionResultHolder.
  template <typename F>
  static ActionResultHolder*
  PerformAction(const Action<F>& action,
                const typename Function<F>::ArgumentTuple& args) {
    return new ActionResultHolder(action.Perform(args));
  }

 private:
  mir::graphics::android::EventSubscription mutable value_;

  // T could be a reference type, so = isn't supported.
  GTEST_DISALLOW_ASSIGN_(ActionResultHolder);
};

}
template<>
struct DefaultValue<mir::graphics::android::EventSubscription> {
  static void Clear() {}
  static bool IsSet() { return false; }
  static bool Exists() { return true; }
  static mir::graphics::android::EventSubscription Get() {
    return std::move(mir::graphics::android::EventSubscription());
  }
};
}
namespace mir
{
namespace test
{
namespace doubles
{

struct MockHWCDeviceWrapper : public graphics::android::HwcWrapper
{
    MOCK_CONST_METHOD1(prepare, void(std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const&));
    MOCK_CONST_METHOD1(set, void(std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const&));
    MOCK_CONST_METHOD1(vsync_signal_on, void(graphics::android::DisplayName));
    MOCK_CONST_METHOD1(vsync_signal_off, void(graphics::android::DisplayName));
    MOCK_CONST_METHOD1(display_on, void(graphics::android::DisplayName));
    MOCK_CONST_METHOD1(display_off, void(graphics::android::DisplayName));
    MOCK_METHOD3(subscribe_to_events, graphics::android::EventSubscription(
        std::function<void(graphics::android::DisplayName, std::chrono::nanoseconds)> const&,
        std::function<void(graphics::android::DisplayName, bool)> const&,
        std::function<void()> const&));
};

}
}
}
#endif /* MIR_TEST_DOUBLES_MOCK_HWC_DEVICE_WRAPPER_H_ */
