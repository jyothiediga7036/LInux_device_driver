#include<linux/module.h>
#include<linux/dma-mapping.h>

#define SIZE 4096

static void*cpu_addr;
static dma_addr_t dma_addr;
struct device *dev;

static int __init dma_init(void)
{
	pr_info("DMA demo init  loadded\n");
	cpu_addr=dma_alloc_coherent(NULL,SIZE,&dma_addr,GFP_KERNEL);
	if(!cpu_addr)
	{
	pr_info("DMA allocation failed\n");
	return -ENOMEM;
	}
	
	pr_info("cpu address =%p\n",cpu_addr);
	pr_info("DMA address =%p\n",&dma_addr);
	
	strcpy(cpu_addr,"hello DMA");
	pr_info("Buffer =%s\n",(char*)cpu_addr);
	
	return 0;
}

static void __exit dma_exit(void)
{
	dma_free_coherent(NULL,SIZE,cpu_addr,dma_addr);
	
	pr_info("DMA buffer freed\n");
	
}
module_init(dma_init);
module_exit(dma_exit);

MODULE_LICENSE("GPL");

	
