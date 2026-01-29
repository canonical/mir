#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include <wayland-client.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "ext-input-trigger-action-v1-client-protocol.h"
#include "ext-input-trigger-registration-v1-client-protocol.h"

/*
 * Standalone Wayland client that registers:
 * 1. A keyboard-sym trigger for Ctrl+Shift+C
 * 2. A keyboard-sym trigger for Alt+X
 * 3. A keyboard-code trigger for Alt+Z (scancode 44)
 *
 * It prints specific messages for begin/end events for each trigger.
 */

namespace
{
ext_input_trigger_registration_manager_v1* registration_manager = nullptr;
ext_input_trigger_action_manager_v1* action_manager = nullptr;

struct ActionContext
{
    std::string name;
    std::string begin_msg;
    std::string end_msg;
};

void registry_global(
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

void registry_global_remove(void*, struct wl_registry*, uint32_t)
{
}

void trigger_done(void* /*data*/, ext_input_trigger_v1* /*trigger*/)
{
    // std::cout << "Trigger registration done\n" << std::flush;
}

void trigger_failed(void* /*data*/, ext_input_trigger_v1* /*trigger*/)
{
    std::cerr << "Trigger registration failed\n" << std::flush;
    std::exit(EXIT_FAILURE);
}

void action_begin(
    void* data, ext_input_trigger_action_v1* /*action*/, uint32_t /*serial*/, char const* /*token*/)
{
    auto* ctx = static_cast<ActionContext*>(data);
    std::cout << ctx->begin_msg << "\n" << std::flush;
}

void action_end(
    void* data, ext_input_trigger_action_v1* /*action*/, uint32_t /*serial*/, char const* /*message*/)
{
    auto* ctx = static_cast<ActionContext*>(data);
    std::cout << ctx->end_msg << "\n" << std::flush;
}

void action_unavailable(void* /*data*/, ext_input_trigger_action_v1* /*action*/)
{
    std::cerr << "Action unavailable\n" << std::flush;
}

void control_done(void* data, ext_input_trigger_action_control_v1* /*control*/, char const* token)
{
    auto* target_str = static_cast<std::string*>(data);
    std::cerr << "Received token: " << token << '\n';
    *target_str = token;
}

wl_registry_listener const registry_listener = {
    registry_global,
    registry_global_remove,
};

ext_input_trigger_v1_listener const trigger_listener = {
    .done = trigger_done,
    .failed = trigger_failed,
};

ext_input_trigger_action_v1_listener const action_listener = {
    .begin = action_begin,
    .end = action_end,
    .unavailable = action_unavailable,
};

ext_input_trigger_action_control_v1_listener const control_listener = {
    .done = control_done,
};

void register_trigger(
    wl_display* display,
    uint32_t modifiers,
    uint32_t key,
    ActionContext* ctx,
    ext_input_trigger_v1** out_trigger,
    ext_input_trigger_action_v1** out_action)
{
    std::cout << "Registering " << ctx->name << " trigger...\n";

    *out_trigger =
        ext_input_trigger_registration_manager_v1_register_keyboard_sym_trigger(registration_manager, modifiers, key);

    ext_input_trigger_v1_add_listener(*out_trigger, &trigger_listener, nullptr);

    ext_input_trigger_action_control_v1* control =
        ext_input_trigger_registration_manager_v1_get_action_control(registration_manager, ctx->name.c_str());

    std::string token;
    ext_input_trigger_action_control_v1_add_listener(control, &control_listener, &token);

    ext_input_trigger_action_control_v1_add_input_trigger_event(control, *out_trigger);

    // Roundtrip to ensure we get the token
    wl_display_roundtrip(display);

    if (token.empty())
    {
        std::cerr << "Failed to get token for " << ctx->name << "\n";
        return;
    }

    *out_action = ext_input_trigger_action_manager_v1_get_input_trigger_action(action_manager, token.c_str());
    if (*out_action)
    {
        std::cout << "Got " << ctx->name << " action\n";
        ext_input_trigger_action_v1_add_listener(*out_action, &action_listener, ctx);
    }
    else
    {
        std::cerr << "Failed to get action for " << ctx->name << "\n";
    }

    ext_input_trigger_action_control_v1_destroy(control);
}
}

void register_keycode_trigger(
    wl_display* display,
    uint32_t modifiers,
    uint32_t scancode,
    ActionContext* ctx,
    ext_input_trigger_v1** out_trigger,
    ext_input_trigger_action_v1** out_action)
{
    std::cout << "Registering " << ctx->name << " keycode trigger (scancode " << scancode << ")...\n";

    *out_trigger = ext_input_trigger_registration_manager_v1_register_keyboard_code_trigger(
        registration_manager, modifiers, scancode);

    ext_input_trigger_v1_add_listener(*out_trigger, &trigger_listener, nullptr);

    ext_input_trigger_action_control_v1* control =
        ext_input_trigger_registration_manager_v1_get_action_control(registration_manager, ctx->name.c_str());

    std::string token;
    ext_input_trigger_action_control_v1_add_listener(control, &control_listener, &token);

    ext_input_trigger_action_control_v1_add_input_trigger_event(control, *out_trigger);

    // Roundtrip to ensure we get the token
    wl_display_roundtrip(display);

    if (token.empty())
    {
        std::cerr << "Failed to get token for " << ctx->name << "\n";
        return;
    }

    *out_action = ext_input_trigger_action_manager_v1_get_input_trigger_action(action_manager, token.c_str());
    if (*out_action)
    {
        std::cout << "Got " << ctx->name << " action\n";
        ext_input_trigger_action_v1_add_listener(*out_action, &action_listener, ctx);
    }
    else
    {
        std::cerr << "Failed to get action for " << ctx->name << "\n";
    }

    ext_input_trigger_action_control_v1_destroy(control);
}

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

    if (!registration_manager || !action_manager)
    {
        std::cerr << "Required globals not available\n";
        wl_display_disconnect(display);
        return EXIT_FAILURE;
    }

    ActionContext ctrl_shift_c_ctx{
        "CTRL + SHIFT + c (AKA CTRL + C)", "Hello from CTRL + SHIFT + c", "Bye from CTRL + SHIFT + c"};
    ext_input_trigger_v1* ctrl_shift_c_trigger = nullptr;
    ext_input_trigger_action_v1* ctrl_shift_c_action = nullptr;

    // Register keyboard sym trigger: Ctrl + Shift + C
    register_trigger(
        display,
        EXT_INPUT_TRIGGER_REGISTRATION_MANAGER_V1_MODIFIERS_SHIFT |
            EXT_INPUT_TRIGGER_REGISTRATION_MANAGER_V1_MODIFIERS_CTRL,
        XKB_KEY_C,
        &ctrl_shift_c_ctx,
        &ctrl_shift_c_trigger,
        &ctrl_shift_c_action);

    ActionContext alt_x_ctx{"ALT + x", "Hello from ALT + x", "Bye from ALT + x"};
    ext_input_trigger_v1* alt_x_trigger = nullptr;
    ext_input_trigger_action_v1* alt_x_action = nullptr;

    register_trigger(
        display,
        EXT_INPUT_TRIGGER_REGISTRATION_MANAGER_V1_MODIFIERS_ALT,
        XKB_KEY_x,
        &alt_x_ctx,
        &alt_x_trigger,
        &alt_x_action);

    // Register keyboard code trigger: Alt + Z (scancode 44)
    // This demonstrates keycode triggers work regardless of layout
    // Scancode 44 is the physical 'Z' key position on QWERTY keyboards
    ActionContext alt_z_ctx{
        "ALT + Z (scancode 44)", "Hello from ALT + Z (keycode trigger)", "Bye from ALT + Z (keycode trigger)"};
    ext_input_trigger_v1* alt_backtick_trigger = nullptr;
    ext_input_trigger_action_v1* alt_backtick_action = nullptr;

    register_keycode_trigger(
        display,
        EXT_INPUT_TRIGGER_REGISTRATION_MANAGER_V1_MODIFIERS_ALT,
        44, // scancode for Z key position
        &alt_z_ctx,
        &alt_backtick_trigger,
        &alt_backtick_action);

    std::cout << "\nAll triggers registered:\n";
    std::cout << "  - Ctrl+Shift+C (keysym trigger)\n";
    std::cout << "  - Alt+X (keysym trigger)\n";
    std::cout << "  - Alt+Z (keycode trigger - works regardless of layout)\n\n";

    // Enter the dispatch loop
    while (wl_display_dispatch(display) != -1)
    {
    }

    if (ctrl_shift_c_action)
        ext_input_trigger_action_v1_destroy(ctrl_shift_c_action);
    if (ctrl_shift_c_trigger)
        ext_input_trigger_v1_destroy(ctrl_shift_c_trigger);
    if (alt_x_action)
        ext_input_trigger_action_v1_destroy(alt_x_action);
    if (alt_x_trigger)
        ext_input_trigger_v1_destroy(alt_x_trigger);
    if (alt_backtick_action)
        ext_input_trigger_action_v1_destroy(alt_backtick_action);
    if (alt_backtick_trigger)
        ext_input_trigger_v1_destroy(alt_backtick_trigger);

    if (registration_manager)
        ext_input_trigger_registration_manager_v1_destroy(registration_manager);
    if (action_manager)
        ext_input_trigger_action_manager_v1_destroy(action_manager);

    wl_registry_destroy(registry);
    wl_display_disconnect(display);

    return 0;
}
