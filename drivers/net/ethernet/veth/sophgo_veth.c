#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/irqreturn.h>
#include <linux/mod_devicetable.h>
#include <linux/spinlock.h>
#include <linux/of.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/string.h>

#include "sophgo_common.h"
#include "sophgo_veth.h"

#define VETH_IRQ

static int set_ready_flag(struct veth_dev *vdev);

static inline void intr_clear(struct veth_dev *vdev)
{
	sg_write32(vdev->top_misc_reg, TOP_MISC_GP_REG31_CLR_OFFSET, SOPH_VETH_RX_IRQ_CLR_BIT);
}

static irqreturn_t veth_irq(int irq, void *id)
{
	struct veth_dev *vdev = id;

	if (atomic_read(&vdev->link)) {
		if (pt_load_rx(vdev->pt)) {
			napi_schedule(&vdev->napi);
		}
	}
	intr_clear(vdev);
	return IRQ_HANDLED;
}

static void sg_enable_eth_irq(struct veth_dev *vdev)
{
	u32 intc_enable;
	u32 intc_mask;

	//intc 63[40]
	intc_enable = sg_read32(vdev->intc_cfg_reg, 0x4);
	intc_enable |= (1 << 31);
	sg_write32(vdev->intc_cfg_reg, 0x4, intc_enable);
	intc_enable = sg_read32(vdev->intc_cfg_reg, 0x2004);
	intc_enable |= (1 << 8);
	sg_write32(vdev->intc_cfg_reg, 0x2004, intc_enable);
	intc_mask = sg_read32(vdev->intc_cfg_reg, 0xc);
	intc_mask &= ~(1 << 31);
	sg_write32(vdev->intc_cfg_reg, 0xc, intc_mask);
	intc_mask = sg_read32(vdev->intc_cfg_reg, 0x200c);
	intc_mask &= ~(1 << 8);
	sg_write32(vdev->intc_cfg_reg, 0x200c, intc_mask);
}

static int sg_eth_change_mtu(struct net_device *ndev, int new_mtu)
{
        if (netif_running(ndev))
                return -EBUSY;

        ndev->mtu = new_mtu;
        netdev_update_features(ndev);

        return 0;
}

static int notify_host(struct veth_dev *vdev)
{
#ifdef VETH_IRQ
	if (atomic_read(&vdev->link)) {
		sg_write32(vdev->top_misc_reg, TOP_MISC_GP_REG30_SET_OFFSET, SOPH_VETH_TX_IRQ_SET_BIT);
	}
	sg_enable_eth_irq(vdev);
#else
	u32 data;
	if (atomic_read(&vdev->link)) {
		data = 0x1;
		sg_write32(vdev->shm_cfg_reg, 0x60, data);
	}
#endif
	return NETDEV_TX_OK;
}

static int veth_open(struct net_device *ndev)
{
	struct veth_dev *vdev = netdev_priv(ndev);
	int err;

	intr_clear(vdev);
	if (devm_request_irq(&vdev->pdev->dev, vdev->rx_irq, veth_irq, IRQF_ONESHOT, "veth", vdev)) {
		pr_err("request rx irq failed!\n");
		return 1;
	}
	sg_enable_eth_irq(vdev);
	disable_irq(vdev->rx_irq);
	atomic_set(&vdev->link, false);
	err = set_ready_flag(vdev);
	if (err) {
		devm_free_irq(&vdev->pdev->dev, vdev->rx_irq, vdev);
		pr_err("set ready falg failed!\n");
		return 1;
	}
	netdev_reset_queue(ndev);
	netif_start_queue(ndev);
	napi_enable(&vdev->napi);

	return 0;
}

static int veth_close(struct net_device *ndev)
{
	struct veth_dev *vdev = netdev_priv(ndev);

	disable_irq(vdev->rx_irq);
	napi_disable(&vdev->napi);
	netif_stop_queue(ndev);
	devm_free_irq(&vdev->pdev->dev, vdev->rx_irq, vdev);
	return 0;
}

static netdev_tx_t veth_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	struct veth_dev *vdev = netdev_priv(ndev);
	int err;
	int ret;

	if (!atomic_read(&vdev->link)) {
		dev_kfree_skb_any(skb);
		return NETDEV_TX_OK;
	}

	pt_load_tx(vdev->pt);
	err = pt_send(vdev->pt, skb->data, skb->len);
	if (err < 0) {
		notify_host(vdev);
		return NETDEV_TX_BUSY;
	}

	ret = pt_store_tx(vdev->pt);
	if (ret != NETDEV_TX_OK) {
		ndev->stats.tx_errors += err;
		return NETDEV_TX_BUSY;
	}

	ret = notify_host(vdev);
	if (ret == NETDEV_TX_BUSY)
		return ret;

	++ndev->stats.tx_packets;
	ndev->stats.tx_bytes += err;
	dev_kfree_skb_any(skb);
	return NETDEV_TX_OK;
}

static const struct net_device_ops veth_ops = {
	.ndo_open = veth_open,
	.ndo_stop = veth_close,
	.ndo_start_xmit = veth_xmit,
	.ndo_set_mac_address = eth_mac_addr,
	.ndo_validate_addr = eth_validate_addr,
	.ndo_change_mtu = sg_eth_change_mtu,
};

static int veth_rx(struct veth_dev *vdev, int limit)
{
	int err;
	struct net_device *ndev;
	struct sk_buff *skb;
	int pkg_len;
	int pkg_cnt;
	struct pt *pt;

	ndev = vdev->ndev;
	pt = vdev->pt;
	pkg_cnt = 0;
	if (!atomic_read(&vdev->link)) {
		pr_info("veth rx not link!\n");
		return pkg_cnt;
	}

	while (pkg_cnt < limit) {
		pkg_len = pt_pkg_len(pt);
		if (pkg_len < 0) {
			napi_complete_done(&vdev->napi, pkg_cnt);
			break;
		}

		skb = netdev_alloc_skb(ndev, pkg_len + NET_IP_ALIGN);
		if (!skb) {
			// pr_err("allocate skb failed with package length %u\n", pkg_len);
			pt_info(pt);
			++ndev->stats.rx_dropped;
			continue;
		}

		skb_reserve(skb, NET_IP_ALIGN);
		err = pt_recv(pt, skb_put(skb, pkg_len), pkg_len);
		/* free space as soon as possible */
		skb->protocol = eth_type_trans(skb, ndev);
		napi_gro_receive(&vdev->napi, skb);
		++ndev->stats.rx_packets;
		ndev->stats.rx_bytes += err;

		pkg_cnt++;
	}

	pt_store_rx(pt);
	return pkg_cnt;
}

static int veth_napi_poll_rx(struct napi_struct *napi, int budget)
{
	struct veth_dev *vdev;
	int err;

	vdev = container_of(napi, struct veth_dev, napi);
	err = veth_rx(vdev, budget);

	return err;
}

static void net_state_work(struct work_struct *worker)
{
	struct veth_dev *vdev;
	struct net_device *ndev;
	struct device *dev;

	vdev = container_of(worker, struct veth_dev, net_state_worker);
	ndev = vdev->ndev;
	dev = &vdev->pdev->dev;

	sg_write32(vdev->shm_cfg_reg, SHM_HANDSHAKE_OFFSET, DEVICE_READY_FLAG);

	while (sg_read32(vdev->shm_cfg_reg, SHM_HANDSHAKE_OFFSET) != HOST_READY_FLAG)
		mdelay(10);
	/* i am ready again */
	sg_write32(vdev->shm_cfg_reg, SHM_HANDSHAKE_OFFSET, DEVICE_READY_FLAG);
	vdev->pt = pt_init(dev, vdev->shm_cfg_reg, vdev);
	if (!vdev->pt) {
		pr_err("package transmiter init failed, try again\n");
		schedule_work(worker);
		return;
	}
	spin_lock_init(&vdev->lock_cdma);
	atomic_set(&vdev->link, true);
	netif_carrier_on(ndev);
	enable_irq(vdev->rx_irq);
	pr_info("connect success!\n");
}

static int set_ready_flag(struct veth_dev *vdev)
{
	if (sg_read32(vdev->shm_cfg_reg, SHM_HANDSHAKE_OFFSET)) {
		pr_err("handshake register not clear!\n");
		return -EINVAL;
	}

	INIT_WORK(&vdev->net_state_worker, net_state_work);
	schedule_work(&vdev->net_state_worker);

	return 0;
}

static void unset_ready_flag(struct veth_dev *vdev)
{
	sg_write32(vdev->shm_cfg_reg, SHM_HANDSHAKE_OFFSET, 0);
}

static int sg_veth_get_resource(struct platform_device *pdev, struct veth_dev *vdev)
{
	int err;

	vdev->shm_cfg_reg = devm_platform_ioremap_resource_byname(pdev, "shm_reg");
	if (!vdev->shm_cfg_reg) {
		pr_err("map shm cfg reg failed!\n");
		err = -ENOMEM;
		return err;
	}

	vdev->top_misc_reg = devm_platform_ioremap_resource_byname(pdev, "top_misc");
	if (!vdev->top_misc_reg) {
		pr_err("map top misc reg failed!\n");
		err = -ENOMEM;
		return err;
	}

	vdev->cdma_cfg_reg = devm_platform_ioremap_resource_byname(pdev, "cdma_cfg");
	if (!vdev->cdma_cfg_reg) {
		pr_err("map cdma cfg reg failed!\n");
		err = -ENOMEM;
		return err;
	}

	vdev->intc_cfg_reg = devm_platform_ioremap_resource_byname(pdev, "intc_cfg");
	if (!vdev->intc_cfg_reg) {
		pr_err("map intc cfg reg failed!\n");
		err = -ENOMEM;
		return err;
	}

	vdev->rx_irq = platform_get_irq_byname(pdev, "rx");
	if (vdev->rx_irq < 0) {
		pr_err("no rx interrupt resource!\n");
		err = -ENOMEM;
		return err;
	}

	// set the veth's initial status to 0
	sg_write32(vdev->shm_cfg_reg, VETH_CTRL_STATUS, 0);

	return 0;
}

#define VETH_MODE_BASE 0x28100004
#define VETH_SEL_0 BIT(1)
static bool sg_veth_init_mode_check(unsigned int index)
{
        void __iomem * addr = ioremap(VETH_MODE_BASE, 0x1);
        uint32_t val = readl(addr) >> 25;

        if (index == 0)
                val &= VETH_SEL_0;
        else
                val = 0;

        return val ? true : false;
}

static ssize_t ipaddr_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct net_device *ndev = to_net_dev(dev);
    struct veth_dev *vdev = netdev_priv(ndev);
    u32 val = sg_read32(vdev->shm_cfg_reg, VETH_IPADDR_OFFSET);
    return sprintf(buf, "%08x\n", val);
}

static ssize_t ipaddr_store(struct device *dev, struct device_attribute *attr,
                        const char *buf, size_t count)
{
    struct net_device *ndev = to_net_dev(dev);
    struct veth_dev *vdev = netdev_priv(ndev);
    u32 val;

    if (kstrtouint(buf, 10, &val))
        return -EINVAL;

    sg_write32(vdev->shm_cfg_reg, VETH_IPADDR_OFFSET, val);
    return count;
}

static ssize_t mask_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct net_device *ndev = to_net_dev(dev);
    struct veth_dev *vdev = netdev_priv(ndev);
    u32 val = sg_read32(vdev->shm_cfg_reg, VETH_MASK_OFFSET);
    return sprintf(buf, "%08x\n", val);
}

static ssize_t mask_store(struct device *dev, struct device_attribute *attr,
                        const char *buf, size_t count)
{
    struct net_device *ndev = to_net_dev(dev);
    struct veth_dev *vdev = netdev_priv(ndev);
    u32 val;

    if (kstrtouint(buf, 10, &val))
        return -EINVAL;

    sg_write32(vdev->shm_cfg_reg, VETH_MASK_OFFSET, val);
    return count;
}

static ssize_t mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct net_device *ndev = to_net_dev(dev);
	struct veth_dev *vdev = netdev_priv(ndev);
	u32 val = sg_read32(vdev->top_misc_reg, 0x4);
	return sprintf(buf, "%08x\n", val);
}

static ssize_t ctrl_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct net_device *ndev = to_net_dev(dev);
	struct veth_dev *vdev = netdev_priv(ndev);
	u32 val = sg_read32(vdev->shm_cfg_reg, VETH_CTRL_STATUS);
	return sprintf(buf, "%08x\n", val);
}

static ssize_t ctrl_status_store(struct device *dev, struct device_attribute *attr,
                        const char *buf, size_t count)
{
    struct net_device *ndev = to_net_dev(dev);
    struct veth_dev *vdev = netdev_priv(ndev);
    u32 val;

    if (kstrtouint(buf, 10, &val))
        return -EINVAL;

    sg_write32(vdev->shm_cfg_reg, VETH_CTRL_STATUS, val);
    return count;
}

static ssize_t link_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct net_device *ndev = to_net_dev(dev);
	struct veth_dev *vdev = netdev_priv(ndev);
	u32 val = sg_read32(vdev->shm_cfg_reg, SHM_HANDSHAKE_OFFSET);
	return sprintf(buf, "%08x\n", val);
}

static ssize_t gw_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct net_device *ndev = to_net_dev(dev);
    struct veth_dev *vdev = netdev_priv(ndev);
    u32 val = sg_read32(vdev->shm_cfg_reg, VETH_GW_OFFSET);
    return sprintf(buf, "%08x\n", val);
}

static ssize_t gw_store(struct device *dev, struct device_attribute *attr,
                        const char *buf, size_t count)
{
    struct net_device *ndev = to_net_dev(dev);
    struct veth_dev *vdev = netdev_priv(ndev);
    u32 val;

    if (kstrtouint(buf, 10, &val))
        return -EINVAL;

    sg_write32(vdev->shm_cfg_reg, VETH_GW_OFFSET, val);
    return count;
}

DEVICE_ATTR_RW(ipaddr);
DEVICE_ATTR_RW(mask);
DEVICE_ATTR_RO(mode);
DEVICE_ATTR_RW(ctrl_status);
DEVICE_ATTR_RO(link_status);
DEVICE_ATTR_RW(gw);

static struct attribute *sg_veth_attrs[] = {
	&dev_attr_ipaddr.attr,
	&dev_attr_mask.attr,
	&dev_attr_mode.attr,
	&dev_attr_ctrl_status.attr,
	&dev_attr_link_status.attr,
	&dev_attr_gw.attr,
	NULL
};

static struct attribute_group sg_veth_attr_group = {
	.name = "params",
	.attrs = sg_veth_attrs,
};

static int sg_veth_probe(struct platform_device *pdev)
{
	struct device *dev;
	struct veth_dev *vdev;
	struct net_device *ndev;
	int err;
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, "sophgon, veth");
	if (!of_device_is_available(node)) {
		pr_err("veth driver node is disable!\n");
		err = -ENODEV;
		return err;
	}

	if (!sg_veth_init_mode_check(0)) {
		pr_info("veth not enabled.\n");
		return -ENODEV;
	}

	dev = &pdev->dev;
	dma_set_mask(dev, DMA_BIT_MASK(64));
	ndev = alloc_etherdev(sizeof(struct veth_dev));
	if (!ndev) {
		pr_err("cannot allocate device instance!\n");
		return -ENOMEM;
	}

	ether_setup(ndev);
	strncpy(ndev->name, "veth%d", IFNAMSIZ);
	eth_hw_addr_random(ndev);

	vdev = netdev_priv(ndev);
	vdev->ndev = ndev;

	err = sg_veth_get_resource(pdev, vdev);
	if (err != 0)
		goto err_free_netdev;

	vdev->pdev = pdev;
	platform_set_drvdata(pdev, ndev);

	ndev->netdev_ops = &veth_ops;
	ndev->irq = vdev->rx_irq;
	ndev->mtu = VETH_DEFAULT_MTU;
	netif_napi_add(ndev, &vdev->napi, veth_napi_poll_rx, NAPI_POLL_WEIGHT);
	err = register_netdev(ndev);
	if (err) {
		pr_err("register net device failed!\n");
		goto err_free_netdev;
	}

	err = sysfs_create_group(&ndev->dev.kobj, &sg_veth_attr_group);
	if (err) {
		pr_err("failed to create sysfs group!\n");
	}

	return 0;

err_free_netdev:
	free_netdev(ndev);
	return err;
}

static int sg_veth_remove(struct platform_device *pdev)
{
	struct veth_dev *vdev;
	struct net_device *ndev;

	ndev = platform_get_drvdata(pdev);

	netif_carrier_off(ndev);
	if (!ndev)
		return -ENODEV;

	sysfs_remove_group(&ndev->dev.kobj, &sg_veth_attr_group);

	vdev = netdev_priv(ndev);
	disable_irq(vdev->rx_irq);

	unset_ready_flag(vdev);
	return 0;
}

static const struct of_device_id sg_veth_match[] = {
	{.compatible = "sophgon, veth"},
	{},
};

static struct platform_driver sg_veth_driver = {
	.probe = sg_veth_probe,
	.remove = sg_veth_remove,
	.driver = {
		.name = "veth",
		.of_match_table = sg_veth_match,
	},
};

module_platform_driver(sg_veth_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dong Yang <dong.yang@sophgon.com>");
MODULE_DESCRIPTION("Sophgon Virtual Ethernet Driver over PCIe");
