Using Mir on an Android device {#using_mir_on_android}
==============================

After installing mir on your device (see \ref installing_prebuilt_on_android)
switch off SurfaceFlinger (see: https://wiki.ubuntu.com/Touch/Testing/Mir#Switch_from_SurfaceFlinger_to_Mir).
(Alternatively, you can install a mir based image as described here: https://wiki.ubuntu.com/Touch/Testing/Mir#Easiest_way)

Open a shell on the device:

    $ adb shell

Now, start Mir and a client application:

    # mir_demo_server_basic &
    # mir_demo_client_something   # take your pick of something

Getting some example client applications
----------------------------------------

You can get some example programs by installing the `mir-demos` package
inside the Ubuntu touch chroot shell (see above):

    # apt-get install mir-demos
