#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/pci.h>
#include <linux/io.h>

#define DRIVER_NAME "pci_demo"

#define PCI_VENDOR_ID_DEMO  0x8086
#define PCI_DEVICE_ID_DEMO  0x9a03


struct pci_demo_device {
    struct pci_dev *pdev;

    void __iomem *bar0;
    resource_size_t bar0_start;
    resource_size_t bar0_len;
};


static int pci_demo_probe(struct pci_dev *pdev,
                          const struct pci_device_id *id)
{
    struct pci_demo_device *dev;
    int ret;

    pr_info(DRIVER_NAME ": probe() called\n");

    pr_info(DRIVER_NAME ": Vendor ID = 0x%04x\n",
            pdev->vendor);

    pr_info(DRIVER_NAME ": Device ID = 0x%04x\n",
            pdev->device);

    dev = devm_kzalloc(&pdev->dev,
                       sizeof(*dev),
                       GFP_KERNEL);

    if (!dev)
        return -ENOMEM;

    dev->pdev = pdev;

    pci_set_drvdata(pdev, dev);

    /*
     * Enable PCI device
     */
    ret = pci_enable_device(pdev);

    if (ret) {
        pr_err(DRIVER_NAME
               ": pci_enable_device() failed\n");
        return ret;
    }

    /*
     * Request PCI BAR resources
     */
    ret = pci_request_region(pdev, 0, DRIVER_NAME);

    if (ret) {
        pr_err(DRIVER_NAME
               ": pci_request_region() failed\n");

        pci_disable_device(pdev);
        return ret;
    }

    /*
     * Get BAR0 information
     */
    dev->bar0_start = pci_resource_start(pdev, 0);
    dev->bar0_len   = pci_resource_len(pdev, 0);

    pr_info(DRIVER_NAME
            ": BAR0 start = 0x%llx\n",
            (unsigned long long)dev->bar0_start);

    pr_info(DRIVER_NAME
            ": BAR0 length = 0x%llx\n",
            (unsigned long long)dev->bar0_len);

    /*
     * Map BAR0 into kernel virtual address space
     */
    dev->bar0 = pci_iomap(pdev, 0, 0);

    if (!dev->bar0) {
        pr_err(DRIVER_NAME
               ": pci_iomap() failed\n");

        pci_release_region(pdev, 0);
        pci_disable_device(pdev);

        return -ENOMEM;
    }

    pr_info(DRIVER_NAME
            ": BAR0 mapped at %p\n",
            dev->bar0);

    pr_info(DRIVER_NAME ": PCI device ready\n");

    return 0;
}


static void pci_demo_remove(struct pci_dev *pdev)
{
    struct pci_demo_device *dev;

    pr_info(DRIVER_NAME ": remove() called\n");

    dev = pci_get_drvdata(pdev);

    if (dev && dev->bar0)
        pci_iounmap(pdev, dev->bar0);

    pci_release_region(pdev, 0);

    pci_disable_device(pdev);

    pr_info(DRIVER_NAME ": PCI device removed\n");
}


/*
 * PCI device ID table
 */
static const struct pci_device_id pci_demo_ids[] = {

    {
        PCI_DEVICE(PCI_VENDOR_ID_DEMO,
                   PCI_DEVICE_ID_DEMO)
    },

    { }
};

MODULE_DEVICE_TABLE(pci, pci_demo_ids);


/*
 * PCI driver structure
 */
static struct pci_driver pci_demo_driver = {

    .name     = DRIVER_NAME,

    .id_table = pci_demo_ids,

    .probe    = pci_demo_probe,

    .remove   = pci_demo_remove,
};


static int __init pci_demo_init(void)
{
    pr_info(DRIVER_NAME ": module loaded\n");

    return pci_register_driver(&pci_demo_driver);
}


static void __exit pci_demo_exit(void)
{
    pr_info(DRIVER_NAME ": module unloaded\n");

    pci_unregister_driver(&pci_demo_driver);
}


module_init(pci_demo_init);
module_exit(pci_demo_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("TechDhaba");
MODULE_DESCRIPTION("Simple Linux PCI driver");
MODULE_VERSION("1.0");
