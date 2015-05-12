Mir component reports {#component_reports}
=====================

Both the server library and the client library include facilities to provide
debugging and tracing information at runtime. This is achieved through
component reports, which are sets of interesting events provided by many Mir
components. A component report can be usually handled in a number of different
ways, configured using command-line options and/or environment variables. By
default, component reports are turned off.

Server reports
--------------

The way component reports are handled on the server can be configured using
either command-line options or environment variables. The environment variables
are prefixed with `MIR_SERVER_` and contain underscores ('_') instead of dashes
('-').  The available component reports and handlers for the server are:

Report                       | Handlers
---------------------------- | --------
connector-report             | log,lttng
compositor-report            | log,lttng
display-report               | log,lttng
input-report                 | log,lttng
legacy-input-report          | log
msg-processor-report         | log,lttng
session-mediator-report      | log,lttng
scene-report                 | log,lttng
shared-library-prober-report | log,lttng

For example, to enable the LTTng input report, one could either use the
`--input-report=lttng` command-line option to the server, or set the
`MIR_SERVER_INPUT_REPORT=lttng` environment variable.

Client reports
--------------

Client side reports can be configured only using environment variables.  The
environment variables are prefixed with `MIR_CLIENT_` and contain only
underscores. The available reports and handlers for the client are:

Report                | Handlers
--------------------- | --------
rpc-report            | log,lttng
input-receiver-report | log,lttng
shared-library-prober-report | log,lttng
perf-report           | log,lttng

For example, to enable the logging RPC report, one should set the
`MIR_CLIENT_RPC_REPORT=log` environment variable.

LTTng support
-------------

Mir provides LTTng tracepoints for various interesting events. You can enable
LTTng tracing for a Mir component by using the corresponding command-line
option or environment variable for that component's report:

    $ lttng create mirsession -o /tmp/mirsession
    $ lttng enable-event -u -a
    $ lttng start
    $ mir_demo_server --msg-processor-report=lttng
    $ lttng stop
    $ babeltrace /tmp/mirsession/<trace-subdir>

LTTng-UST versions up to and including 2.1.2, and up to and including 2.2-rc2
contain a bug (lttng #538) that prevents event recording if the tracepoint
provider is dlopen()-ed at runtime, like in the case of Mir. If you have a
version of LTTng affected by this bug, you need to pre-load the server
tracepoint provider library:

    $ LD_PRELOAD=libmirserverlttng.so mir_demo_server --msg-processor-report=lttng

The bug also affects client-side LTTng tracing, in which case you need to
pre-load the client tracepoint provider library:

    $ LD_PRELOAD=libmirclientlttng.so MIR_CLIENT_RPC_REPORT=lttng myclient
