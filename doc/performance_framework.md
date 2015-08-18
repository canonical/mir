Mir performance framework {#performance_framework}
=========================

In order to facilitate writing portable and easy to share performance tests,
Mir provides a performance framework written in python3.

Each test, along with code to process the test results, is contained in a
single python3 script which uses the Mir performance framework modules.

Invoking tests
--------------

To run a test, just execute the python3 script containing it.

We need to ensure that the Mir performance framework can be found by python.
If you have installed the Mir performance framework to one of the standard
python3 library locations (e.g., with make install or a package installation),
then the framework should be automatically detected. If you are using the
framework from within the source tree you need to add the directory containing
the mir_perf_framework/ directory (i.e., its parent directory) to the
PYTHONPATH env. variable:

    sudo PYTHONPATH=/path/to/mir/benchmarks python3 testscript.py

If you are using an Ubuntu system the framework comes pre-packaged in the
python3-mir-perf-framework package, which, besides the framework itself,
also installs a few interesting tests in /usr/share/mir-perf-framework/.

Writing test scripts
--------------------

### Specifying the test configuration

To create a test we first need to instantiate objects representing the servers
and clients we want to run as part of the test, and also the kind of LTTng
reports we want to enable for each one of them:

    from mir_perf_framework import Server, Client
    host = Server()
    nested = Server(...)
    client = Client(...)

### Specifying the executable to run

By default the servers use the 'mir_demo_server' executable and the clients the
'mir_demo_client_egltriangle' executable. We can override this by using the
'executable' option. We can also set custom command line arguments to use when
invoking the executable (even the default ones). Finally we can set extra
environment variables using the 'env' option, which is a dictionary of variable
names and values.

     server = Server(executable=shutil.which("my_test_server"),
                     options=["--window-manager", "fullscreen"],
                     env = {"MIR_SERVER_MY_OPTION": "on"})

### Specifying the host server a server should connect to

By default a server is started in host mode. If we want to create a nested
server we need to set the host server it will connect to by using the 'host'
option:

    host = Server()
    nested = Server(host=host)

### Specifying the server a should connect to

A client needs to know the server to connect to. This is set with the 'server'
option:

    host = Server()
    client = Client(server=host)

### Specifying the lttng reports to enable

To enable an LTTng report add it to the list passed to the 'reports' option. To
enable a client report (which only makes sense for clients or nested servers),
prefix them with 'client-':

    nested = Server(host=host, reports=["compositor", "client-input-receiver"])
    client = Client(server=nested, reports=["client-input-receiver"])

Starting and stopping the test
------------------------------

After we have instantiated objects representing our test setup, we need
to create a PerformanceTest using these objects:

    from mir_perf_framework import Server, Client, PerformanceTest
    ...
    test = PerformanceTest([host, nested, client])

Note that the order of the objects in the list matters, since server/clients
are created and run in list order when the test is started.

To start the test use the start() method, and after finishing stop it using the
stop() method:

    test = PerformanceTest([host, nested, client])
    test.start()
    ...
    test.stop()

Simulating input events
-----------------------

To simulate input events during tests, you can use the evdev python3 module.

    import evdev

    ui = evdev.UInput()

    # during testing ...
    ui.write(evdev.ecodes.EV_KEY, evdev.ecodes.KEY_A, 1)
    ui.syn()

Accessing LTTng events
-----------------------

After the test has finished (i.e., after it has been stop()-ed), we can access
the recorded LTTng events in two ways. The first is to get the raw babeltrace
trace object and extract the data from it:

    ### NOT RECOMMENDED unless you know what you are doing
    ### use test.events() instead
    trace = test.babeltrace()

    for event in trace.events:
        print(event.name)
        interesting_event = event # This works, but interesting_event will
                                # contain invalid contents later.

    print(trace.events[i].name) # Random access is not supported.


Unfortunately, accessing events this way has two severe limitations. First,
only sequential access is supported, and, second, we are allowed to access only
one event at a time (the active iteration event), since references to other
events do not remain valid after we have moved on.

To make life easier, the PerformanceTest object offers the events() method,
which returns all the LTTng events in a normal, random-access capable python
list.

    events = test.events()

    for event in events:
        print(event.name)
        interesting_event = event # OK! Interesting event is always valid.

    print(events[i].name) # Random access is OK too!

This is the recommend way of accessing the events, unless you need some
functionality offered only by the babeltrace APIs. Note, however, that
creating the list of events may incur a small delay.
