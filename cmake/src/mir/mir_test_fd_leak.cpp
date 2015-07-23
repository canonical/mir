#include <fcntl.h>
#include <stdio.h>

int main()
{
    puts("[==========] 0 tests");
    open("/dev/zero", O_RDONLY);
}
