#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

int main(int argc, char** argv)
{
    assert(argc > 0);
    setenv("MIR_CLIENT_PLATFORM_PATH", BUILD_PREFIX "/lib/client-modules/", 1);

    argv[0] = BUILD_PREFIX "/bin/" EXECUTABLE;
    execv(argv[0], argv);

    perror("Failed to execute real binary " EXECUTABLE);
    return 1;
}
