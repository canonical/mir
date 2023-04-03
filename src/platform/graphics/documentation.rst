Platform interfaces
===================

Top level design
----------------

Constraints
___________

* Platforms differ in their capabilities
* A single system may require multiple different platforms to support all the hardware

Relevant objects:
-----------------

`RenderingPlatform`
  Provides graphics buffer allocation services, both to clients and internally to Mir.
  In conjunction with the `DisplayPlatform` provides a
