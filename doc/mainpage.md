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

### Building and installing from source

If you are curious about Mir internals or intend to contribute to it, you should
get the source and build it:

 - \ref building_source_for_pc
 - \ref building_source_for_arm

### Preparing a VM to run Mir

Especially if you want to debug the shell without locking your system this might be a helpful setup:

- \ref setup_kvm_for_mir
- \ref setup_vmware_for_mir

Using Mir
---------

 - \ref using_mir_on_pc
 - \ref demo_server

Getting involved
----------------

The best place to ask questions and discuss about the Mir project is
the \#ubuntu-mir IRC channel on freenode.

The Mir project is hosted on Launchpad: https://launchpad.net/mir

Currently, the Mir code activity is performed on a development branch:
lp:~mir-team/mir/development-branch

Approximately fortnightly, this development branch is promoted to the branch
used for the ubuntu archive and touch images. Please submit any merge proposals 
against the development branch.

Please file bug reports at: https://bugs.launchpad.net/mir

The Mir development mailing list can be found at: https://lists.ubuntu.com/mailman/listinfo/Mir-devel

The Mir coding guidelines are [here](cppguide/index.html).

Writing client applications
---------------------------

 - \ref mir_toolkit "Mir API Documentation"
 - \subpage basic.c "basic.c: A basic Mir client (which does nothing)"

Writing server applications
---------------------------

Mir server is written as a library which allows the server code to be adapted
for bespoke applications.

 - \subpage server_example.cpp 
   "server_example.cpp: a test executable hosting the following"
 - \subpage server_example_input_event_filter.cpp 
   "server_example_input_event_filter.cpp: provide a Quit command"
 - \subpage server_example_display_configuration_policy.cpp 
   "server_example_display_configuration_policy.cpp: configuring display layout"
 - \subpage server_example_input_filter.cpp 
   "server_example_input_filter.cpp: print input events to stdout"
 - \subpage server_example_log_options.cpp 
   "server_example_log_options.cpp: replace Mir logger with glog"
 - \subpage server_example_basic_window_manager.h 
   "server_example_basic_window_manager.h: How to wire up a window manager"
 - \subpage server_example_window_management.cpp 
   "server_example_window_management.cpp: simple window management examples"
 - \subpage server_example_canonical_window_manager.cpp 
   "server_example_canonical_window_manager.cpp: canonical window management policy"
 - \subpage server_example_custom_compositor.cpp 
   "server_example_custom_compositor.cpp: demonstrate writing an alternative GL rendering code"

Working on Mir code
-------------------

 - \ref md_README  "Mir Read me"
 - \ref md_HACKING "Mir hacking guide"
 - \ref component_reports
 - \ref dso_versioning_guide
 - \ref abi_compatibility_tools
 - \ref performance_framework
 - \ref latency "Measuring visual latency"
