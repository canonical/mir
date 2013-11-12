Welcome to Mir {#mainpage}
==============

Mir is a next generation display server targeted as a replacement for the X
window server system to unlock next-generation user experiences for devices
ranging from Linux desktop to mobile devices powered by Ubuntu. The primary
purpose of Mir is to enable the development of the next generation
[Unity](http://unity.ubuntu.com).

More detailed information about the motivation, scope, and high-level design
of Mir can be found at http://wiki.ubuntu.com/MirSpec .

Getting and installing Mir
--------------------------

### Using pre-built packages

If you just want to try out mir, or write client applications, then the easiest
way is to use the pre-built packages:

 - \ref installing_prebuilt_on_pc
 - \ref installing_prebuilt_on_android

### Building and installing from source

If you are curious about Mir internals or intend to contribute to it, you should
get the source and build it:

 - \ref building_source_for_pc
 - \ref building_source_for_android

Using Mir
---------

 - \ref using_mir_on_pc
 - \ref using_mir_on_android
 - \ref debug_for_xmir

Getting involved
----------------

The best place to ask questions and discuss about the Mir project is the
#ubuntu-mir IRC channel on freenode.

The Mir project is hosted on Launchpad: https://launchpad.net/mir

Please file bug reports at: https://bugs.launchpad.net/mir

The Mir development mailing list can be found at: https://lists.ubuntu.com/mailman/listinfo/Mir-devel

The Mir coding guidelines are [here](cppguide/index.html).

Learn about Mir
----------------
Android technical info:
 - \ref android_technical_details

Writing client applications
---------------------------

 - \ref mir_toolkit "Mir API Documentation"
 - \subpage basic.c "basic.c: A basic Mir client (which does nothing)"

Writing server applications
---------------------------

Mir server is written as a library which allows the server code to be adapted
for bespoke applications.

 - \subpage render_surfaces-example "render_surfaces.cpp: A simple program using the Mir library"
 - \ref demo_inprocess_egl

Working on Mir code
-------------------

 - \ref md_README  "Mir Read me"
 - \ref md_HACKING "Mir hacking guide"
 - \ref component_reports
