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

#include "mir/input/dispatcher.h"
#include "mir/input/event.h"
#include "mir/input/event_handler.h"
#include "mir/input/filter.h"

#include <cassert>

namespace mi = mir::input;

mi::Dispatcher::Dispatcher(TimeSource* time_source,
                           mi::Filter* shell_filter,
                           mi::Filter* grab_filter,
                           mi::Filter* application_filter)
        : time_source(time_source),
          shell_filter(shell_filter),
          grab_filter(grab_filter),
          application_filter(application_filter)
{
    assert(time_source);
    assert(shell_filter);
    assert(grab_filter);
    assert(application_filter);
}

// Implemented from EventHandler
void mi::Dispatcher::OnEvent(mi::Event* e)
{
    if (!e)
        return;

    e->SetSystemTimestamp(time_source->Sample());
    
    if (shell_filter->Accept(e))
        return;

    if (grab_filter->Accept(e))
        return;

    application_filter->Accept(e);
}

void mi::Dispatcher::RegisterShellFilter(mi::Filter* f)
{
    assert(f);
    shell_filter = f;
}

void mi::Dispatcher::RegisterGrabFilter(mi::Filter* f)
{
    assert(f);
    grab_filter = f;
}

void mi::Dispatcher::RegisterApplicationFilter(mi::Filter* f)
{
    assert(f);
    application_filter = f;
}
