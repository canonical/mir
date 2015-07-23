#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main()
{
    int ret = open("/dev/shm", O_TMPFILE | O_RDWR | O_EXCL, S_IRWXU);

    if (ret == -1)
        return 1;
    close(ret);
    return 0;
}
