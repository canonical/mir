Using Mir on an Android device {#using_mir_on_android}
==============================

Mir is the default on Ubuntu Touch images, so if you're using Ubuntu Touch,
you're already using mir.

If you would like to run a pre-release version of mir on your device, you'll
need to recompile our downstream dependencies (unity-system-compositor and 
qtmir) to ensure ABI compatibility with the pre-release version of mir.

Using some demo applications
----------------------------

Simpler demos are available in the `mir-demos` package.

First install the demos:

    $ apt-get install mir-demos

Next unsure that the Unity8 session is ended:

    $ service lightdm stop

Finally, start mir and a client application:

    $ mir_demo_server --test-client mir_demo_client_egltriangle

and you should see a triangle on screen.
