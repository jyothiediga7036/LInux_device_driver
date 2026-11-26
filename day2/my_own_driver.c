#include <fcntl.h>
#include <unistd.h>

int main()
{
    int fd;

    fd = open("/dev/mydevice", O_RDWR);

    write(fd, "Hello", 5);

    close(fd);

    return 0;
}
