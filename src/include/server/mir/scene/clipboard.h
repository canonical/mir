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
 */

#ifndef MIR_SCENE_CLIPBOARD_H_
#define MIR_SCENE_CLIPBOARD_H_

#include "mir/observer_registrar.h"
#include "mir/fd.h"

#include <memory>
#include <vector>

namespace mir
{
namespace scene
{
/// Interface representing a data source (such as an offer by a client to transfer data). Implemented by the
/// frontend. Implementations must be threadsafe.
class ClipboardSource
{
public:
    ClipboardSource() = default;
    virtual ~ClipboardSource() = default;

    /// Returns the list of supported mime types.
    virtual auto mime_types() const -> std::vector<std::string> const& = 0;

    /// Instructs the source to start sending it's data in the given mime type to the given fd.
    virtual void initiate_send(std::string const& mime_type, Fd const& target_fd) = 0;

private:
    ClipboardSource(ClipboardSource const&) = delete;
    ClipboardSource& operator=(ClipboardSource const&) = delete;
};

/// Allows the frontend to listen for changes in the clipboard state
class ClipboardObserver
{
public:
    ClipboardObserver() = default;
    virtual ~ClipboardObserver() = default;

    /// The copy-paste data source has been set or cleared. If cleared, source is null. Note that the clipboard's mutex
    /// does not remain locked when notifying observers, so clipboard->paste_source() may return a different value than
    /// source.
    virtual void paste_source_set(std::shared_ptr<ClipboardSource> const& source) = 0;

private:
    ClipboardObserver(ClipboardObserver const&) = delete;
    ClipboardObserver& operator=(ClipboardObserver const&) = delete;
};

/// Interface for the global object that manages data transfers between clients (such as copy-paste)
class Clipboard : public ObserverRegistrar<ClipboardObserver>
{
public:
    /// Get the current copy-paste source.
    virtual auto paste_source() const -> std::shared_ptr<ClipboardSource> = 0;

    /// Sets the given source to be the current copy-paste source for all clients.
    virtual void set_paste_source(std::shared_ptr<ClipboardSource> const& source) = 0;

    /// Clears the current copy-paste source
    virtual void clear_paste_source() = 0;

    /// Clears the current copy-paste source ONLY if it is the same as the given source, otherwise does nothing.
    virtual void clear_paste_source(ClipboardSource const& source) = 0;
};
}
}

#endif // MIR_SCENE_CLIPBOARD_H_
