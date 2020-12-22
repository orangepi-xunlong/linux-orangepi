#include "g2d.h"
#include <linux/clk.h>
#include <linux/clk/sunxi.h>
#include <linux/clk/sunxi_name.h>
#include <linux/interrupt.h>
#include "g2d_driver_i.h"

static struct clk *g2d_mclk = NULL;
static struct clk *g2d_src = NULL;
extern __g2d_drv_t	 g2d_ext_hd;

int g2d_openclk(void)
{
	__u32 ret;
		
	/* g2d gating */
	g2d_mclk = clk_get(NULL,MP_CLK);
	if(NULL == g2d_mclk || IS_ERR(g2d_mclk))
	{
		printk("%s err: clk_get %s failed\n", __func__, "mp");
		return -EPERM;
	}

	/*disable mp clk reset*/
	sunxi_periph_reset_assert(g2d_mclk);
	
	/* set g2d clk value */
	g2d_src = clk_get(NULL,PLL10_CLK);//pll3/7/9/10
	if(NULL == g2d_src || IS_ERR(g2d_src))
	{
		printk("%s err: clk_get %s failed\n", __func__, "pll10");
		return -EPERM;
	}

	if(clk_set_parent(g2d_mclk, g2d_src))
	{
		printk("%s:set g2d_mclk parent to sys_pll10 failed!\n",__func__);
	}
	ret = clk_get_rate(g2d_src);
	ret = ret/2;
	if(clk_set_rate(g2d_mclk,500000000))
		printk("%s:set g2d_mclk rate to %dHZ failed!\n",__func__,ret);
	clk_put(g2d_src);
		
	return 0;
}

int g2d_closeclk(void)/* used once when g2d driver exit */
{	
	if(NULL == g2d_mclk || IS_ERR(g2d_mclk))
	{
		printk("g2d_mclk handle is invalid,just return!\n");
		return 0;
	}
	else
	{
//		clk_disable_unprepare(g2d_mclk);
		clk_put(g2d_mclk);
		g2d_mclk = NULL;
	}

	return 0;
}

int g2d_clk_on(void)/* used in request */
{	
	if(0 != clk_prepare_enable(g2d_mclk))
	{
		printk("%s err: clk_enable failed, line %d\n", __func__, __LINE__);
		return -EPERM;
	}

	return  0;
}

int g2d_clk_off(void)/* used in release */
{
	if(NULL == g2d_mclk || IS_ERR(g2d_mclk))
	{
		printk("g2d_mclk handle is invalid,just return!\n");
		return 0;
	}
	else
	{
		clk_disable_unprepare(g2d_mclk);
	}
	
	return  0;
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
	g2d_closeclk();
	
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
		printk("wait g2d irq pending flag timeout\n");
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
	err = mixer_blt(para);
   	
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
	err = mixer_stretchblt(para);
   	
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

