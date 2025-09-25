/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "basic_clipboard.h"

#include "mir/fatal.h"

namespace ms = mir::scene;

auto ms::BasicClipboard::paste_source() const -> std::shared_ptr<DataExchangeSource>
{
    std::lock_guard lock{paste_mutex};
    return paste_source_;
}

void ms::BasicClipboard::set_paste_source(std::shared_ptr<DataExchangeSource> const& source)
{
    if (!source)
    {
        fatal_error("BasicClipboard::set_paste_source(nullptr)");
    }
    bool notify{false};
    {
        std::lock_guard lock{paste_mutex};
        if (paste_source_ != source)
        {
            notify = true;
            paste_source_ = source;
        }
    }
    if (notify)
    {
        multiplexer.paste_source_set(source);
    }
}

void ms::BasicClipboard::clear_paste_source()
{
    bool notify{false};
    {
        std::lock_guard lock{paste_mutex};
        if (paste_source_)
        {
            notify = true;
            paste_source_ = nullptr;
        }
    }
    if (notify)
    {
        multiplexer.paste_source_set(nullptr);
    }
}

void ms::BasicClipboard::clear_paste_source(DataExchangeSource const& source)
{
    bool notify{false};
    {
        std::lock_guard lock{paste_mutex};
        if (paste_source_.get() == &source)
        {
            notify = true;
            paste_source_ = nullptr;
        }
    }
    if (notify)
    {
        multiplexer.paste_source_set(nullptr);
    }
}


void mir::scene::BasicClipboard::set_drag_n_drop_source(std::shared_ptr<DataExchangeSource> const& source)
{
    if (!source)
    {
        fatal_error("BasicClipboard::start_drag_n_drop_gesture(nullptr)");
    }
    bool notify{false};
    {
        std::lock_guard lock{paste_mutex};
        if (dnd_source_ != source)
        {
            notify = true;
            dnd_source_ = source;
        }
    }
    if (notify)
    {
        multiplexer.drag_n_drop_source_set(source);
    }
}

void mir::scene::BasicClipboard::clear_drag_n_drop_source(std::shared_ptr<DataExchangeSource> const& source)
{
    if (!source)
    {
        fatal_error("BasicClipboard::clear_drag_n_drop_source(nullptr)");
    }
    bool notify{false};
    {
        std::lock_guard lock{paste_mutex};
        if (dnd_source_ == source)
        {
            notify = true;
            dnd_source_.reset();
        }
    }
    if (notify)
    {
        multiplexer.drag_n_drop_source_cleared(source);
    }
}

void mir::scene::BasicClipboard::end_of_dnd_gesture()
{
    multiplexer.end_of_dnd_gesture();
}
