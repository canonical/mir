#!/usr/bin/python3

from mir_perf_framework import PerformanceTest, Server, Client
import time
import evdev
import statistics
import subprocess

###### Helper classes ######

class TouchScreen:
    def __init__(self):
        res = self.get_resolution()

        allowed_events = {
            evdev.ecodes.EV_ABS : (
                (evdev.ecodes.ABS_MT_POSITION_X, (0, res[0], 0, 0)),
                (evdev.ecodes.ABS_MT_POSITION_Y, (0, res[1], 0, 0)),
                (evdev.ecodes.ABS_MT_TOUCH_MAJOR, (0, 30, 0, 0)),
                (evdev.ecodes.ABS_MT_TRACKING_ID, (0, 65535, 0, 0)),
                (evdev.ecodes.ABS_MT_PRESSURE, (0, 255, 0, 0)),
                (evdev.ecodes.ABS_MT_SLOT, (0, 9, 0, 0))
                ),
            evdev.ecodes.EV_KEY: [
                evdev.ecodes.BTN_TOUCH,
                ]
        }
        
        self.ui = evdev.UInput(events=allowed_events, name="autopilot-finger")

    def get_resolution(self):
        out = subprocess.check_output(["fbset", "-s"])
        out_list = out.split()
        geometry = out_list.index(b"geometry")
        return (int(out_list[geometry + 1]), int(out_list[geometry + 2]))

    def finger_down_at(self, xy):
        self.ui.write(evdev.ecodes.EV_ABS, evdev.ecodes.ABS_MT_SLOT, 0)
        self.ui.write(evdev.ecodes.EV_ABS, evdev.ecodes.ABS_MT_TRACKING_ID, 0)
        self.ui.write(evdev.ecodes.EV_KEY, evdev.ecodes.BTN_TOUCH, 1)
        self.ui.write(evdev.ecodes.EV_ABS, evdev.ecodes.ABS_MT_POSITION_X, xy[0])
        self.ui.write(evdev.ecodes.EV_ABS, evdev.ecodes.ABS_MT_POSITION_Y, xy[1])
        self.ui.write(evdev.ecodes.EV_ABS, evdev.ecodes.ABS_MT_PRESSURE, 50)
        self.ui.write(evdev.ecodes.EV_ABS, evdev.ecodes.ABS_MT_TOUCH_MAJOR, 4)
        self.ui.syn()

    def finger_up(self):
        self.ui.write(evdev.ecodes.EV_ABS, evdev.ecodes.ABS_MT_TRACKING_ID, -1)
        self.ui.syn()

    def finger_move_to(self, xy):
        self.ui.write(evdev.ecodes.EV_ABS, evdev.ecodes.ABS_MT_SLOT, 0)
        self.ui.write(evdev.ecodes.EV_ABS, evdev.ecodes.ABS_MT_POSITION_X, xy[0])
        self.ui.write(evdev.ecodes.EV_ABS, evdev.ecodes.ABS_MT_POSITION_Y, xy[1])
        self.ui.syn()


####### TEST #######

# Disable input resampling so that the event_time field of input events,
# used to calculate latency, is accurate
no_resampling_env = {"MIR_CLIENT_INPUT_RATE": "0"}

host = Server(reports=["input"])
nested = Server(host=host, reports=["client-input-receiver"], env=no_resampling_env)
client = Client(server=nested, reports=["client-input-receiver"], options=["-f"], env=no_resampling_env)

test = PerformanceTest([host, nested, client])
touch_screen = TouchScreen()

test.start()

# Perform three 1-second drag movements
for i in range(3):
    xy = (100,100)
    touch_screen.finger_down_at(xy)
    for i in range(100):
        touch_screen.finger_move_to(xy)
        xy = (xy[0] + 5, xy[1] + 5)
        time.sleep(0.01)
    touch_screen.finger_up()

test.stop()

####### TRACE PARSING #######

trace = test.babeltrace()

pids = {}
data = {}

for event in trace.events:
    if event.name == "mir_client_input_receiver:touch_event":
        pid = event["vpid"]
        if pid not in pids.values():
            if "nested" not in pids:
                pids["nested"] = pid
            elif "client" not in pids:
                pids["client"] = pid
        if pid not in data: data[pid] = []

        data[pid].append((event.timestamp - event["event_time"]) / 1000000.0)


print("=== Results ===")

nested_data = data[pids["nested"]]
print("Nested server received %d events" % len(nested_data))
print("Kernel to nested server mean: %f ms stdev: %f ms" %
      (statistics.mean(nested_data), statistics.stdev(nested_data)))

client_data = data[pids["client"]]
print("Client received %d events" % len(client_data))
print("Kernel to client mean: %f ms stdev: %f ms" %
      (statistics.mean(client_data), statistics.stdev(client_data)))
