#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>

#define UART0_BASE_PHYS    0xFE201000
#define UART0_SIZE         0x90

/* UART Register Offsets */
#define UART_DR            0x00
#define UART_FR            0x18
#define UART_IBRD          0x24
#define UART_FBRD          0x28
#define UART_LCRH          0x2C
#define UART_CR            0x30
#define UART_ICR           0x44

/* Flag Register Bits */
#define FR_TXFF            (1 << 5)   /* Transmit FIFO Full */
#define FR_RXFE            (1 << 4)   /* Receive FIFO Empty */

static void __iomem *uart_base;

/*----------------------------------------------------------*/
/* Send one character                                        */
/*----------------------------------------------------------*/
static void uart_putc(char c)
{
    while (readl(uart_base + UART_FR) & FR_TXFF)
        cpu_relax();

    writel(c, uart_base + UART_DR);
}

/*----------------------------------------------------------*/
/* Receive one character                                     */
/*----------------------------------------------------------*/
static char uart_getc(void)
{
    while (readl(uart_base + UART_FR) & FR_RXFE)
        cpu_relax();

    return (char)(readl(uart_base + UART_DR) & 0xFF);
}

/*----------------------------------------------------------*/
/* Send a string                                             */
/*----------------------------------------------------------*/
static void uart_puts(const char *str)
{
    while (*str)
        uart_putc(*str++);
}

/*----------------------------------------------------------*/
/* Module Initialization                                     */
/*----------------------------------------------------------*/
static int __init uart_init(void)
{
    char c;

    pr_info("UART ioremap driver loading...\n");

    /* Map UART registers */
    uart_base = ioremap(UART0_BASE_PHYS, UART0_SIZE);
    if (!uart_base) {
        pr_err("Failed to map UART registers\n");
        return -ENOMEM;
    }

    /* Disable UART before configuration */
    writel(0x0, uart_base + UART_CR);

    /* Clear pending interrupts */
    writel(0x7FF, uart_base + UART_ICR);

    /*
     * UART Clock = 48 MHz
     * Baud Rate = 115200
     *
     * Baud Divider = UARTCLK / (16 × Baud)
     *               = 48000000 / (16 × 115200)
     *               = 26.0416
     *
     * Integer Part = 26
     * Fraction Part = 3
     */
    writel(26, uart_base + UART_IBRD);
    writel(3, uart_base + UART_FBRD);

    /* Configure UART
     * WLEN = 11 (8-bit)
     * FEN  = 1 (Enable FIFO)
     */
    writel((1 << 4) | (3 << 5), uart_base + UART_LCRH);

    /* Enable UART, TX and RX */
    writel((1 << 0) | (1 << 8) | (1 << 9),
           uart_base + UART_CR);

    uart_puts("\r\n=================================\r\n");
    uart_puts(" BCM2711 UART Driver Started\r\n");
    uart_puts("=================================\r\n");

    uart_puts("Type one character: ");

    c = uart_getc();

    uart_puts("\r\nYou typed: ");
    uart_putc(c);
    uart_puts("\r\n");

    return 0;
}

/*----------------------------------------------------------*/
/* Module Exit                                                */
/*----------------------------------------------------------*/
static void __exit uart_exit(void)
{
    uart_puts("\r\nStopping UART driver...\r\n");

    /* Disable UART */
    writel(0x0, uart_base + UART_CR);

    /* Unmap registers */
    if (uart_base)
        iounmap(uart_base);

    pr_info("UART ioremap driver unloaded\n");
}

module_init(uart_init);
module_exit(uart_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Prashant");
MODULE_DESCRIPTION("BCM2711 PL011 UART Driver using ioremap()");
