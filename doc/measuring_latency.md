Measuring Visual Latency {#latency}
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

Accuracy
--------

A common question is how accurate mirvanity's results are. An LCD monitor
typically refreshes at 60Hz on an interval of about 16.6ms, and you have
the added variability of the camera which even at high speed has a frame
interval of about 5ms. So surely you have at least 22ms of variability?

Yes indeed instantaneous measurements will have wide variability that is
the sum of the display and camera frame intervals. However both of these
devices are very precise even though they're not in phase. So using
many samples over a short period of time, `mirvanity` calculates the
expected variability and compares it to the measured variability. After
the expectation starts to match the measurement you have a good estimate of
the baseline latency and estimated error range. `mirvanity` prints out this
error and for a typical 60Hz monitor and common 187Hz PlayStation Eye camera,
we observe pretty much the variability expected of around 22ms.

Knowing this variability is the sum of both waves, you can simply take the
trough (or peak) as your measurement and thus eliminate the variability of
the display and camera from the results. If you take the trough, you are
excluding all display and camera latency. If you take the peak then you are
including worst case display and camera latency. `mirvanity` reports all of
these numbers for completeness. This is based on the
[Superposition Principle](https://en.wikipedia.org/wiki/Superposition_principle)
and typically yields a stable measurement with approximately 3ms or less
random variation.
