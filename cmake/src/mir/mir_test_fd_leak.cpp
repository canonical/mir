#include <fcntl.h>

int main()
{
    open("/dev/zero", O_RDONLY);
}
