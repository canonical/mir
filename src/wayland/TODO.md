# Design

1. C++ global gets initialized, which tries to register itself with Rust. TODO: Send the global down as the factory!
1. When bound, the C++ global returns a new "Instance" of that Global which will extend
   the `*Handler` class. The C++ global defines a factory for this.
1. Now, other interfaces can be made off of that global very easily!
1. Generated globals should end up in their own file

The change here is that:

1. The entire idea of "Global" and "Resource" are going to go away in favor of the generated "Handler" classes.
1. The subclasses of the "Handlers" will probably need to be modified to meet the new need that we have

# TODO Later

- Generate implementations for each handler that call into their respective C++ classes

  - This should remove the need for "Thunks"
  - The handler should be a `friend` class of the `Global` here

- Update the poor C-casted parameters to be virtual classes that wrap callbacks

- Major cleanups

- Add a way to remove a global from C++
