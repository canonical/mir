/*
 * Copyright © 2012 Canonical Ltd.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Canonical Ltd. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Canonical Ltd. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * CANONICAL, LTD. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL CANONICAL, LTD. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/input/device.h"
#include "mir/input/dispatcher.h"
#include "mir/input/filter.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mi = mir::input;

namespace
{

class mock_filter : public mi::filter
{
public:
  MOCK_METHOD1(accept, bool(mi::event*));
};

class mock_input_device : public mi::device
{
public:
  mock_input_device(mi::event_handler* h) : mi::device(h)
  {
  }

  void trigger_event()
  {
    handler->on_event(nullptr);
  }
};
}

TEST(input_dispatch, incoming_input_triggers_filter)
{
  using namespace testing;
  mi::dispatcher dispatcher;

  mock_filter filter;
  dispatcher.register_filter(&filter);

  mock_input_device device(&dispatcher);

  EXPECT_CALL(filter, accept(_)).Times(AtLeast(1));

  device.trigger_event();
}
