# TODO
- Generate `wayland_rs_register_global` definitions for each toplevel handler.
    - When the C++ class who extends the `Global` constructs itself, we will register the global
      with the internal `WaylandServer`. 
    - When a client binds to an interface, we call into the `HandlerFactory`. *This* is the point
      at which we should call into the original `Global` to `bind`. But how do we resolve the
      `Global` object here? 
    - When the `Global` instantaites, it should register itself as the "listener" on the `GlobalHandlerFactory`
    - When someone binds to a non-global, we should reach out to the parent interface to bind to it.
      
- Generate implementations for each handler that call into their respective C++ classes 
    - This should remove the need for "Thunks"
    - The handler should be a `friend` class of the `Global` here
- Update the poor C-casted parameters to be virtual classes that wrap callbacks
- Major cleanups
- Add a way to remove a global from C++
