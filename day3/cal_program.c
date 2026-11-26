#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "common.h"

int main()
{
    struct calculator data;

    int fd = open("/dev/caldev", O_RDWR);
    if (fd < 0)
    {
        perror("open");
        return 1;
    }

    data.a = 10;
    data.b = 20;
    data.op = '+';
    if (write(fd, &data, sizeof(data)) < 0)
    {
        perror("write");
        close(fd);
        return 1;
    }
    lseek(fd, 0, SEEK_SET);
    if (read(fd, &data, sizeof(data)) < 0)
    {
        perror("read");
        close(fd);
        return 1;
    }

    printf("result = %d\n", data.result);

    close(fd);

    return 0;
}
