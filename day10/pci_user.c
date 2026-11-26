#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define DEVICE "/dev/pci_demo"


int main(void)
{
    int fd;

    uint32_t tx_data;
    uint32_t rx_data;

    ssize_t ret;


    /*
     * Open PCI device.
     */
    fd = open(DEVICE, O_RDWR);

    if (fd < 0) {

        perror("open");

        return 1;
    }


    printf("PCI device opened\n");


    /*
     * -----------------------------------------
     * SEND
     * -----------------------------------------
     */

    tx_data = 0x12345678;


    printf("Sending: 0x%08x\n",
           tx_data);


    ret = write(fd,
                &tx_data,
                sizeof(tx_data));


    if (ret < 0) {

        perror("write");

    } else {

        printf("Sent %zd bytes\n",
               ret);
    }


    /*
     * -----------------------------------------
     * RECEIVE
     * -----------------------------------------
     */

    ret = read(fd,
               &rx_data,
               sizeof(rx_data));


    if (ret < 0) {

        perror("read");

    } else {

        printf("Received: 0x%08x\n",
               rx_data);
    }


    close(fd);


    return 0;
}
