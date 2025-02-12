#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <asm/uaccess.h>
#include <linux/mutex.h>

#define USE_DMA_MALLOC
// #define DMA_CONFIG_DEBUG

#define DEVICE_NAME	"udma"
#define IOC_MAGIC	'c'
#define DMA_MEMCPY_CMD	_IOR(IOC_MAGIC, 0, int)
#define DMA_VA_TO_PA	_IOR(IOC_MAGIC, 1, int)

static unsigned char dma_major;
static struct class *dma_class;
static struct dma_device *dma_dev;
static struct dma_chan *dma_chan;
static struct dma_async_tx_descriptor *dma_tx;
static struct list_head dmabuf_list;
static struct completion dma_m2m_ok;
static struct mutex dma_mutex;

typedef struct {
	void			*src;
	void			*dst;
	size_t			size;
#ifdef DMA_CONFIG_DEBUG
	long long		time[10];
	int			time_cnt;
	int			dma_irq_subscript;
#endif
} memcpy_msg_t;

typedef struct {
	void 			*user_addr;
	void 			*dma_addr;
} va_to_pa_msg_t;

typedef struct {
	size_t			size;		// Size of the buffer
	unsigned long		user_addr;	// User virtual address of the buffer
	void			*kern_addr;	// Kernel virtual address of the buffer
	dma_addr_t		dma_addr;	// DMA bus address of the buffer
	struct list_head	list;		// List node pointers for dma alloc list
} dma_map_info_t;

typedef struct {
	dma_addr_t 		addr;
	size_t 			size;
	int 			dirty;
} va2pa_t;

#ifdef DMA_CONFIG_DEBUG
#include <linux/time.h>

#define DMA_TIME_STAMP()			\
	do {					\
		g_time[g_time_cnt++] = getus();	\
	} while (0)

static volatile int g_time_cnt;
static volatile long long g_time[10];
static int g_dma_irq_subscript;

static long long getus(void)
{
	return ktime_to_us(ktime_get());
}
#else
#define DMA_TIME_STAMP()
#endif

static void dma_callback_func(void *priv)
{
#ifdef DMA_CONFIG_DEBUG
	g_dma_irq_subscript = g_time_cnt;
#endif
	DMA_TIME_STAMP();
	complete(&dma_m2m_ok);
}

static int dma_open(struct inode *inode, struct file *filp)
{
	return (dma_dev != NULL) ? 0 : -EPERM;
}

static int dma_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t dma_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
	return size;
}
 
static ssize_t dma_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
	return size;
}

#ifdef USE_DMA_MALLOC
static int dma_malloc(dma_map_info_t *dma_info, struct vm_area_struct *vma)
{
	int ret;

	dma_info->kern_addr = dma_alloc_coherent(dma_dev->dev, dma_info->size, &dma_info->dma_addr, GFP_KERNEL);
	if (!dma_info->kern_addr) {
		dev_err(dma_dev->dev,"Unable to allocate contiguous DMA memory region of size " \
				   "%zu.\n", dma_info->size);
		return -ENOMEM;
	}

	ret = dma_mmap_coherent(dma_dev->dev, vma, dma_info->kern_addr,
						   dma_info->dma_addr, dma_info->size);
	if (ret < 0) {
		dev_err(dma_dev->dev,"Unable to remap address %p to userspace address 0x%lx, size "\
				   "%zu.\n", dma_info->kern_addr, dma_info->user_addr, \
				   dma_info->size);
		return -1;
	}

	return 0;
}

static int dma_free(dma_map_info_t *dma_info)
{
	dma_free_coherent(dma_dev->dev, dma_info->size, dma_info->kern_addr, dma_info->dma_addr);

	return 0;
}
#else
static int kernel_malloc(dma_map_info_t *dma_info, struct vm_area_struct *vma)
{
	dma_info->kern_addr = kmalloc(dma_info->size, GFP_KERNEL);
	if (!dma_info->kern_addr) {
		dev_err(dma_dev->dev,"kmalloc failed\n");
		return -ENOMEM;
	}

	if (remap_pfn_range(vma, 
			vma->vm_start,
			(virt_to_phys(dma_info->kern_addr) >> PAGE_SHIFT),
			vma->vm_end - vma->vm_start,
			vma->vm_page_prot)) {
		return -EAGAIN;
	}

	dma_info->dma_addr = dma_map_single(dma_dev->dev, dma_info->kern_addr, dma_info->size, DMA_FROM_DEVICE);
	if (dma_mapping_error(dma_dev->dev, dma_info->dma_addr)) {
		dev_err(dma_dev->dev,"mapping buffer failed\n");
		return -1;
	}

	return 0;
}

static int kernel_free(dma_map_info_t *dma_info)
{
	dma_unmap_single(dma_dev->dev, dma_info->dma_addr, dma_info->size, DMA_FROM_DEVICE);
	kfree(dma_info->kern_addr);

	return 0;
}
#endif

static void dma_vma_close(struct vm_area_struct *vma)
{
	dma_map_info_t *dma_info;

	dma_info = vma->vm_private_data;
#ifdef USE_DMA_MALLOC
	dma_free(dma_info);
#else
	kernel_free(dma_info);
#endif
	kfree(dma_info);

	return;
}

static const struct vm_operations_struct dma_vm_ops = {
	.close = dma_vma_close,
};

static int dma_mmap(struct file *file, struct vm_area_struct *vma)
{
	int		ret;
	dma_map_info_t *dma_info;

	dma_info = kmalloc(sizeof(*dma_info), GFP_KERNEL);
	if (dma_info == NULL) {
		dev_err(dma_dev->dev,"Unable to allocate VMA data structure.");
		return -ENOMEM;
	}

	dma_info->size		= vma->vm_end - vma->vm_start;
	dma_info->user_addr	= vma->vm_start;

#ifdef USE_DMA_MALLOC
	ret = dma_malloc(dma_info, vma);
#else
	ret = kernel_malloc(dma_info, vma);
#endif

	if (ret < 0) {
		return -1;
	}

	vma->vm_ops		= &dma_vm_ops;
	vma->vm_private_data	= dma_info;
	vm_flags_set(vma, VM_DONTCOPY);

	list_add(&dma_info->list, &dmabuf_list);
	return 0;
}

static int va2pa(void *va, size_t size, va2pa_t **va2pa)
{
	pmd_t *pmd;
	pte_t *pte;

	unsigned long pg_offset;
	unsigned long pg_address;
	unsigned long old_pfn;
	unsigned long paddr;
	size_t total;
	int flag = 0;

	va2pa_t *p;
	int num = size/sizeof(PAGE_SIZE);
	unsigned long vaddr = (unsigned long)va;
	int i, j;

	p = kmalloc(num*sizeof(va2pa_t), GFP_KERNEL);
	*va2pa = p;

	j = 0;
	old_pfn = 0;
	total = 0;

	for (i = 0; i < num; i++) {
		memset(p +i, 0x00, sizeof(va2pa_t));
		pmd = pmd_off(current->mm, vaddr);
		if(pmd_none(*pmd)) {
			dev_err(dma_dev->dev, "not in the pmd!");
			flag = -1;
			break;
		}

		pte = pte_offset_map(pmd, vaddr);
		if(pte_none(*pte)) {
			dev_err(dma_dev->dev, "not in the pte!");
			flag = -1;
			break;
		}

		pg_offset = offset_in_page(vaddr);
		pg_address = pte_pfn(__pte(pte_val(*pte))) << PAGE_SHIFT;
		paddr = pg_address | pg_offset;
		pte_unmap(pte);
		if ((old_pfn + PAGE_SIZE) == pg_address) {
			p[j].size += (PAGE_SIZE - (vaddr - (vaddr & PAGE_MASK)));
			total += (PAGE_SIZE - (vaddr - (vaddr & PAGE_MASK)));
			p[j].dirty = 1;
		} else {
			if (p[j].dirty == 1) {
				j++;
			}
			p[j].addr = paddr;
			p[j].size = (PAGE_SIZE - (vaddr - (vaddr & PAGE_MASK)));
			total += p[j].size;
			p[j].dirty = 1;
		}
		if (total >= size) {
			j ++;
			break;
		}
		old_pfn = pg_address;
		vaddr = (vaddr & PAGE_MASK) + PAGE_SIZE;
	}

	if (flag == -1) {
		kfree(p);
		*va2pa = NULL;
		j = 0;
	}

	return j;
}

static int dma_memcpy(dma_addr_t dst, dma_addr_t src, size_t size)
{
#ifndef USE_DMA_MALLOC
	dma_sync_single_for_cpu(dma_dev->dev, (dma_addr_t)src, size, DMA_FROM_DEVICE);
#endif
	mutex_lock(&dma_mutex);
	dma_tx = dma_dev->device_prep_dma_memcpy(dma_chan,
						dst,
						src,
						size,
						DMA_CTRL_ACK | DMA_PREP_INTERRUPT);
	if (!dma_tx){
		mutex_unlock(&dma_mutex);
		dev_err(dma_dev->dev, "Failed to prepare DMA memcpy");
		return -1;
	}

	dma_tx->callback		= dma_callback_func;//set call back function
	dma_tx->callback_param		= NULL;
	if (dma_submit_error(dma_tx->tx_submit(dma_tx))){
		mutex_unlock(&dma_mutex);
		dev_err(dma_dev->dev, "Failed to do DMA tx_submit");
		return -1;
	}

	init_completion(&dma_m2m_ok);
	dma_async_issue_pending(dma_chan);//begin dma transfer
	wait_for_completion(&dma_m2m_ok);
	reinit_completion(&dma_m2m_ok);

#ifndef USE_DMA_MALLOC
	dma_sync_single_for_device(dma_dev->dev, (dma_addr_t)msg.dst, msg.size, DMA_FROM_DEVICE);
#endif
	mutex_unlock(&dma_mutex);

	return 0;
}

static long dma_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (cmd == DMA_MEMCPY_CMD) {
		memcpy_msg_t msg;
		va2pa_t *s_pa_l, *d_pa_l;
		int i, loop, ret;
		size_t off;

		if(copy_from_user(&msg, (void *)arg, sizeof(memcpy_msg_t))) {
			return -EFAULT;
		}

		ret = va2pa(msg.src, msg.size, &s_pa_l);
		if (ret != 1) {
			if (s_pa_l) {
				kfree(s_pa_l);
			}
			return -1;
		}
		ret = va2pa(msg.dst, msg.size, &d_pa_l);
		if (ret == 0) {
			kfree(s_pa_l);
			return -1;
		}

		loop = ret;
		off = 0;

		for (i = 0; i < loop; i++) {	
			dma_memcpy(d_pa_l[i].addr, s_pa_l[0].addr + off, d_pa_l[i].size);
			off += d_pa_l[i].size;
		}

		kfree(s_pa_l);
		kfree(d_pa_l);
	}
	return 0;
}

static const struct file_operations dma_fops = {
 	.owner		= THIS_MODULE,
 	.read		= dma_read,
 	.write		= dma_write,
 	.open		= dma_open,
	.release	= dma_release,
	.mmap		= dma_mmap,
	.unlocked_ioctl = dma_ioctl,
};

static int dma_init(void)
{
	dma_cap_mask_t mask;
	struct device *dev_ret;

	dma_major = register_chrdev(0, DEVICE_NAME, &dma_fops);
	if (dma_major < 0)
		return dma_major;

	dma_class = class_create(DEVICE_NAME);
	if (IS_ERR(dma_class))
		return -1;

	dev_ret = device_create(dma_class, NULL, MKDEV(dma_major, 0), NULL, DEVICE_NAME);
	if (IS_ERR(dev_ret))
		return PTR_ERR(dev_ret);

	dma_cap_zero(mask);
	dma_cap_set(DMA_MEMCPY, mask);//direction:memory to memory
	dma_chan = dma_request_channel(mask,NULL,NULL); //request a dma channel
	if (!dma_chan) {
		printk(KERN_ERR"dma request failed\n");
		return -1;
	}

	dma_dev = dma_chan->device;
	dma_set_mask(dma_dev->dev, DMA_BIT_MASK(32));

	INIT_LIST_HEAD(&dmabuf_list);
	dev_dbg(dma_dev->dev, "dma channel id = %d\n",dma_chan->chan_id);
	mutex_init(&dma_mutex);

	return 0;
}

static void dma_exit(void)
{
	unregister_chrdev(dma_major, DEVICE_NAME);
	device_destroy(dma_class, MKDEV(dma_major, 0));
	class_destroy(dma_class);
	dma_release_channel(dma_chan);
}

module_init(dma_init);
module_exit(dma_exit);

MODULE_LICENSE("GPL");
