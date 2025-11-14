#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <wayland-client.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "ext-input-trigger-registration-v1-client-protocol.h"
#include "ext-input-trigger-action-v1-client-protocol.h"

/*
 * Standalone Wayland client that registers a keyboard-sym trigger for
 * Ctrl+Shift+C via ext-input-trigger-registration-v1 and prints:
 *   Hey  -> on activation (begin)
 *   Bye  -> on deactivation (end)
 *
 * Note: This uses the generated protocol headers under the build-directory
 * (e.g. ${CMAKE_BINARY_DIR}/generated). The CMake change below ensures that
 * the generated headers directory is added to the include path for this target.
 */

static ext_input_trigger_registration_manager_v1* registration_manager = nullptr;
static ext_input_trigger_action_manager_v1* action_manager = nullptr;
static ext_input_trigger_v1* trigger_obj = nullptr;
static ext_input_trigger_action_v1* action_obj = nullptr;
static char const* action_control_token = nullptr;

static void registry_global(
    void* /*data*/, struct wl_registry* registry, uint32_t name, char const* interface, uint32_t version)
{
    if (strcmp(interface, "ext_input_trigger_registration_manager_v1") == 0)
    {
        registration_manager = static_cast<ext_input_trigger_registration_manager_v1*>(wl_registry_bind(
            registry, name, &ext_input_trigger_registration_manager_v1_interface, std::min<uint32_t>(version, 1)));
    }
    else if (strcmp(interface, "ext_input_trigger_action_manager_v1") == 0)
    {
        action_manager = static_cast<ext_input_trigger_action_manager_v1*>(wl_registry_bind(
            registry, name, &ext_input_trigger_action_manager_v1_interface, std::min<uint32_t>(version, 1)));
    }
}

static void registry_global_remove(void*, struct wl_registry*, uint32_t)
{
}

static wl_registry_listener const registry_listener = {registry_global, registry_global_remove};

static void trigger_done(void* /*data*/, ext_input_trigger_v1* /*trigger*/)
{
    std::cout << "Trigger registration done\n" << std::flush;
}

static void trigger_failed(void* /*data*/, ext_input_trigger_v1* /*trigger*/)
{
    std::cerr << "Trigger registration failed\n" << std::flush;
    std::exit(EXIT_FAILURE);
}

static ext_input_trigger_v1_listener const trigger_listener = {.done = trigger_done, .failed = trigger_failed};

static void action_begin(
    void* /*data*/, ext_input_trigger_action_v1* /*action*/, uint32_t /*serial*/, char const* /*token*/)
{
    std::cout << "Hey\n" << std::flush;
}

static void action_update(
    void* /*data*/,
    ext_input_trigger_action_v1* /*action*/,
    uint32_t /*time*/,
    char const* /*message*/,
    wl_fixed_t /*progress*/)
{
    // unused
}

static void action_end(
    void* /*data*/, ext_input_trigger_action_v1* /*action*/, uint32_t /*serial*/, char const* /*message*/)
{
    std::cout << "Bye\n" << std::flush;
}

static void action_unavailable(void* /*data*/, ext_input_trigger_action_v1* /*action*/)
{
    std::cerr << "Action unavailable\n" << std::flush;
}

static ext_input_trigger_action_v1_listener const action_listener = {
    .begin = action_begin, .update = action_update, .end = action_end, .unavailable = action_unavailable};

int main(int /*argc*/, char** /*argv*/)
{
    wl_display* display = wl_display_connect(nullptr);
    if (!display)
    {
        std::fprintf(stderr, "Failed to connect to Wayland display\n");
        return EXIT_FAILURE;
    }

    wl_registry* registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, nullptr);

    // Roundtrip to get globals
    wl_display_roundtrip(display);

    if (!registration_manager)
    {
        std::cerr << "ext_input_trigger_registration_manager_v1 not available\n";
        wl_display_disconnect(display);
        return EXIT_FAILURE;
    }

    if (!action_manager)
    {
        std::cerr << "ext_input_trigger_action_manager_v1 not available\n";
        wl_display_disconnect(display);
        return EXIT_FAILURE;
    }

    // Register keyboard sym trigger: Ctrl + Shift + C
    uint32_t modifiers = EXT_INPUT_TRIGGER_REGISTRATION_MANAGER_V1_MODIFIERS_SHIFT |
                         EXT_INPUT_TRIGGER_REGISTRATION_MANAGER_V1_MODIFIERS_CTRL;
    trigger_obj = ext_input_trigger_registration_manager_v1_register_keyboard_sym_trigger(
        registration_manager, modifiers, XKB_KEY_C);

    ext_input_trigger_v1_add_listener(trigger_obj, &trigger_listener, nullptr);

    // Get an action control for a chosen name (some compositors will return a token
    // in action_control.done; robust clients should listen for that token).
    char const* action_name = "ctrl_shift_c";
    ext_input_trigger_action_control_v1* action_control =
        ext_input_trigger_registration_manager_v1_get_action_control(registration_manager, action_name);

    // Listener to capture a token returned via action_control.done
    ext_input_trigger_action_control_v1_listener const action_control_listener = {
        .done =
        [](void* /*data*/, ext_input_trigger_action_control_v1* /*control*/, char const* token)
        {
            action_control_token = strdup(token);
        },
    };
    ext_input_trigger_action_control_v1_add_listener(action_control, &action_control_listener, nullptr);

    // Map trigger to the action control
    ext_input_trigger_action_control_v1_add_input_trigger_event(action_control, trigger_obj);

    // Ensure the registration is processed
    wl_display_roundtrip(display);

    // Get the action object. Some compositors may require waiting for a token; the
    // robust approach is to wait for action_control.done and use the token it returns.
    ext_input_trigger_action_v1* action =
        ext_input_trigger_action_manager_v1_get_input_trigger_action(action_manager, action_control_token);

    if (action)
    {
        ext_input_trigger_action_v1_add_listener(action, &action_listener, nullptr);
        action_obj = action;
    }
    else
    {
        std::cerr << "Failed to get action from action_manager (server might require token-based flow)\n";
    }

    // Enter the dispatch loop
    while (wl_display_dispatch(display) != -1)
    {
    }

    if (action_obj)
        ext_input_trigger_action_v1_destroy(action_obj);
    if (trigger_obj)
        ext_input_trigger_v1_destroy(trigger_obj);
    if (registration_manager)
        ext_input_trigger_registration_manager_v1_destroy(registration_manager);
    if (action_manager)
        ext_input_trigger_action_manager_v1_destroy(action_manager);

    wl_registry_destroy(registry);
    wl_display_disconnect(display);

    return 0;
}
