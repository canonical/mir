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

#ifndef MIR_SCENE_BASIC_CLIPBOARD_H_
#define MIR_SCENE_BASIC_CLIPBOARD_H_

#include "mir/scene/clipboard.h"
#include "mir/observer_multiplexer.h"

#include <mutex>

namespace mir
{
namespace scene
{
class ClipboardObserverMultiplexer: public ObserverMultiplexer<ClipboardObserver>
{
public:
    ClipboardObserverMultiplexer()
        : ObserverMultiplexer{immediate_executor}
    {
    }

    void paste_source_set(std::shared_ptr<DataExchangeSource> const& source) override
    {
        for_each_observer(&ClipboardObserver::paste_source_set, source);
    }

    void drag_n_drop_source_set(std::shared_ptr<DataExchangeSource> const& source) override
    {
        for_each_observer(&ClipboardObserver::drag_n_drop_source_set, source);
    }

    void drag_n_drop_source_cleared(std::shared_ptr<DataExchangeSource> const& source) override
    {
        for_each_observer(&ClipboardObserver::drag_n_drop_source_cleared, source);
    }

    void end_of_dnd_gesture() override
    {
        for_each_observer(&ClipboardObserver::end_of_dnd_gesture);
    }
};

/// Interface for the global object that manages data transfers between clients (such as copy-paste)
class BasicClipboard : public Clipboard
{
public:
    auto paste_source() const -> std::shared_ptr<DataExchangeSource> override;
    void set_paste_source(std::shared_ptr<DataExchangeSource> const& source) override;
    void clear_paste_source() override;
    void clear_paste_source(DataExchangeSource const& source) override;
    void set_drag_n_drop_source(std::shared_ptr<DataExchangeSource> const& source) override;

    void clear_drag_n_drop_source(std::shared_ptr<DataExchangeSource> const& source) override;
    void end_of_dnd_gesture() override;

    /// Implement ObserverRegistrar<ClipboardObserver>
    /// @{
    void register_interest(std::weak_ptr<ClipboardObserver> const& observer) override
    {
        multiplexer.register_interest(observer);
        observer.lock()->paste_source_set(paste_source_);
    }
    void register_interest(std::weak_ptr<ClipboardObserver> const& observer, Executor& executor) override
    {
        multiplexer.register_interest(observer, executor);
        observer.lock()->paste_source_set(paste_source_);
    }
    void register_early_observer(std::weak_ptr<ClipboardObserver> const& observer, Executor& executor) override
    {
        multiplexer.register_early_observer(observer, executor);
        observer.lock()->paste_source_set(paste_source_);
    }
    void unregister_interest(ClipboardObserver const& observer) override
    {
        multiplexer.unregister_interest(observer);
    }
    /// @}

private:
    ClipboardObserverMultiplexer multiplexer;

    std::mutex mutable paste_mutex;
    std::shared_ptr<DataExchangeSource> paste_source_; ///< Can be null
    std::shared_ptr<DataExchangeSource> dnd_source_; ///< Can be null
};
}
}

#endif // MIR_SCENE_BASIC_CLIPBOARD_H_
