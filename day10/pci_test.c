/*
 * pcie_test.c - User-space test utility for /dev/mypcieN
 *
 * Examples:
 *
 * sudo ./pcie_test /dev/mypcie0 read 0x0 64
 * sudo ./pcie_test /dev/mypcie0 write 0x100 "Hello PCIe"
 * sudo ./pcie_test /dev/mypcie0 read32 0x20
 * sudo ./pcie_test /dev/mypcie0 write32 0x20 0x12345678
 * sudo ./pcie_test /dev/mypcie0 verify 0x100 "Hello PCIe"
 */

#define _FILE_OFFSET_BITS 64

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_TRANSFER (64U * 1024U)

static void usage(const char *program)
{
    fprintf(
        stderr,

        "Usage:\n"
        "  %s <device> read    <offset> <length>\n"
        "  %s <device> write   <offset> <text>\n"
        "  %s <device> read32  <offset>\n"
        "  %s <device> write32 <offset> <value>\n"
        "  %s <device> verify  <offset> <text>\n",

        program,
        program,
        program,
        program,
        program
    );
}

static int parse_u64(const char *text, uint64_t *value)
{
    char *end = NULL;
    unsigned long long parsed;

    errno = 0;

    parsed = strtoull(
        text,
        &end,
        0
    );

    if (errno ||
        end == text ||
        *end != '\0') {
        return -1;
    }

    *value = (uint64_t)parsed;

    return 0;
}

static int parse_size(const char *text, size_t *value)
{
    uint64_t parsed;

    if (parse_u64(text, &parsed) < 0 ||
        parsed == 0 ||
        parsed > MAX_TRANSFER) {
        return -1;
    }

    *value = (size_t)parsed;

    return 0;
}

static void dump_buffer(
    const unsigned char *buffer,
    size_t length
)
{
    size_t base;
    size_t i;

    for (base = 0;
         base < length;
         base += 16) {

        printf("%08zx  ", base);

        for (i = 0; i < 16; i++) {
            if (base + i < length)
                printf(
                    "%02x ",
                    buffer[base + i]
                );
            else
                printf("   ");

            if (i == 7)
                putchar(' ');
        }

        printf(" |");

        for (i = 0;
             i < 16 && base + i < length;
             i++) {

            unsigned char c = buffer[base + i];

            putchar(
                (c >= 32 && c <= 126)
                    ? (char)c
                    : '.'
            );
        }

        printf("|\n");
    }
}

static int read_bytes(
    int fd,
    uint64_t offset,
    size_t length,
    unsigned char **out_buffer
)
{
    unsigned char *buffer;
    ssize_t ret;

    if (offset > (uint64_t)INT64_MAX) {
        fprintf(
            stderr,
            "Offset is too large\n"
        );

        return -1;
    }

    buffer = calloc(1, length);

    if (!buffer) {
        perror("calloc");
        return -1;
    }

    ret = pread(
        fd,
        buffer,
        length,
        (off_t)offset
    );

    if (ret < 0) {
        perror("pread");
        free(buffer);
        return -1;
    }

    if ((size_t)ret != length) {
        fprintf(
            stderr,
            "Short read: requested %zu, "
            "received %zd byte(s)\n",
            length,
            ret
        );
    }

    *out_buffer = buffer;

    return (int)ret;
}

static int write_bytes(
    int fd,
    uint64_t offset,
    const void *buffer,
    size_t length
)
{
    ssize_t ret;

    if (offset > (uint64_t)INT64_MAX) {
        fprintf(
            stderr,
            "Offset is too large\n"
        );

        return -1;
    }

    ret = pwrite(
        fd,
        buffer,
        length,
        (off_t)offset
    );

    if (ret < 0) {
        perror("pwrite");
        return -1;
    }

    printf(
        "Wrote %zd byte(s) at "
        "BAR offset 0x%" PRIx64 "\n",
        ret,
        offset
    );

    if ((size_t)ret != length) {
        fprintf(
            stderr,
            "Short write: requested %zu, "
            "wrote %zd byte(s)\n",
            length,
            ret
        );

        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    const char *device_path;
    const char *operation;
    uint64_t offset;
    int fd;
    int status = EXIT_FAILURE;

    if (argc < 4) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    device_path = argv[1];
    operation = argv[2];

    if (parse_u64(argv[3], &offset) < 0) {
        fprintf(
            stderr,
            "Invalid offset: %s\n",
            argv[3]
        );

        return EXIT_FAILURE;
    }

    fd = open(device_path, O_RDWR);

    if (fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    /*
     * Read a block of bytes.
     */
    if (strcmp(operation, "read") == 0) {
        unsigned char *buffer;
        size_t length;
        int bytes;

        if (argc != 5 ||
            parse_size(argv[4], &length) < 0) {
            usage(argv[0]);
            goto out;
        }

        bytes = read_bytes(
            fd,
            offset,
            length,
            &buffer
        );

        if (bytes < 0)
            goto out;

        printf(
            "Read %d byte(s) at "
            "BAR offset 0x%" PRIx64 "\n",
            bytes,
            offset
        );

        dump_buffer(
            buffer,
            (size_t)bytes
        );

        free(buffer);

        status = EXIT_SUCCESS;
    }

    /*
     * Write a text string.
     */
    else if (strcmp(operation, "write") == 0) {
        const char *text;

        if (argc != 5) {
            usage(argv[0]);
            goto out;
        }

        text = argv[4];

        if (write_bytes(
                fd,
                offset,
                text,
                strlen(text)
            ) == 0) {
            status = EXIT_SUCCESS;
        }
    }

    /*
     * Read one native-endian 32-bit value.
     */
    else if (strcmp(operation, "read32") == 0) {
        unsigned char *buffer;
        uint32_t value;
        int bytes;

        if (argc != 4) {
            usage(argv[0]);
            goto out;
        }

        bytes = read_bytes(
            fd,
            offset,
            sizeof(value),
            &buffer
        );

        if (bytes != (int)sizeof(value)) {
            free(buffer);
            goto out;
        }

        memcpy(
            &value,
            buffer,
            sizeof(value)
        );

        free(buffer);

        printf(
            "BAR[0x%" PRIx64 "] = "
            "0x%08" PRIx32
            " (%" PRIu32 ")\n",

            offset,
            value,
            value
        );

        status = EXIT_SUCCESS;
    }

    /*
     * Write one native-endian 32-bit value.
     */
    else if (strcmp(operation, "write32") == 0) {
        uint64_t parsed_value;
        uint32_t value;

        if (argc != 5 ||
            parse_u64(argv[4], &parsed_value) < 0 ||
            parsed_value > UINT32_MAX) {

            usage(argv[0]);
            goto out;
        }

        value = (uint32_t)parsed_value;

        if (write_bytes(
                fd,
                offset,
                &value,
                sizeof(value)
            ) == 0) {
            status = EXIT_SUCCESS;
        }
    }

    /*
     * Write a string and read it back from the same BAR offset.
     *
     * This works only when the endpoint provides scratch memory or
     * read/write registers at that offset.
     */
    else if (strcmp(operation, "verify") == 0) {
        const char *text;
        size_t length;
        unsigned char *readback;
        int bytes;

        if (argc != 5) {
            usage(argv[0]);
            goto out;
        }

        text = argv[4];
        length = strlen(text);

        if (length == 0 ||
            length > MAX_TRANSFER) {

            fprintf(
                stderr,
                "Text length must be 1-%u bytes\n",
                MAX_TRANSFER
            );

            goto out;
        }

        if (write_bytes(
                fd,
                offset,
                text,
                length
            ) < 0) {
            goto out;
        }

        bytes = read_bytes(
            fd,
            offset,
            length,
            &readback
        );

        if (bytes != (int)length) {
            free(readback);
            goto out;
        }

        if (memcmp(
                text,
                readback,
                length
            ) != 0) {

            fprintf(
                stderr,
                "Verification failed\n"
            );

            dump_buffer(
                readback,
                length
            );

            free(readback);
            goto out;
        }

        free(readback);

        printf("Verification passed\n");

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
