This benchmark uses a client server setup and simulated touch events in order to compute two metrics: Average Pixel Lag, and Frame Uniformity.

Average pixel lag is measured as follows. A client is run, and put in position to receive the simulated touch events. Said client renders frames as fast as allowed (in the test, a simulated vsync timer on the server throttles rendering). At each frame time, the client records its most up to date pointer sample. Following receipt of all the events, the touch simulation parameters are used to interpolate where the touch was expected at each frame time. The average pixel lag is the average difference between this physical (simulated) touch location, and the client recording at each frame time.

Frame uniformity is the standard deviation of the average pixel lag over all samples.

Several test parameters are variable : TODO: Explain how to vary, currently requires code changes.
Touch event start
Touch event end
Touch duration
Vsync rate
Input event rate
Test repeat count (resulting in averaged results).
