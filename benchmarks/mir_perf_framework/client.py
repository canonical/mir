import os
import subprocess
import shutil
import time
from .common import env_variable_for_client_report

class Client:
    def __init__(self, executable=shutil.which("mir_demo_client_egltriangle"), server=None, reports=[], options=[], env={}):
        """ Creates a Client object.

        The client is not run until it is passed to a PerformanceTest and the
        test is started.

        Args:
            executable (str): The client executable to run.
            server (Server): The server to connect to.
            reports (list of str): The lttng reports to enable. Use the 'client-' prefix to
                                   enable a client report.
            options (list of str): Command-line options to use when running the client.
            env (dict of str=>str): Extra environment variables to set when running the client
        """
        self.executable = executable
        self.server = server
        self.reports = reports
        self.options = options
        self.env = env
        self.process = None

    def start(self):
        cmdline = [self.executable, "-m", self.server.socket_path]
        cmdline.extend(self.options)

        env = os.environ.copy()
        env["LTTNG_UST_WITHOUT_BADDR_STATEDUMP"] = "1"
        for report in self.reports:
            env[env_variable_for_client_report(report)] = "lttng"
        for name,value in self.env.items():
            env[name] = value

        self.process = subprocess.Popen(cmdline, env=env)
        # Wait for client to start
        # TODO: Find a better way to wait for client readiness
        time.sleep(0.25)

    def stop(self):
        self.process.terminate()
