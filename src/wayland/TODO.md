# TODO
- Generate `wayland_rs_register_global` definitions for each toplevel handler.
- Generate implementations for each handler that call into their respective C++ classes 
    - This should remove the need for "Thunks"
    - The handler should be a `friend` class of the `Global` here
- Update the poor C-casted parameters to be virtual classes that wrap callbacks
- Major cleanups
