// SPDX-License-Identifier: GPL-2.0
/*
 * Basic Linux DMAEngine memory-to-memory demo for x86.
 *
 * This module:
 * 1. Requests a DMA_MEMCPY channel.
 * 2. Allocates coherent source and destination buffers.
 * 3. Starts a DMA transfer.
 * 4. Waits for the completion callback.
 * 5. Compares source and destination.
 */

#include <linux/completion.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/string.h>

#define DRIVER_NAME     "x86_dma_demo"
#define DMA_BUFFER_SIZE 4096
#define DMA_TIMEOUT_MS  2000

static struct dma_chan *demo_chan;
static struct device *demo_dma_dev;

static void *source_buffer;
static void *destination_buffer;

static dma_addr_t source_dma;
static dma_addr_t destination_dma;

static DECLARE_COMPLETION(dma_done);

/*
 * Called by the DMA controller driver when the transaction completes.
 */
static void dma_complete_callback(void *argument)
{
    struct completion *done = argument;

    complete(done);
}

static void free_demo_resources(void)
{
    if (destination_buffer) {
        dma_free_coherent(demo_dma_dev,
                          DMA_BUFFER_SIZE,
                          destination_buffer,
                          destination_dma);

        destination_buffer = NULL;
    }

    if (source_buffer) {
        dma_free_coherent(demo_dma_dev,
                          DMA_BUFFER_SIZE,
                          source_buffer,
                          source_dma);

        source_buffer = NULL;
    }

    if (demo_chan) {
        dma_release_channel(demo_chan);
        demo_chan = NULL;
    }
}

static int run_dma_copy(void)
{
    struct dma_async_tx_descriptor *descriptor;
    dma_cookie_t cookie;
    enum dma_status status;
    unsigned long completed;
    int ret;

    /*
     * Prepare test data.
     */
    memset(source_buffer, 0, DMA_BUFFER_SIZE);
    memset(destination_buffer, 0xA5, DMA_BUFFER_SIZE);

    strscpy(source_buffer,
            "Hello from a real Linux DMAEngine transfer on x86!",
            DMA_BUFFER_SIZE);

    reinit_completion(&dma_done);

    /*
     * Prepare a memory-to-memory DMA descriptor.
     *
     * Important:
     * These are DMA addresses, not CPU virtual addresses.
     */
    descriptor = dmaengine_prep_dma_memcpy(
        demo_chan,
        destination_dma,
        source_dma,
        DMA_BUFFER_SIZE,
        DMA_PREP_INTERRUPT | DMA_CTRL_ACK);

    if (!descriptor) {
        pr_err(DRIVER_NAME
               ": could not prepare DMA memcpy descriptor\n");
        return -EIO;
    }

    /*
     * Register completion callback.
     */
    descriptor->callback = dma_complete_callback;
    descriptor->callback_param = &dma_done;

    /*
     * Submit the descriptor to the DMAEngine framework.
     */
    cookie = dmaengine_submit(descriptor);

    ret = dma_submit_error(cookie);
    if (ret) {
        pr_err(DRIVER_NAME
               ": dmaengine_submit failed: %d\n",
               ret);
        return ret;
    }

    pr_info(DRIVER_NAME
            ": submitted cookie=%d on channel %s\n",
            cookie,
            dma_chan_name(demo_chan));

    /*
     * Tell the DMA controller to start pending transactions.
     */
    dma_async_issue_pending(demo_chan);

    /*
     * Wait until the callback calls complete().
     */
    completed = wait_for_completion_timeout(
        &dma_done,
        msecs_to_jiffies(DMA_TIMEOUT_MS));

    if (!completed) {
        pr_err(DRIVER_NAME ": DMA transfer timed out\n");

        dmaengine_terminate_sync(demo_chan);
        return -ETIMEDOUT;
    }

    /*
     * Check DMAEngine transaction status.
     */
    status = dma_async_is_tx_complete(
        demo_chan,
        cookie,
        NULL,
        NULL);

    if (status != DMA_COMPLETE) {
        pr_err(DRIVER_NAME
               ": DMA completed with status=%d\n",
               status);
        return -EIO;
    }

    /*
     * Verify that DMA copied the complete buffer.
     */
    if (memcmp(source_buffer,
               destination_buffer,
               DMA_BUFFER_SIZE) != 0) {
        pr_err(DRIVER_NAME
               ": verification failed: buffers differ\n");
        return -EIO;
    }

    pr_info(DRIVER_NAME ": DMA copy successful\n");

    pr_info(DRIVER_NAME ": destination says: %s\n",
            (char *)destination_buffer);

    return 0;
}

static int __init x86_dma_demo_init(void)
{
    dma_cap_mask_t capability_mask;
    int ret;

    pr_info(DRIVER_NAME ": loading\n");

    /*
     * We need a DMA controller that supports memory-to-memory copying.
     */
    dma_cap_zero(capability_mask);
    dma_cap_set(DMA_MEMCPY, capability_mask);

    /*
     * Request any available DMA_MEMCPY channel.
     */
    demo_chan = dma_request_channel(
        capability_mask,
        NULL,
        NULL);

    if (!demo_chan) {
        pr_err(DRIVER_NAME
               ": no DMA_MEMCPY channel is available\n");

        pr_err(DRIVER_NAME
               ": check /sys/class/dma and try loading ioatdma\n");

        return -ENODEV;
    }

    /*
     * This device must be used for DMA allocation and mapping.
     */
    demo_dma_dev = demo_chan->device->dev;

    pr_info(DRIVER_NAME
            ": using DMA channel %s, provider device %s\n",
            dma_chan_name(demo_chan),
            dev_name(demo_dma_dev));

    /*
     * dma_alloc_coherent() returns:
     *
     * source_buffer = CPU virtual address
     * source_dma    = DMA/bus address
     */
    source_buffer = dma_alloc_coherent(
        demo_dma_dev,
        DMA_BUFFER_SIZE,
        &source_dma,
        GFP_KERNEL);

    if (!source_buffer) {
        ret = -ENOMEM;
        goto error;
    }

    destination_buffer = dma_alloc_coherent(
        demo_dma_dev,
        DMA_BUFFER_SIZE,
        &destination_dma,
        GFP_KERNEL);

    if (!destination_buffer) {
        ret = -ENOMEM;
        goto error;
    }

    pr_info(DRIVER_NAME
            ": source CPU=%p DMA=%pad\n",
            source_buffer,
            &source_dma);

    pr_info(DRIVER_NAME
            ": destination CPU=%p DMA=%pad\n",
            destination_buffer,
            &destination_dma);

    ret = run_dma_copy();
    if (ret)
        goto error;

    return 0;

error:
    free_demo_resources();
    return ret;
}

static void __exit x86_dma_demo_exit(void)
{
    dmaengine_terminate_sync(demo_chan);
    free_demo_resources();

    pr_info(DRIVER_NAME ": unloaded\n");
}

module_init(x86_dma_demo_init);
module_exit(x86_dma_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TechDhaba demo");
MODULE_DESCRIPTION("Basic x86 Linux DMAEngine memcpy demonstration");
MODULE_VERSION("1.0");
