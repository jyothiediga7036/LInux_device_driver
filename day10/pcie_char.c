// SPDX-License-Identifier: GPL-2.0
/*
 * pcie_char.c - Minimal PCI/PCIe BAR character driver
 *
 * Features:
 *   - Matches a PCI vendor/device ID
 *   - Enables a memory BAR and maps it with pci_iomap()
 *   - Creates /dev/mypcieN
 *   - read()/write() access a bounded BAR memory window
 *   - llseek() selects the BAR offset
 *   - Handles driver unbind/hot-remove while descriptors are open
 *
 * This driver is intended only for a custom FPGA/test endpoint whose BAR is
 * explicitly designed as a byte-addressable scratch/register window.
 *
 * Do NOT bind it to an NVMe controller, GPU, NIC, storage controller, or any
 * other production device. Arbitrary MMIO accesses can hang or damage data.
 */

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/idr.h>
#include <linux/io.h>
#include <linux/kref.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#define DRIVER_NAME             "pcie_char"
#define DEVICE_NAME             "mypcie"
#define PCIE_MAX_DEVICES        256
#define PCIE_MAX_IO_SIZE        (64U * 1024U)

/*
 * Replace these demonstration IDs with your custom endpoint's IDs.
 *
 * Find them using:
 *
 *     lspci -nn
 */
#define PCIE_VENDOR_ID          0x1234
#define PCIE_DEVICE_ID          0x5678

/*
 * BAR0 is selected by default.
 *
 * Override it while loading:
 *
 *     sudo insmod pcie_char.ko bar=2
 */
static int bar;
module_param(bar, int, 0444);
MODULE_PARM_DESC(bar, "PCI BAR number to expose (0-5, default 0)");

struct pcie_char_dev {
    struct pci_dev *pdev;

    void __iomem *bar_addr;
    resource_size_t bar_start;
    resource_size_t bar_len;
    int bar_num;

    struct mutex io_lock;
    struct kref kref;
    bool removed;

    struct cdev *cdev;
    dev_t devt;
    int minor;
    struct device *device;
};

static dev_t pcie_base_devt;
static struct class *pcie_class;
static DEFINE_IDA(pcie_minor_ida);

/*
 * open() resolves devices through their minor number.
 *
 * This table and its mutex protect against open/remove races.
 */
static DEFINE_MUTEX(pcie_table_lock);

static struct pcie_char_dev *pcie_devices[PCIE_MAX_DEVICES];

static struct pci_driver pcie_char_driver;

static void pcie_char_delete(struct kref *kref)
{
    struct pcie_char_dev *dev;

    dev = container_of(kref, struct pcie_char_dev, kref);

    pci_dev_put(dev->pdev);
    kfree(dev);
}

/*
 * Called when user space executes:
 *
 *     open("/dev/mypcie0", O_RDWR);
 */
static int pcie_char_open(struct inode *inode, struct file *file)
{
    struct pcie_char_dev *dev = NULL;
    unsigned int absolute_minor = iminor(inode);
    unsigned int first_minor = MINOR(pcie_base_devt);
    unsigned int index;
    int ret = 0;

    if (absolute_minor < first_minor)
        return -ENODEV;

    index = absolute_minor - first_minor;

    if (index >= PCIE_MAX_DEVICES)
        return -ENODEV;

    /*
     * Find the device and acquire a reference while holding the
     * global table lock.
     */
    mutex_lock(&pcie_table_lock);

    dev = pcie_devices[index];

    if (!dev || !kref_get_unless_zero(&dev->kref))
        dev = NULL;

    mutex_unlock(&pcie_table_lock);

    if (!dev)
        return -ENODEV;

    mutex_lock(&dev->io_lock);

    if (dev->removed)
        ret = -ENODEV;
    else
        file->private_data = dev;

    mutex_unlock(&dev->io_lock);

    if (ret) {
        kref_put(&dev->kref, pcie_char_delete);
        return ret;
    }

    return 0;
}

/*
 * Called when the application closes its file descriptor.
 */
static int pcie_char_release(struct inode *inode, struct file *file)
{
    struct pcie_char_dev *dev = file->private_data;

    if (!dev)
        return -ENODEV;

    file->private_data = NULL;

    kref_put(&dev->kref, pcie_char_delete);

    return 0;
}

/*
 * Read bytes from the PCI BAR.
 *
 * User-space flow:
 *
 *     pread(fd, buffer, length, bar_offset)
 *          |
 *          v
 *     pcie_char_read()
 *          |
 *          v
 *     memcpy_fromio()
 *          |
 *          v
 *     PCIe BAR
 */
static ssize_t pcie_char_read(struct file *file,
                              char __user *user_buffer,
                              size_t count,
                              loff_t *ppos)
{
    struct pcie_char_dev *dev = file->private_data;
    unsigned char *buffer;
    resource_size_t remaining;
    size_t transfer_len;
    ssize_t ret;

    if (!dev)
        return -ENODEV;

    if (count == 0)
        return 0;

    if (*ppos < 0)
        return -EINVAL;

    mutex_lock(&dev->io_lock);

    if (dev->removed || !dev->bar_addr) {
        ret = -ENODEV;
        goto out_unlock;
    }

    /*
     * Reading beyond the end of the BAR behaves like reaching EOF.
     */
    if ((resource_size_t)*ppos >= dev->bar_len) {
        ret = 0;
        goto out_unlock;
    }

    remaining = dev->bar_len - (resource_size_t)*ppos;

    transfer_len = min_t(resource_size_t,
                         remaining,
                         count);

    transfer_len = min_t(size_t,
                         transfer_len,
                         PCIE_MAX_IO_SIZE);

    buffer = kmalloc(transfer_len, GFP_KERNEL);

    if (!buffer) {
        ret = -ENOMEM;
        goto out_unlock;
    }

    /*
     * Copy from MMIO space into a normal kernel buffer.
     */
    memcpy_fromio(
        buffer,
        (u8 __iomem *)dev->bar_addr + *ppos,
        transfer_len
    );

    if (copy_to_user(user_buffer, buffer, transfer_len)) {
        ret = -EFAULT;
        goto out_free;
    }

    *ppos += transfer_len;
    ret = transfer_len;

out_free:
    kfree(buffer);

out_unlock:
    mutex_unlock(&dev->io_lock);

    return ret;
}

/*
 * Write bytes into the PCI BAR.
 *
 * User-space flow:
 *
 *     pwrite(fd, buffer, length, bar_offset)
 *          |
 *          v
 *     pcie_char_write()
 *          |
 *          v
 *     memcpy_toio()
 *          |
 *          v
 *     PCIe BAR
 *
 * PCIe writes can be posted. A production driver must implement the
 * readback, acknowledgement, or ordering requirements documented by
 * the particular PCIe endpoint.
 */
static ssize_t pcie_char_write(struct file *file,
                               const char __user *user_buffer,
                               size_t count,
                               loff_t *ppos)
{
    struct pcie_char_dev *dev = file->private_data;
    unsigned char *buffer;
    resource_size_t remaining;
    size_t transfer_len;
    ssize_t ret;

    if (!dev)
        return -ENODEV;

    if (count == 0)
        return 0;

    if (*ppos < 0)
        return -EINVAL;

    mutex_lock(&dev->io_lock);

    if (dev->removed || !dev->bar_addr) {
        ret = -ENODEV;
        goto out_unlock;
    }

    if ((resource_size_t)*ppos >= dev->bar_len) {
        ret = -ENOSPC;
        goto out_unlock;
    }

    remaining = dev->bar_len - (resource_size_t)*ppos;

    transfer_len = min_t(resource_size_t,
                         remaining,
                         count);

    transfer_len = min_t(size_t,
                         transfer_len,
                         PCIE_MAX_IO_SIZE);

    buffer = memdup_user(user_buffer, transfer_len);

    if (IS_ERR(buffer)) {
        ret = PTR_ERR(buffer);
        goto out_unlock;
    }

    /*
     * Copy normal kernel memory into PCIe MMIO space.
     */
    memcpy_toio(
        (u8 __iomem *)dev->bar_addr + *ppos,
        buffer,
        transfer_len
    );

    *ppos += transfer_len;
    ret = transfer_len;

    kfree(buffer);

out_unlock:
    mutex_unlock(&dev->io_lock);

    return ret;
}

/*
 * Select a BAR offset using:
 *
 *     lseek()
 *     pread()
 *     pwrite()
 */
static loff_t pcie_char_llseek(struct file *file,
                               loff_t offset,
                               int whence)
{
    struct pcie_char_dev *dev = file->private_data;
    loff_t ret;

    if (!dev)
        return -ENODEV;

    mutex_lock(&dev->io_lock);

    if (dev->removed)
        ret = -ENODEV;
    else
        ret = fixed_size_llseek(
            file,
            offset,
            whence,
            dev->bar_len
        );

    mutex_unlock(&dev->io_lock);

    return ret;
}

static const struct file_operations pcie_char_fops = {
    .owner   = THIS_MODULE,
    .open    = pcie_char_open,
    .release = pcie_char_release,
    .read    = pcie_char_read,
    .write   = pcie_char_write,
    .llseek  = pcie_char_llseek,
};

/*
 * Called when PCI core finds a matching VID/DID.
 */
static int pcie_char_probe(struct pci_dev *pdev,
                           const struct pci_device_id *id)
{
    struct pcie_char_dev *dev;
    unsigned long flags;
    int ret;

    if (bar < 0 || bar >= PCI_STD_NUM_BARS) {
        dev_err(
            &pdev->dev,
            "invalid BAR number %d; expected 0-5\n",
            bar
        );

        return -EINVAL;
    }

    dev = kzalloc(sizeof(*dev), GFP_KERNEL);

    if (!dev)
        return -ENOMEM;

    kref_init(&dev->kref);
    mutex_init(&dev->io_lock);

    dev->pdev = pci_dev_get(pdev);
    dev->bar_num = bar;
    dev->minor = -1;

    /*
     * Enable PCI memory decoding for this device.
     */
    ret = pci_enable_device_mem(pdev);

    if (ret) {
        dev_err(
            &pdev->dev,
            "pci_enable_device_mem failed: %d\n",
            ret
        );

        goto error_put;
    }

    flags = pci_resource_flags(pdev, bar);

    /*
     * This driver supports only memory BARs.
     * Legacy PCI I/O-port BARs are rejected.
     */
    if (!(flags & IORESOURCE_MEM)) {
        dev_err(
            &pdev->dev,
            "BAR%d is not a memory BAR\n",
            bar
        );

        ret = -ENODEV;
        goto error_disable;
    }

    dev->bar_start = pci_resource_start(pdev, bar);
    dev->bar_len = pci_resource_len(pdev, bar);

    if (!dev->bar_start || !dev->bar_len) {
        dev_err(
            &pdev->dev,
            "BAR%d is empty or unassigned\n",
            bar
        );

        ret = -ENODEV;
        goto error_disable;
    }

    /*
     * Reserve the BAR so another driver cannot use it simultaneously.
     */
    ret = pci_request_region(
        pdev,
        bar,
        DRIVER_NAME
    );

    if (ret) {
        dev_err(
            &pdev->dev,
            "cannot reserve BAR%d; another driver may own it: %d\n",
            bar,
            ret
        );

        goto error_disable;
    }

    /*
     * Map the complete BAR into kernel virtual address space.
     */
    dev->bar_addr = pci_iomap(pdev, bar, 0);

    if (!dev->bar_addr) {
        dev_err(
            &pdev->dev,
            "pci_iomap BAR%d failed\n",
            bar
        );

        ret = -ENOMEM;
        goto error_release_region;
    }

    dev->minor = ida_alloc_range(
        &pcie_minor_ida,
        0,
        PCIE_MAX_DEVICES - 1,
        GFP_KERNEL
    );

    if (dev->minor < 0) {
        ret = dev->minor;

        dev_err(
            &pdev->dev,
            "minor allocation failed: %d\n",
            ret
        );

        goto error_iounmap;
    }

    dev->devt = MKDEV(
        MAJOR(pcie_base_devt),
        MINOR(pcie_base_devt) + dev->minor
    );

    /*
     * Use a separately allocated cdev so its kernel-managed lifetime
     * is independent of the PCI private structure.
     */
    dev->cdev = cdev_alloc();

    if (!dev->cdev) {
        ret = -ENOMEM;
        goto error_free_minor;
    }

    dev->cdev->owner = THIS_MODULE;
    dev->cdev->ops = &pcie_char_fops;

    ret = cdev_add(
        dev->cdev,
        dev->devt,
        1
    );

    if (ret) {
        dev_err(
            &pdev->dev,
            "cdev_add failed: %d\n",
            ret
        );

        goto error_put_cdev;
    }

    mutex_lock(&pcie_table_lock);
    pcie_devices[dev->minor] = dev;
    mutex_unlock(&pcie_table_lock);

    /*
     * Creates:
     *
     *     /dev/mypcie0
     *     /dev/mypcie1
     */
    dev->device = device_create(
        pcie_class,
        &pdev->dev,
        dev->devt,
        dev,
        DEVICE_NAME "%d",
        dev->minor
    );

    if (IS_ERR(dev->device)) {
        ret = PTR_ERR(dev->device);
        dev->device = NULL;

        dev_err(
            &pdev->dev,
            "device_create failed: %d\n",
            ret
        );

        goto error_remove_table;
    }

    pci_set_drvdata(pdev, dev);

    dev_info(
        &pdev->dev,
        "bound to %s, VID=%04x DID=%04x, "
        "BAR%d start=%pa size=%pa, node=/dev/%s%d\n",

        pci_name(pdev),
        pdev->vendor,
        pdev->device,

        bar,
        &dev->bar_start,
        &dev->bar_len,

        DEVICE_NAME,
        dev->minor
    );

    return 0;

error_remove_table:
    mutex_lock(&pcie_table_lock);
    pcie_devices[dev->minor] = NULL;
    mutex_unlock(&pcie_table_lock);

    cdev_del(dev->cdev);
    dev->cdev = NULL;

    goto error_free_minor;

error_put_cdev:
    kobject_put(&dev->cdev->kobj);
    dev->cdev = NULL;

error_free_minor:
    ida_free(&pcie_minor_ida, dev->minor);
    dev->minor = -1;

error_iounmap:
    pci_iounmap(pdev, dev->bar_addr);
    dev->bar_addr = NULL;

error_release_region:
    pci_release_region(pdev, bar);

error_disable:
    pci_disable_device(pdev);

error_put:
    kref_put(&dev->kref, pcie_char_delete);

    return ret;
}

/*
 * Called when:
 *
 *   - the PCIe device is removed;
 *   - the driver is unbound;
 *   - or the module is unloaded.
 */
static void pcie_char_remove(struct pci_dev *pdev)
{
    struct pcie_char_dev *dev = pci_get_drvdata(pdev);

    if (!dev)
        return;

    pci_set_drvdata(pdev, NULL);

    /*
     * Stop new opens.
     */
    mutex_lock(&pcie_table_lock);

    if (dev->minor >= 0 &&
        pcie_devices[dev->minor] == dev) {
        pcie_devices[dev->minor] = NULL;
    }

    mutex_unlock(&pcie_table_lock);

    if (dev->device) {
        device_destroy(
            pcie_class,
            dev->devt
        );

        dev->device = NULL;
    }

    if (dev->cdev) {
        cdev_del(dev->cdev);
        dev->cdev = NULL;
    }

    if (dev->minor >= 0) {
        ida_free(
            &pcie_minor_ida,
            dev->minor
        );

        dev->minor = -1;
    }

    /*
     * Wait for a current BAR access to complete and make existing
     * open file descriptors fail with -ENODEV.
     */
    mutex_lock(&dev->io_lock);

    dev->removed = true;

    if (dev->bar_addr) {
        pci_iounmap(
            pdev,
            dev->bar_addr
        );

        dev->bar_addr = NULL;
    }

    mutex_unlock(&dev->io_lock);

    pci_release_region(
        pdev,
        dev->bar_num
    );

    pci_disable_device(pdev);

    dev_info(
        &pdev->dev,
        "device removed; character node destroyed\n"
    );

    /*
     * Open descriptors retain their own kref until release().
     */
    kref_put(
        &dev->kref,
        pcie_char_delete
    );
}

/*
 * PCI vendor/device matching table.
 */
static const struct pci_device_id pcie_char_ids[] = {
    {
        PCI_DEVICE(
            PCIE_VENDOR_ID,
            PCIE_DEVICE_ID
        )
    },
    { }
};

MODULE_DEVICE_TABLE(pci, pcie_char_ids);

static struct pci_driver pcie_char_driver = {
    .name     = DRIVER_NAME,
    .id_table = pcie_char_ids,
    .probe    = pcie_char_probe,
    .remove   = pcie_char_remove,
};

static int __init pcie_char_init(void)
{
    int ret;

    ret = alloc_chrdev_region(
        &pcie_base_devt,
        0,
        PCIE_MAX_DEVICES,
        DRIVER_NAME
    );

    if (ret)
        return ret;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    pcie_class = class_create(DRIVER_NAME);
#else
    pcie_class = class_create(
        THIS_MODULE,
        DRIVER_NAME
    );
#endif

    if (IS_ERR(pcie_class)) {
        ret = PTR_ERR(pcie_class);

        unregister_chrdev_region(
            pcie_base_devt,
            PCIE_MAX_DEVICES
        );

        return ret;
    }

    ret = pci_register_driver(&pcie_char_driver);

    if (ret) {
        class_destroy(pcie_class);

        unregister_chrdev_region(
            pcie_base_devt,
            PCIE_MAX_DEVICES
        );

        return ret;
    }

    pr_info(
        DRIVER_NAME
        ": loaded; matching %04x:%04x, BAR%d\n",
        PCIE_VENDOR_ID,
        PCIE_DEVICE_ID,
        bar
    );

    return 0;
}

static void __exit pcie_char_exit(void)
{
    pci_unregister_driver(&pcie_char_driver);

    class_destroy(pcie_class);

    unregister_chrdev_region(
        pcie_base_devt,
        PCIE_MAX_DEVICES
    );

    ida_destroy(&pcie_minor_ida);

    pr_info(
        DRIVER_NAME ": unloaded\n"
    );
}

module_init(pcie_char_init);
module_exit(pcie_char_exit);

MODULE_AUTHOR("Prashant Soni / TechDhaba");
MODULE_DESCRIPTION(
    "PCIe BAR character driver with bounded MMIO read/write"
);
MODULE_LICENSE("GPL");
