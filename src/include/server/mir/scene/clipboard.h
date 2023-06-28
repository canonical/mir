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

#ifndef MIR_SCENE_CLIPBOARD_H_
#define MIR_SCENE_CLIPBOARD_H_

#include "mir/scene/data_exchange.h"

#include "mir/observer_registrar.h"

#include <memory>

namespace mir
{
namespace scene
{

/// Allows the frontend to listen for changes in the clipboard state
class ClipboardObserver
{
public:
    ClipboardObserver() = default;
    virtual ~ClipboardObserver() = default;

    /// The copy-paste data source has been set or cleared. If cleared, source is null. Note that the clipboard's mutex
    /// does not remain locked when notifying observers, so clipboard->paste_source() may return a different value than
    /// source.
    virtual void paste_source_set(std::shared_ptr<DataExchangeSource> const& source) = 0;
    virtual void drag_n_drop_source_set(std::shared_ptr<DataExchangeSource> const& source) = 0;
    virtual void drag_n_drop_source_cleared(std::shared_ptr<DataExchangeSource> const& source) = 0;

private:
    ClipboardObserver(ClipboardObserver const&) = delete;
    ClipboardObserver& operator=(ClipboardObserver const&) = delete;
};

/// Interface for the global object that manages data transfers between clients (such as copy-paste)
class Clipboard : public ObserverRegistrar<ClipboardObserver>
{
public:
    /// Get the current copy-paste source.
    virtual auto paste_source() const -> std::shared_ptr<DataExchangeSource> = 0;

    /// Sets the given source to be the current copy-paste source for all clients.
    virtual void set_paste_source(std::shared_ptr<DataExchangeSource> const& source) = 0;

    /// Clears the current copy-paste source
    virtual void clear_paste_source() = 0;

    /// Clears the current copy-paste source ONLY if it is the same as the given source, otherwise does nothing.
    virtual void clear_paste_source(DataExchangeSource const& source) = 0;

    /// Sets the given source to be the current drag & drop source for all clients.
    virtual void set_drag_n_drop_source(std::shared_ptr<DataExchangeSource> const& source) = 0;
    /// Clears the current drag-n-drop source ONLY if it is the same as the given source, otherwise does nothing.
    virtual void clear_drag_n_drop_source(std::shared_ptr<DataExchangeSource> const& source) = 0;
};
}
}

#endif // MIR_SCENE_CLIPBOARD_H_
