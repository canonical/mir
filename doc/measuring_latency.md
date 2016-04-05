Measuring Visual Latency
========================
Mir includes a utility called `mirvanity` that can use a high speed camera
to measure the visual latency of your display (from client rendering to
your eye).

It is called "mirvanity" because it works by rendering a pattern to the
screen and then looking at the screen itself via the camera. In theory you
could also use a laptop screen and webcam with a mirror, however laptop
webcams tend to be too slow for this task. In order to use `mirvanity` you
need a high speed camera supported by Linux, such as a PlayStation Eye.

How to use mirvanity
--------------------
  1. Start your Mir server
  2. Plug in your camera
  3. Position the camera on a stable surface so that it does not move
     during the test, and such that it is primarily looking at your
     display only. The camera must be centred on the display.
  4. Run `mirvanity`
  5. Wait until the output (in the console/stdout for now) shows that the
     test is complete (which means enough data has been gathered).
