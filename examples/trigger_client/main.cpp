#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include <wayland-client.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "ext-input-trigger-registration-v1-client-protocol.h"
#include "ext-input-trigger-action-v1-client-protocol.h"

/*
 * Standalone Wayland client that registers:
 * 1. A keyboard-sym trigger for Ctrl+Shift+C (HOLD)
 * 2. A keyboard-sym trigger for Alt+X (TAP)
 *
 * It prints specific messages for begin/end events for each trigger.
 */

static ext_input_trigger_registration_manager_v1* registration_manager = nullptr;
static ext_input_trigger_action_manager_v1* action_manager = nullptr;

struct ActionContext {
    std::string name;
    std::string begin_msg;
    std::string end_msg;
};

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
    // std::cout << "Trigger registration done\n" << std::flush;
}

static void trigger_failed(void* /*data*/, ext_input_trigger_v1* /*trigger*/)
{
    std::cerr << "Trigger registration failed\n" << std::flush;
    std::exit(EXIT_FAILURE);
}

static ext_input_trigger_v1_listener const trigger_listener = {.done = trigger_done, .failed = trigger_failed};

static void action_begin(
    void* data, ext_input_trigger_action_v1* /*action*/, uint32_t /*serial*/, char const* /*token*/)
{
    auto* ctx = static_cast<ActionContext*>(data);
    std::cout << ctx->begin_msg << "\n" << std::flush;
}

static void action_end(
    void* data, ext_input_trigger_action_v1* /*action*/, uint32_t /*serial*/, char const* /*message*/)
{
    auto* ctx = static_cast<ActionContext*>(data);
    std::cout << ctx->end_msg << "\n" << std::flush;
}

static void action_unavailable(void* /*data*/, ext_input_trigger_action_v1* /*action*/)
{
    std::cerr << "Action unavailable\n" << std::flush;
}

static ext_input_trigger_action_v1_listener const action_listener = {
    .begin = action_begin, .end = action_end, .unavailable = action_unavailable};

static void control_done(void* data, ext_input_trigger_action_control_v1* /*control*/, char const* token)
{
    auto* target_str = static_cast<std::string*>(data);
    std::cerr << "Received token: " << token << '\n';
    *target_str = token;
}

static ext_input_trigger_action_control_v1_listener const control_listener = { .done = control_done };

void register_trigger(
    wl_display* display,
    uint32_t modifiers,
    uint32_t key,
    ActionContext* ctx,
    ext_input_trigger_v1** out_trigger,
    ext_input_trigger_action_v1** out_action)
{
    std::cout << "Registering " << ctx->name << " trigger...\n";

    *out_trigger = ext_input_trigger_registration_manager_v1_register_keyboard_sym_trigger(
        registration_manager, modifiers, key);

    ext_input_trigger_v1_add_listener(*out_trigger, &trigger_listener, nullptr);

    ext_input_trigger_action_control_v1* control =
        ext_input_trigger_registration_manager_v1_get_action_control(registration_manager, ctx->name.c_str());

    std::string token;
    ext_input_trigger_action_control_v1_add_listener(control, &control_listener, &token);

    ext_input_trigger_action_control_v1_add_input_trigger_event(control, *out_trigger);

    // Roundtrip to ensure we get the token
    wl_display_roundtrip(display);

    if (token.empty()) {
        std::cerr << "Failed to get token for " << ctx->name << "\n";
        return;
    }

    *out_action = ext_input_trigger_action_manager_v1_get_input_trigger_action(action_manager, token.c_str());
    if (*out_action) {
        std::cout << "Got " << ctx->name << " action\n";
        ext_input_trigger_action_v1_add_listener(*out_action, &action_listener, ctx);
    } else {
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

    ActionContext hold_ctx{"CTRL + SHIFT + C", "Hello from CTRL + SHIFT + C", "Bye from CTRL + SHIFT + C"};
    ext_input_trigger_v1* hold_trigger = nullptr;
    ext_input_trigger_action_v1* hold_action = nullptr;

    register_trigger(
        display,
        EXT_INPUT_TRIGGER_REGISTRATION_MANAGER_V1_MODIFIERS_SHIFT | EXT_INPUT_TRIGGER_REGISTRATION_MANAGER_V1_MODIFIERS_CTRL,
        XKB_KEY_C,
        &hold_ctx,
        &hold_trigger,
        &hold_action
    );

    ActionContext tap_ctx{"ALT + x", "Hello from ALT + x", "Bye from ALT + x"};
    ext_input_trigger_v1* tap_trigger = nullptr;
    ext_input_trigger_action_v1* tap_action = nullptr;

    register_trigger(
        display,
        EXT_INPUT_TRIGGER_REGISTRATION_MANAGER_V1_MODIFIERS_ALT,
        XKB_KEY_x,
        &tap_ctx,
        &tap_trigger,
        &tap_action
    );

    // Enter the dispatch loop
    while (wl_display_dispatch(display) != -1)
    {
    }

    if (hold_action) ext_input_trigger_action_v1_destroy(hold_action);
    if (hold_trigger) ext_input_trigger_v1_destroy(hold_trigger);
    if (tap_action) ext_input_trigger_action_v1_destroy(tap_action);
    if (tap_trigger) ext_input_trigger_v1_destroy(tap_trigger);

    if (registration_manager)
        ext_input_trigger_registration_manager_v1_destroy(registration_manager);
    if (action_manager)
        ext_input_trigger_action_manager_v1_destroy(action_manager);

    wl_registry_destroy(registry);
    wl_display_disconnect(display);

    return 0;
}
