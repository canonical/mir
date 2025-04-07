# Mousekeys API

## Introduction
To facilitate modularity and customizability, mousekeys support has been wrapped
in `miral::MouseKeysConfig`. ***Mousekeys are not enabled by default***.
Passing an object of this type to the runner will enable mousekeys and add
configuration options to control various aspects of mousekeys.

This object also provides a programmatic interface to customize mousekeys. This
can be useful if you wish to configure mousekeys at runtime via a GUI or a
configuration file.

## API examples

Note: all methods defined on `MouseKeysConfig` require the server to be started
to apply their changes.

### Enabling or Disabling MouseKeys at Runtime
You can enable or disable mousekeys at runtime by calling
`MouseKeysConfig::enabled(<boolean value>)`.

### Setting The Maximum Pointer Speed
To control the maximum speed on the X and Y axes, you can call
`MouseKeysConfig::set_max_speed(<x axis max speed>, <y axis max speed> )`.

### Setting a Custom Keymap
Setting a custom mousekeys keymap is a bit more involved. You first have to
create a `mir::input::MouseKeysKeymap` object, which can be done as follows:
```cpp
mir::input::MouseKeysKeymap keymap{
    {XKB_KEY_w, mir::input::MouseKeysKeymap::Action::move_up},
    {XKB_KEY_s, mir::input::MouseKeysKeymap::Action::move_down},
    {XKB_KEY_a, mir::input::MouseKeysKeymap::Action::move_left},
    {XKB_KEY_d, mir::input::MouseKeysKeymap::Action::move_right},
};
```

or, if you want to incrementally build this map:
```cpp
mir::input::MouseKeysKeymap keymap;
keymap.set_action(XKB_KEY_w, mir::input::MouseKeysKeymap::Action::move_up);
keymap.set_action(XKB_KEY_s, mir::input::MouseKeysKeymap::Action::move_down);
keymap.set_action(XKB_KEY_a, mir::input::MouseKeysKeymap::Action::move_left);
keymap.set_action(XKB_KEY_d, mir::input::MouseKeysKeymap::Action::move_right);
```

Note that `mir::input::MouseKeysKeymap::set_action` accepts an optional as the
action, allowing you to pass `std::nullopt` to clear a previously set
key-action mapping.

Once you have your keymap built up, you can set it via
`MouseKeysConfig::set_keymap(<keymap>)`.
