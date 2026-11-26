/*
 * test_app.c - User-space test for /dev/myusbN
 *
 * Usage:
 *
 *   ./test_app /dev/myusb0 write "hello"
 *   ./test_app /dev/myusb0 read 64
 *   ./test_app /dev/myusb0 exchange "hello" 64
 *
 * The USB device firmware must understand data sent through bulk-OUT.
 * For read/exchange operations, it must send data through bulk-IN.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_READ_SIZE 512U
#define MAX_READ_SIZE     (16U * 1024U)

static void usage(const char *program)
{
    fprintf(
        stderr,
        "Usage:\n"
        "  %s <device> write <text>\n"
        "  %s <device> read [bytes]\n"
        "  %s <device> exchange <text> [bytes]\n",
        program,
        program,
        program
    );
}

static int write_data(int fd,
                      const void *buffer,
                      size_t length)
{
    ssize_t ret;

    ret = write(fd, buffer, length);
    if (ret < 0) {
        perror("write");
        return -1;
    }

    printf("Sent %zd byte(s)\n", ret);

    if ((size_t)ret != length) {
        fprintf(
            stderr,
            "Warning: only %zd of %zu byte(s) sent\n",
            ret,
            length
        );
    }

    return 0;
}

static int read_data(int fd, size_t requested)
{
    unsigned char *buffer;
    ssize_t ret;
    size_t i;

    if (requested == 0 || requested > MAX_READ_SIZE) {
        fprintf(
            stderr,
            "Read size must be between 1 and %u bytes\n",
            MAX_READ_SIZE
        );

        return -1;
    }

    buffer = calloc(1, requested + 1);
    if (!buffer) {
        perror("calloc");
        return -1;
    }

    ret = read(fd, buffer, requested);
    if (ret < 0) {
        perror("read");
        free(buffer);
        return -1;
    }

    printf("Received %zd byte(s)\n", ret);

    /*
     * Display printable characters.
     */
    printf("ASCII: ");

    for (i = 0; i < (size_t)ret; i++) {
        unsigned char c = buffer[i];

        if (c >= 32 && c <= 126)
            putchar(c);
        else
            putchar('.');
    }

    putchar('\n');

    /*
     * Display hexadecimal bytes.
     */
    printf("HEX  : ");

    for (i = 0; i < (size_t)ret; i++)
        printf("%02x ", buffer[i]);

    putchar('\n');

    free(buffer);
    return 0;
}

static int parse_read_size(const char *text, size_t *value)
{
    char *end = NULL;
    unsigned long parsed;

    errno = 0;
    parsed = strtoul(text, &end, 0);

    if (errno != 0 ||
        end == text ||
        *end != '\0' ||
        parsed == 0 ||
        parsed > MAX_READ_SIZE) {
        return -1;
    }

    *value = (size_t)parsed;

    return 0;
}

int main(int argc, char **argv)
{
    const char *device_path;
    const char *operation;
    size_t read_size = DEFAULT_READ_SIZE;
    int fd;
    int status = EXIT_FAILURE;

    if (argc < 3) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    device_path = argv[1];
    operation = argv[2];

    fd = open(device_path, O_RDWR);
    if (fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    /*
     * Write operation:
     *
     * ./test_app /dev/myusb0 write "hello"
     */
    if (strcmp(operation, "write") == 0) {
        if (argc != 4) {
            usage(argv[0]);
            goto out;
        }

        if (write_data(
                fd,
                argv[3],
                strlen(argv[3])
            ) == 0) {
            status = EXIT_SUCCESS;
        }
    }

    /*
     * Read operation:
     *
     * ./test_app /dev/myusb0 read 64
     */
    else if (strcmp(operation, "read") == 0) {
        if (argc > 4) {
            usage(argv[0]);
            goto out;
        }

        if (argc == 4 &&
            parse_read_size(argv[3], &read_size) < 0) {
            fprintf(
                stderr,
                "Invalid read size: %s\n",
                argv[3]
            );

            goto out;
        }

        if (read_data(fd, read_size) == 0)
            status = EXIT_SUCCESS;
    }

    /*
     * Write followed by read:
     *
     * ./test_app /dev/myusb0 exchange "hello" 64
     */
    else if (strcmp(operation, "exchange") == 0) {
        if (argc < 4 || argc > 5) {
            usage(argv[0]);
            goto out;
        }

        if (argc == 5 &&
            parse_read_size(argv[4], &read_size) < 0) {
            fprintf(
                stderr,
                "Invalid read size: %s\n",
                argv[4]
            );

            goto out;
        }

        if (write_data(
                fd,
                argv[3],
                strlen(argv[3])
            ) < 0) {
            goto out;
        }

        if (read_data(fd, read_size) == 0)
            status = EXIT_SUCCESS;
    }

    else {
        fprintf(
            stderr,
            "Unknown operation: %s\n",
            operation
        );

        usage(argv[0]);
    }

out:
    if (close(fd) < 0)
        perror("close");

    return status;
}
