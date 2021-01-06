/*
 * Copyright © 2021 Canonical Ltd.
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
 *
 * Authored By: William Wold <william.wold@canonical.com>
 */

#include "basic_clipboard.h"

#include "mir/fatal.h"

namespace ms = mir::scene;

auto ms::BasicClipboard::paste_source() const -> std::shared_ptr<ClipboardSource>
{
    std::lock_guard<std::mutex> lock{paste_mutex};
    return paste_source_;
}

void ms::BasicClipboard::set_paste_source(std::shared_ptr<ClipboardSource> const& source)
{
    if (!source)
    {
        fatal_error("BasicClipboard::set_paste_source(nullptr)");
    }
    bool notify{false};
    {
        std::lock_guard<std::mutex> lock{paste_mutex};
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

void ms::BasicClipboard::clear_paste_source(ClipboardSource const& source)
{
    bool notify{false};
    {
        std::lock_guard<std::mutex> lock{paste_mutex};
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
