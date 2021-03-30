#include"g2d_driver_i.h"
#include<linux/g2d_driver.h>

#define G2D_BYTE_ALIGN(x) ( ( (x + (4*1024-1)) >> 12) << 12)			 /* alloc based on 4K byte */
static struct info_mem g2d_mem[MAX_G2D_MEM_INDEX];
static int	g2d_mem_sel = 0;
static enum g2d_scan_order scan_order;
static struct mutex global_lock;

static struct class	*g2d_class;
static struct cdev	*g2d_cdev;
static dev_t		 devid ;
__g2d_drv_t			 g2d_ext_hd;
__g2d_info_t		 para;

#if !defined(CONFIG_OF)
static struct resource g2d_resource[2] =
{
	[0] = {
		.start	= SUNXI_MP_PBASE,
		.end	= SUNXI_MP_PBASE + 0x000fffff,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= INTC_IRQNO_DE_MIX,
		.end	= INTC_IRQNO_DE_MIX,
		.flags	= IORESOURCE_IRQ,
	},
};
#endif

__s32 drv_g2d_init(void)
{
	g2d_init_para init_para;

	DBG("drv_g2d_init\n");
	init_para.g2d_base		= (__u32)para.io;
	memset(&g2d_ext_hd, 0, sizeof(__g2d_drv_t));
	init_waitqueue_head(&g2d_ext_hd.queue);
	g2d_init(&init_para);

	return 0;
}

void *g2d_malloc(__u32 bytes_num, __u32 *phy_addr)
{
	void* address = NULL;

#if defined(CONFIG_ION_SUNXI)
	u32 actual_bytes;
	if (bytes_num != 0) {
		actual_bytes = PAGE_ALIGN(bytes_num);

		address = dma_alloc_coherent(para.dev, actual_bytes, (dma_addr_t*)phy_addr, GFP_KERNEL);
		if (address) {
			DBG("dma_alloc_coherent ok, address=0x%p, size=0x%x\n", (void*)(*(unsigned long*)phy_addr), bytes_num);
			return address;
		} else {
			ERR("dma_alloc_coherent fail, size=0x%x\n", bytes_num);
			return NULL;
		}
	} else {
		ERR("%s size is zero\n", __func__);
	}
#else
	unsigned map_size = 0;
	struct page *page;

	if (bytes_num != 0) {
		map_size = PAGE_ALIGN(bytes_num);
		page = alloc_pages(GFP_KERNEL,get_order(map_size));
		if (page != NULL) {
			address = page_address(page);
			if (NULL == address)	{
				free_pages((unsigned long)(page),get_order(map_size));
				ERR("page_address fail!\n");
				return NULL;
			}
			*phy_addr = virt_to_phys(address);
			return address;
		}	else {
			ERR("alloc_pages fail!\n");
			return NULL;
		}
	} else {
		ERR("%s size is zero\n", __func__);
	}
#endif

	return NULL;
}

void g2d_free(void *virt_addr, void *phy_addr, unsigned int size)
{
#if defined(CONFIG_ION_SUNXI)
	u32 actual_bytes;
	actual_bytes = PAGE_ALIGN(size);
	if (phy_addr && virt_addr)
		dma_free_coherent(para.dev, actual_bytes, virt_addr, (dma_addr_t)phy_addr);
#else
	unsigned map_size = PAGE_ALIGN(size);
	unsigned page_size = map_size;

	if (NULL == virt_addr)
		return ;

	free_pages((unsigned long)virt_addr,get_order(page_size));
#endif
	return ;
}

__s32 g2d_get_free_mem_index(void)
{
	__u32 i = 0;

	for(i=0; i<MAX_G2D_MEM_INDEX; i++)
	{
		if(g2d_mem[i].b_used == 0)
		{
			return i;
		}
	}
	return -1;
}

int g2d_mem_request(__u32 size)
{
	__s32 sel;
	__u32 ret = 0;
	__u32 phy_addr;

	sel = g2d_get_free_mem_index();
	if(sel < 0)
	{
		ERR("g2d_get_free_mem_index fail!\n");
		return -EINVAL;
	}

	ret = (__u32)g2d_malloc(size,&phy_addr);
	if(ret != 0)
	{
		g2d_mem[sel].virt_addr = (void*)ret;
		memset(g2d_mem[sel].virt_addr,0,size);
		g2d_mem[sel].phy_addr = phy_addr;
		g2d_mem[sel].mem_len = size;
		g2d_mem[sel].b_used = 1;

		INFO("map_g2d_memory[%d]: pa=%08lx va=%p size:%x\n",sel,g2d_mem[sel].phy_addr, g2d_mem[sel].virt_addr, size);
		return sel;
	}
	else
	{
		ERR("fail to alloc reserved memory!\n");
		return -ENOMEM;
	}
}

int g2d_mem_release(__u32 sel)
{
	if(g2d_mem[sel].b_used == 0)
	{
		ERR("mem not used in g2d_mem_release,%d\n",sel);
		return -EINVAL;
	}

	g2d_free((void *)g2d_mem[sel].virt_addr,(void *)g2d_mem[sel].phy_addr,g2d_mem[sel].mem_len);
	memset(&g2d_mem[sel],0,sizeof(struct info_mem));

	return 0;
}

int g2d_mmap(struct file *file, struct vm_area_struct * vma)
{
	unsigned long mypfn = vma->vm_pgoff;
	unsigned long vmsize = vma->vm_end-vma->vm_start;
	vma->vm_pgoff = 0;

	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	if (remap_pfn_range(vma,vma->vm_start,mypfn,vmsize,vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}

static int g2d_open(struct inode *inode, struct file *file)
{
	mutex_lock(&para.mutex);
	if (!para.opened) {
		if (para.clk)
			clk_prepare_enable(para.clk);
		para.opened = true;
	}
	mutex_unlock(&para.mutex);
	return 0;
}

static int g2d_release(struct inode *inode, struct file *file)
{
	mutex_lock(&para.mutex);
	if (para.opened) {
		if (para.clk)
			clk_disable(para.clk);
		para.opened = false;
	}
	mutex_unlock(&para.mutex);

	mutex_lock(&global_lock);
	scan_order = G2D_SM_TDLR;
	mutex_unlock(&global_lock);
	return 0;
}

irqreturn_t g2d_handle_irq(int irq, void *dev_id)
{
	__u32 mod_irq_flag,cmd_irq_flag;

	mod_irq_flag = mixer_get_irq();
	cmd_irq_flag = mixer_get_irq0();
	if(mod_irq_flag & G2D_FINISH_IRQ)
	{
		mixer_clear_init();
		g2d_ext_hd.finish_flag = 1;
		wake_up(&g2d_ext_hd.queue);
	}
	else if(cmd_irq_flag & G2D_FINISH_IRQ)
	{
		mixer_clear_init0();
		g2d_ext_hd.finish_flag = 1;
		wake_up(&g2d_ext_hd.queue);
	}

	return IRQ_HANDLED;
}

int g2d_init(g2d_init_para *para)
{
	mixer_set_reg_base(para->g2d_base);

	return 0;
}

int g2d_exit(void)
{
	__u8 err = 0;

	return err;
}

int g2d_wait_cmd_finish(void)
{
	long timeout = 100; /* 100ms */

	timeout = wait_event_timeout(g2d_ext_hd.queue, g2d_ext_hd.finish_flag == 1, msecs_to_jiffies(timeout));
	if(timeout == 0)
	{
		mixer_clear_init();
		mixer_clear_init0();
		pr_warn("wait g2d irq pending flag timeout\n");
		g2d_ext_hd.finish_flag = 1;
		wake_up(&g2d_ext_hd.queue);
		return -1;
	}
	return 0;
}

int g2d_blit(g2d_blt * para)
{
	__s32 err = 0;
	__u32 tmp_w,tmp_h;

	if ((para->flag & G2D_BLT_ROTATE90) || (para->flag & G2D_BLT_ROTATE270)){tmp_w = para->src_rect.h;tmp_h = para->src_rect.w;}
	else {tmp_w = para->src_rect.w;tmp_h = para->src_rect.h;}
	/* check the parameter valid */
	if(((para->src_rect.x < 0)&&((-para->src_rect.x) > para->src_rect.w)) ||
	   ((para->src_rect.y < 0)&&((-para->src_rect.y) > para->src_rect.h)) ||
	   ((para->dst_x < 0)&&((-para->dst_x) > tmp_w)) ||
	   ((para->dst_y < 0)&&((-para->dst_y) > tmp_h)) ||
	   ((para->src_rect.x > 0)&&(para->src_rect.x > para->src_image.w - 1)) ||
	   ((para->src_rect.y > 0)&&(para->src_rect.y > para->src_image.h - 1)) ||
	   ((para->dst_x > 0)&&(para->dst_x > para->dst_image.w - 1)) ||
	   ((para->dst_y > 0)&&(para->dst_y > para->dst_image.h - 1)))
	{
		printk("invalid blit parameter setting");
		return -EINVAL;
	}
	else
	{
		if(((para->src_rect.x < 0)&&((-para->src_rect.x) < para->src_rect.w)))
		{
			para->src_rect.w = para->src_rect.w + para->src_rect.x;
			para->src_rect.x = 0;
		}
		else if((para->src_rect.x + para->src_rect.w) > para->src_image.w)
		{
			para->src_rect.w = para->src_image.w - para->src_rect.x;
		}
		if(((para->src_rect.y < 0)&&((-para->src_rect.y) < para->src_rect.h)))
		{
			para->src_rect.h = para->src_rect.h + para->src_rect.y;
			para->src_rect.y = 0;
		}
		else if((para->src_rect.y + para->src_rect.h) > para->src_image.h)
		{
			para->src_rect.h = para->src_image.h - para->src_rect.y;
		}

		if(((para->dst_x < 0)&&((-para->dst_x) < tmp_w)))
		{
			para->src_rect.w = tmp_w + para->dst_x;
			para->src_rect.x = (-para->dst_x);
			para->dst_x = 0;
		}
		else if((para->dst_x + tmp_w) > para->dst_image.w)
		{
			para->src_rect.w = para->dst_image.w - para->dst_x;
		}
		if(((para->dst_y < 0)&&((-para->dst_y) < tmp_h)))
		{
			para->src_rect.h = tmp_h + para->dst_y;
			para->src_rect.y = (-para->dst_y);
			para->dst_y = 0;
		}
		else if((para->dst_y + tmp_h) > para->dst_image.h)
		{
			para->src_rect.h = para->dst_image.h - para->dst_y;
		}
	}

	g2d_ext_hd.finish_flag = 0;

	/* Add support inverted order copy, however,
	 * hardware have a bug when reciving y coordinate,
	 * it use (y + height) rather than (y) on inverted
	 * order mode, so here adjust it before pass it to hardware.
	 */
	mutex_lock(&global_lock);
	if (scan_order > G2D_SM_TDRL)
		para->dst_y += para->src_rect.h;
	mutex_unlock(&global_lock);

	err = mixer_blt(para, scan_order);

	return err;
}

int g2d_fill(g2d_fillrect * para)
{
	__s32 err = 0;

	/* check the parameter valid */
	if(((para->dst_rect.x < 0)&&((-para->dst_rect.x)>para->dst_rect.w)) ||
	   ((para->dst_rect.y < 0)&&((-para->dst_rect.y)>para->dst_rect.h)) ||
	   ((para->dst_rect.x > 0)&&(para->dst_rect.x > para->dst_image.w - 1)) ||
	   ((para->dst_rect.y > 0)&&(para->dst_rect.y > para->dst_image.h - 1)))
	{
		printk("invalid fillrect parameter setting");
		return -EINVAL;
	}
	else
	{
		if(((para->dst_rect.x < 0)&&((-para->dst_rect.x) < para->dst_rect.w)))
		{
			para->dst_rect.w = para->dst_rect.w + para->dst_rect.x;
			para->dst_rect.x = 0;
		}
		else if((para->dst_rect.x + para->dst_rect.w) > para->dst_image.w)
		{
			para->dst_rect.w = para->dst_image.w - para->dst_rect.x;
		}
		if(((para->dst_rect.y < 0)&&((-para->dst_rect.y) < para->dst_rect.h)))
		{
			para->dst_rect.h = para->dst_rect.h + para->dst_rect.y;
			para->dst_rect.y = 0;
		}
		else if((para->dst_rect.y + para->dst_rect.h) > para->dst_image.h)
		{
			para->dst_rect.h = para->dst_image.h - para->dst_rect.y;
		}
	}

	g2d_ext_hd.finish_flag = 0;
	err = mixer_fillrectangle(para);

	return err;
}

int g2d_stretchblit(g2d_stretchblt * para)
{
	__s32 err = 0;

	/* check the parameter valid */
	if(((para->src_rect.x < 0)&&((-para->src_rect.x) > para->src_rect.w)) ||
	   ((para->src_rect.y < 0)&&((-para->src_rect.y) > para->src_rect.h)) ||
	   ((para->dst_rect.x < 0)&&((-para->dst_rect.x) > para->dst_rect.w)) ||
	   ((para->dst_rect.y < 0)&&((-para->dst_rect.y) > para->dst_rect.h)) ||
	   ((para->src_rect.x > 0)&&(para->src_rect.x > para->src_image.w - 1)) ||
	   ((para->src_rect.y > 0)&&(para->src_rect.y > para->src_image.h - 1)) ||
	   ((para->dst_rect.x > 0)&&(para->dst_rect.x > para->dst_image.w - 1)) ||
	   ((para->dst_rect.y > 0)&&(para->dst_rect.y > para->dst_image.h - 1)))
	{
		printk("invalid stretchblit parameter setting");
		return -EINVAL;
	}
	else
	{
		if(((para->src_rect.x < 0)&&((-para->src_rect.x) < para->src_rect.w)))
		{
			para->src_rect.w = para->src_rect.w + para->src_rect.x;
			para->src_rect.x = 0;
		}
		else if((para->src_rect.x + para->src_rect.w) > para->src_image.w)
		{
			para->src_rect.w = para->src_image.w - para->src_rect.x;
		}
		if(((para->src_rect.y < 0)&&((-para->src_rect.y) < para->src_rect.h)))
		{
			para->src_rect.h = para->src_rect.h + para->src_rect.y;
			para->src_rect.y = 0;
		}
		else if((para->src_rect.y + para->src_rect.h) > para->src_image.h)
		{
			para->src_rect.h = para->src_image.h - para->src_rect.y;
		}

		if(((para->dst_rect.x < 0)&&((-para->dst_rect.x) < para->dst_rect.w)))
		{
			para->dst_rect.w = para->dst_rect.w + para->dst_rect.x;
			para->dst_rect.x = 0;
		}
		else if((para->dst_rect.x + para->dst_rect.w) > para->dst_image.w)
		{
			para->dst_rect.w = para->dst_image.w - para->dst_rect.x;
		}
		if(((para->dst_rect.y < 0)&&((-para->dst_rect.y) < para->dst_rect.h)))
		{
			para->dst_rect.h = para->dst_rect.h + para->dst_rect.y;
			para->dst_rect.y = 0;
		}
		else if((para->dst_rect.y + para->dst_rect.h) > para->dst_image.h)
		{
			para->dst_rect.h = para->dst_image.h - para->dst_rect.y;
		}
	}

	g2d_ext_hd.finish_flag = 0;

	/* Add support inverted order copy, however,
	 * hardware have a bug when reciving y coordinate,
	 * it use (y + height) rather than (y) on inverted
	 * order mode, so here adjust it before pass it to hardware.
	 */

	mutex_lock(&global_lock);
	if (scan_order > G2D_SM_TDRL)
		para->dst_rect.y += para->src_rect.h;
	mutex_unlock(&global_lock);

	err = mixer_stretchblt(para, scan_order);

	return err;
}

int g2d_set_palette_table(g2d_palette *para)
{

	if((para->pbuffer == NULL) || (para->size < 0) || (para->size>1024))
	{
		printk("para invalid in mixer_set_palette\n");
		return -1;
	}

	mixer_set_palette(para);

	return 0;
}

int g2d_cmdq(unsigned int para)
{
	__s32 err = 0;

	g2d_ext_hd.finish_flag = 0;
	err = mixer_cmdq(para);

	return err;
}

long g2d_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	__s32	ret = 0;

	if(!mutex_trylock(&para.mutex)) {
			mutex_lock(&para.mutex);
	}
	switch (cmd) {

	/* Proceed to the operation */
	case G2D_CMD_BITBLT:{
		g2d_blt blit_para;
		if(copy_from_user(&blit_para, (g2d_blt *)arg, sizeof(g2d_blt)))
		{
			kfree(&blit_para);
			ret = -EFAULT;
			goto err_noput;
		}
		ret = g2d_blit(&blit_para);
		break;
	}
	case G2D_CMD_FILLRECT:{
		g2d_fillrect fill_para;
		if(copy_from_user(&fill_para, (g2d_fillrect *)arg, sizeof(g2d_fillrect)))
		{
			kfree(&fill_para);
			ret = -EFAULT;
			goto err_noput;
		}
		ret = g2d_fill(&fill_para);
		break;
	}
	case G2D_CMD_STRETCHBLT:{
		g2d_stretchblt stre_para;
		if(copy_from_user(&stre_para, (g2d_stretchblt *)arg, sizeof(g2d_stretchblt)))
		{
			kfree(&stre_para);
			ret = -EFAULT;
			goto err_noput;
		}
		ret = g2d_stretchblit(&stre_para);
		break;
	}
	case G2D_CMD_PALETTE_TBL:{
		g2d_palette pale_para;
		if(copy_from_user(&pale_para, (g2d_palette *)arg, sizeof(g2d_palette)))
		{
			kfree(&pale_para);
			ret = -EFAULT;
			goto err_noput;
		}
		ret = g2d_set_palette_table(&pale_para);
		break;
	}
	case G2D_CMD_QUEUE:{
		unsigned int cmdq_addr;
		if(copy_from_user(&cmdq_addr, (unsigned int *)arg, sizeof(unsigned int)))
		{
			kfree(&cmdq_addr);
			ret = -EFAULT;
			goto err_noput;
		}
		ret = g2d_cmdq(cmdq_addr);
		break;
	}

	/* just management memory for test */
	case G2D_CMD_MEM_REQUEST:
		ret =  g2d_mem_request(arg);
		break;

	case G2D_CMD_MEM_RELEASE:
		ret =  g2d_mem_release(arg);
		break;

	case G2D_CMD_MEM_SELIDX:
		g2d_mem_sel = arg;
		break;

	case G2D_CMD_MEM_GETADR:
		if(g2d_mem[arg].b_used)
		{
			ret = g2d_mem[arg].phy_addr;
		}
		else
		{
			ERR("mem not used in G2D_CMD_MEM_GETADR\n");
			ret = -1;
		}
		break;

	case G2D_CMD_INVERTED_ORDER:
	{
		if (arg > G2D_SM_DTRL) {
			ERR("scan mode is err.\n");
			ret = -EINVAL;
			goto err_noput;
		}

		mutex_lock(&global_lock);
		scan_order = arg;
		mutex_unlock(&global_lock);
		break;
	}

	/* Invalid IOCTL call */
	default:
		return -EINVAL;
	}

err_noput:
	mutex_unlock(&para.mutex);

	return ret;
}

static struct file_operations g2d_fops = {
	.owner				= THIS_MODULE,
	.open				= g2d_open,
	.release			= g2d_release,
	.unlocked_ioctl		= g2d_ioctl,
	.mmap				= g2d_mmap,
};

static int g2d_probe(struct platform_device *pdev)
{
#if !defined(CONFIG_OF)
	int size;
	struct resource	*res;
#endif
	int	ret = 0;
	__g2d_info_t	*info = NULL;

	info = &para;
	info->dev = &pdev->dev;
	platform_set_drvdata(pdev,info);

#if !defined(CONFIG_OF)
	/* get the memory region */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(res == NULL)
	{
		ERR("failed to get memory register\n");
		ret = -ENXIO;
		goto  dealloc_fb;
	}

	size = (res->end - res->start) + 1;
	/* map the memory */
	info->io = ioremap(res->start, size);
	if(info->io == NULL)
	{
		ERR("iormap() of register failed\n");
		ret = -ENXIO;
		goto  dealloc_fb;
	}
#else
	info->io = of_iomap(pdev->dev.of_node, 0);
	if(info->io == NULL)
	{
		ERR("iormap() of register failed\n");
		ret = -ENXIO;
		goto  dealloc_fb;
	}
#endif

#if !defined(CONFIG_OF)
	/* get the irq */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if(res == NULL)
	{
		ERR("failed to get irq resource\n");
		ret = -ENXIO;
		goto release_regs;
	}
	info->irq = res->start;
#else
	info->irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
	if (!info->irq) {
		ERR("irq_of_parse_and_map irq fail for transform\n");
		ret = -ENXIO;
		goto release_regs;
	}
#endif

	/* request the irq */
	ret = request_irq(info->irq,g2d_handle_irq,0,dev_name(&pdev->dev),NULL);
	if(ret)
	{
		ERR("failed to install irq resource\n");
		goto release_regs;
	}
#if defined(CONFIG_OF)
	/* clk init */
	info->clk = of_clk_get(pdev->dev.of_node, 0);
	if (IS_ERR(info->clk)) {
		ERR("fail to get clk\n");
	}
#endif

	drv_g2d_init();
	mutex_init(&info->mutex);
	mutex_init(&global_lock);
	return 0;

release_regs:
#if !defined(CONFIG_OF)
		iounmap(info->io);
#endif
dealloc_fb:
		platform_set_drvdata(pdev, NULL);

	return ret;
}

static int g2d_remove(struct platform_device *pdev)
{
	__g2d_info_t *info = platform_get_drvdata(pdev);

	free_irq(info->irq, NULL);
#if !defined(CONFIG_OF)
	iounmap(info->io);
#endif
	platform_set_drvdata(pdev, NULL);

	INFO("Driver unloaded succesfully.\n");
	return 0;
}

static int g2d_suspend(struct platform_device *pdev, pm_message_t state)
{
	INFO("%s. \n", __func__);
	mutex_lock(&para.mutex);
	if (para.opened) {
		if (para.clk)
			clk_disable(para.clk);
	}
	mutex_unlock(&para.mutex);
	INFO("g2d_suspend succesfully.\n");

	return 0;
}

static int g2d_resume(struct platform_device *pdev)
{
	INFO("%s. \n", __func__);
	mutex_lock(&para.mutex);
	if (para.opened) {
		if (para.clk)
			clk_prepare_enable(para.clk);
	}
	mutex_unlock(&para.mutex);
	INFO("g2d_resume succesfully.\n");

	return 0;
}

#if !defined(CONFIG_OF)
struct platform_device g2d_device =
{
	.name		= "g2d",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(g2d_resource),
	.resource	= g2d_resource,
	.dev		=
	{
	},
};
#else
static const struct of_device_id sunxi_g2d_match[] = {
	{ .compatible = "allwinner,sunxi-g2d", },
	{},
};
#endif

static struct platform_driver g2d_driver = {
	.probe		= g2d_probe,
	.remove		= g2d_remove,
	.suspend	= g2d_suspend,
	.resume		= g2d_resume,
	.suspend	= NULL,
	.resume		= NULL,
	.driver			=
	{
		.owner		= THIS_MODULE,
		.name		= "g2d",
		.of_match_table	= sunxi_g2d_match,
	},
};

int __init g2d_module_init(void)
{
	int ret = 0, err;

	alloc_chrdev_region(&devid, 0, 1, "g2d_chrdev");
	g2d_cdev = cdev_alloc();
	cdev_init(g2d_cdev, &g2d_fops);
	g2d_cdev->owner = THIS_MODULE;
	err = cdev_add(g2d_cdev, devid, 1);
	if (err)
	{
		ERR("I was assigned major number %d.\n", MAJOR(devid));
		return -1;
	}

	g2d_class = class_create(THIS_MODULE, "g2d_class");
	if (IS_ERR(g2d_class))
	{
		ERR("create class error\n");
		return -1;
	}

	device_create(g2d_class, NULL, devid, NULL, "g2d");
#if !defined(CONFIG_OF)
	ret = platform_device_register(&g2d_device);
#endif
	if (ret == 0)
	{
		ret = platform_driver_register(&g2d_driver);
	}

	INFO("Module initialized.major:%d\n", MAJOR(devid));
	return ret;
}

static void __exit g2d_module_exit(void)
{
	INFO("g2d_module_exit\n");
	kfree(g2d_ext_hd.g2d_finished_sem);

	platform_driver_unregister(&g2d_driver);
#if !defined(CONFIG_OF)
	platform_device_unregister(&g2d_device);
#endif
	device_destroy(g2d_class,  devid);
	class_destroy(g2d_class);

	cdev_del(g2d_cdev);
}

module_init(g2d_module_init);
module_exit(g2d_module_exit);

MODULE_AUTHOR("yupu_tang");
MODULE_AUTHOR("tyle <tyle@allwinnertech.com>");
MODULE_DESCRIPTION("g2d driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:g2d");

