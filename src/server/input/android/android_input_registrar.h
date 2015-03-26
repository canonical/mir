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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_INPUT_ANDROID_REGISTRAR_H_
#define MIR_INPUT_ANDROID_REGISTRAR_H_

#include "android_window_handle_repository.h"

#include "mir/scene/null_observer.h"

#include <utils/StrongPointer.h>

#include <map>
#include <memory>
#include <mutex>

namespace android
{
class InputDispatcherInterface;
class InputWindowHandle;
}

namespace droidinput = android;

namespace mir
{
namespace compositor
{
class Scene;
}
namespace scene
{
class Surface;
}
namespace input
{
namespace android
{
class InputConfiguration;
class InputTargeter;

class InputRegistrar : public WindowHandleRepository
{
public:
    explicit InputRegistrar(std::shared_ptr<mir::compositor::Scene> const& scene);
    virtual ~InputRegistrar() noexcept(true);

    virtual droidinput::sp<droidinput::InputWindowHandle> handle_for_channel(std::shared_ptr<input::InputChannel const> const& channel);

    void set_dispatcher(std::shared_ptr<droidinput::InputDispatcherInterface> const& dispatcher);
    void add_window_handle_for_surface(mir::scene::Surface *);
    void remove_window_handle_for_surface(mir::scene::Surface *);
private:
    struct SceneObserver : mir::scene::NullObserver
    {
        typedef std::function<void(mir::scene::Surface*)> SurfaceCallback;
        SceneObserver(SurfaceCallback const& add, SurfaceCallback const& remove);
        void surface_added(scene::Surface* surface) override;
        void surface_removed(scene::Surface* surface) override;
        void surface_exists(scene::Surface* surface) override;
        void scene_changed() override;

        SurfaceCallback add, remove;
    };
    // weak ptr as input dispatcher already has a strong references to the registrar through the mir::input::android::InputTargetEnumerator
    std::weak_ptr<droidinput::InputDispatcherInterface> input_dispatcher;

    std::map<std::shared_ptr<input::InputChannel const>, droidinput::sp<droidinput::InputWindowHandle>> window_handles;

    std::mutex handles_mutex;
    std::shared_ptr<compositor::Scene> const scene;
    std::shared_ptr<SceneObserver> const observer;
};

}
}
} // namespace mir

#endif // MIR_INPUT_ANDROID_REGISTRAR_H_
