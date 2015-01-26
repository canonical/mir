How to avoid deploy-and-test.sh
===============================

If you're running tests regularly from different branches on a remote
Ubuntu touch device, you should avoid deploy-and-test.sh because it's very
slow to complete. Particularly if you're making code changes and need to
repeat the script each time. Instead, you can set up your device for rapid
iteration like so:

One-time setup
--------------
   adb push ~/.ssh/id_rsa.pub /home/phablet/.ssh/authorized_keys
   adb shell
      chown phablet ~/.ssh
      chmod -R go-w ~/.ssh
      mkdir ~/testrundir
      sudo start ssh
      sudo mount -o remount,rw /
      sudo apt-get install umockdev

Now for each branch you want to deploy and test
-----------------------------------------------
   device=<ip-or-hostname-of-your-device>
   cd build-android-arm
   rsync -avH lib bin phablet@$device:testrundir/
   adb shell  (or ssh phablet@$device)
      cd testrundir
      umockdev-run bin/mir_unit-tests
      ...
