Using Mir on an Android device {#using_mir_on_android}
==============================

Mir is the default on Ubuntu Touch images, so if you're using Ubuntu Touch,
you're already using Mir.

If you would like to run a pre-release version of Mir on your device, you'll
need to recompile our downstream dependencies (unity-system-compositor and 
qtmir) to ensure ABI compatibility with the pre-release version of Mir.

Using some demo applications
----------------------------

Simple demos are available in the `mir-demos` package.

First install the demos:

    $ sudo apt-get install mir-demos

Next ensure that the Unity8 session is ended:

    $ sudo stop lightdm

Finally, start mir and a client application:

    $ mir_demo_server --test-client mir_demo_client_egltriangle

and you should see a triangle on screen.
