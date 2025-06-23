// SPDX-License-Identifier: GPL-2.0
/*
 * PD driver for typec connection
 */

#include <linux/bits.h>
#include <linux/bitfield.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regmap.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/usb/pd.h>
#include <linux/usb/typec.h>
#include <linux/usb/typec_altmode.h>
#include <linux/usb/typec_dp.h>
#include <linux/usb/typec_mux.h>
#include <linux/usb/typec_tbt.h>
#include <linux/usb/role.h>
#include <linux/regulator/consumer.h>
#include <linux/acpi.h>
#include "class.h"

#define RTS_FIELD_GET(_mask, _reg) ((typeof(_mask))((_reg) & (_mask)) >> __bf_shf(_mask))
#define RTS5453_REG_VID		0x00
#define RTS5453_REG_PID		0x00
#define RTS_DATA_CONTROL 0x50
#define I2C_INT_ACK_BIT BIT(2)
#define HPD_IRQ_ACK BIT(5)
#define RTS_DATA_STATUS	 0x5F
//byte 1
#define CONNECTION_ORIENTATION  BIT(1)
#define USB2_CONNECTION BIT(4)
#define USB3_2_CONNECTION BIT(5)
#define USB_DATA_ROLE BIT(7)
//byte 2
#define DP_CONNECTION BIT(0)
#define DP_SOURCE_SINK BIT(1)
#define DEBUG_ACCESSORY_ATTACHED BIT(4)
#define HPD_IRQ BIT(6)
#define HPD_STATE BIT(7)
//byte 3
#define POWER_SOURCE BIT(0)
#define POWER_SINK BIT(1)
#define NO_POWER 0
#define POWER_MASK 3

#define DATA_STATUS_COUNT 6

enum {
	ALTMODE_DP = 0,
	ALTMODE_MAX,
};

struct rts5453h {
	struct device *dev;
	struct regmap *regmap;
	struct mutex lock; /* device lock */
	u8 i2c_protocol:1;	/*i2c or smbus*/
	int id;
	u8 data_status[DATA_STATUS_COUNT];
	struct typec_mux_state state;
	struct typec_capability caps;
	struct typec_switch *ori_sw;
	struct typec_mux *mux;
	struct usb_role_switch *role_sw;
	struct typec_altmode p_altmode[ALTMODE_MAX];
	struct typec_port *port;
	enum usb_role role;
	unsigned long mode;
	u32 hpd_status;
	struct typec_partner	*partner;
	struct typec_partner_desc desc;
};

#define RTS_MAX_LEN	64

static int
rts5453h_block_read(struct rts5453h *rts, u8 reg, void *val, size_t len)
{
	return regmap_bulk_read(rts->regmap, reg, val, len);
}
static int rts5453h_block_write(struct rts5453h *rts, u8 reg,
				const void *val, size_t len)
{
	return regmap_bulk_write(rts->regmap, reg, val, len);
}
static inline int rts5453h_read16(struct rts5453h *rts, u8 reg, u16 *val)
{
	return rts5453h_block_read(rts, reg, val, sizeof(u16));
}
static inline int rts5453h_write16(struct rts5453h *rts, u8 reg, u16 val)
{
	return rts5453h_block_write(rts, reg, &val, sizeof(u16));
}
static inline int rts5453h_read32(struct rts5453h *rts, u8 reg, u32 *val)
{
	return rts5453h_block_read(rts, reg, val, sizeof(u32));
}
static inline int rts5453h_write32(struct rts5453h *rts, u8 reg, u32 val)
{
	return rts5453h_block_write(rts, reg, &val, sizeof(u32));
}

static const struct regmap_config rts5453h_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0x7F,
};

char dp_only_mode[] = "Alt Mode:DP";
char dp_usb_mode[] = "Alt Mode:DP + USB";
char usb_mode[] = "Alt Mode:USB";

char power_source[] = "power source";
char power_sink[] = "power sink";

char oritation_normal[] = "oritation normal";
char oritation_reverse[] = "oritation reverse";

char data_host[] = "usb host";
char data_device[] = "usb device";


static int rts5453h_typec_port_update(struct rts5453h *typec)
{
	enum typec_orientation orientation;
	int ret = 0;
	struct typec_displayport_data dp_data;
	enum usb_role role;

	if (!(typec->data_status[1] & USB3_2_CONNECTION) && !(typec->data_status[2] & DP_CONNECTION)) {
		orientation = TYPEC_ORIENTATION_NONE;
	} else if (typec->data_status[1] & CONNECTION_ORIENTATION) {
		orientation = TYPEC_ORIENTATION_REVERSE;
	} else {
		orientation = TYPEC_ORIENTATION_NORMAL;
	}

	ret = typec_switch_set(typec->ori_sw, orientation);

	if (ret)
		return ret;

	if ((!(typec->data_status[1] & USB3_2_CONNECTION)) && (!(typec->data_status[1] & USB2_CONNECTION))) {
		role = USB_ROLE_NONE;
	} else if (typec->data_status[1] & USB_DATA_ROLE) {
		role = USB_ROLE_DEVICE;
	} else {
		if ((!(typec->data_status[1] & USB3_2_CONNECTION)) && (typec->data_status[2] & DP_CONNECTION)) {
			role = USB_ROLE_HOST_20;
		} else {
			role = USB_ROLE_HOST;
		}
	}

	if (typec->data_status[2] & DEBUG_ACCESSORY_ATTACHED) {
		//debug accessory mode
		dev_err(typec->dev, "debug accessory mode not support, try with another usb cable\n");
		return 0;
	}

	if ((typec->data_status[1] & USB3_2_CONNECTION) &&
		(typec->data_status[2] & DP_CONNECTION)) {
		//usb + dp
		dp_data.status = DP_STATUS_ENABLED;
		if (typec->data_status[2] & HPD_STATE)
			dp_data.status |= DP_STATUS_HPD_STATE;
		if (typec->data_status[2] & HPD_IRQ)
			dp_data.status |= DP_STATUS_IRQ_HPD;

		typec->state.data = &dp_data;
		typec->state.mode = TYPEC_DP_STATE_D;
		typec->state.alt = &typec->p_altmode[ALTMODE_DP];
	} else if (typec->data_status[2] & DP_CONNECTION) {
		//dp only
		dp_data.status = DP_STATUS_ENABLED;
		if (typec->data_status[2] & HPD_STATE)
			dp_data.status |= DP_STATUS_HPD_STATE;
		if (typec->data_status[2] & HPD_IRQ)
			dp_data.status |= DP_STATUS_IRQ_HPD;
		typec->state.data = &dp_data;
		typec->state.mode = TYPEC_DP_STATE_E;
		typec->state.alt = &typec->p_altmode[ALTMODE_DP];
	} else if (typec->data_status[1] & USB3_2_CONNECTION) {
		//usb only
		dp_data.status = 0;
		typec->state.mode = TYPEC_STATE_USB;
		typec->state.alt = NULL;
		typec->state.data = NULL;
	} else {
		//disconnect
		dp_data.status = 0;
		typec->state.mode = TYPEC_STATE_SAFE;
		typec->state.alt = NULL;
		typec->state.data = NULL;
	}

	ret = typec_mux_set(typec->mux, &typec->state);

	if (ret)
		return ret;

	if ((typec->data_status[3]& POWER_MASK) == POWER_SOURCE) {
		typec->caps.type = TYPEC_PORT_SRC;
		typec_set_pwr_role(typec->port, TYPEC_SOURCE);
	} else if ((typec->data_status[3]& POWER_MASK) == POWER_SINK) {
		typec->caps.type = TYPEC_PORT_SNK;
		typec_set_pwr_role(typec->port, TYPEC_SINK);
	} else {
		dev_dbg(typec->dev, "typec port(%d): unkonwn power state %d\n", typec->id, typec->data_status[3]);
	}

	if (typec->role != role) {
		typec->role = role;
		ret = usb_role_switch_set_role(typec->role_sw, role);
		if (role == USB_ROLE_DEVICE) {
			typec_set_data_role(typec->port, TYPEC_DEVICE);
		} else {
			typec_set_data_role(typec->port, TYPEC_HOST);
		}
	}

	if (typec->mode == typec->state.mode && typec->hpd_status == dp_data.status)
		return ret;

	if (typec->state.mode == TYPEC_DP_STATE_D) {
		dev_info(typec->dev, "typec port(%d): alt mode(%s) \n oritation(%s), usb_data_role(%s)\n power role(%s), hpd_state(%d)\n",
			typec->id,
			dp_usb_mode,
			orientation == TYPEC_ORIENTATION_REVERSE ? oritation_reverse : oritation_normal,
			role == USB_ROLE_DEVICE ? data_device : data_host,
			typec->caps.type == TYPEC_PORT_SRC ? power_source:power_sink,
			(int)(dp_data.status & DP_STATUS_HPD_STATE));
	} else if (typec->state.mode == TYPEC_DP_STATE_E) {
		dev_info(typec->dev, "typec port(%d): alt mode(%s) \n oritation(%s)\n power role(%s), hpd_state(%d)\n",
			typec->id,
			dp_only_mode,
			orientation == TYPEC_ORIENTATION_REVERSE ? oritation_reverse : oritation_normal,
			typec->caps.type == TYPEC_PORT_SRC ? power_source:power_sink,
			(int)(dp_data.status & DP_STATUS_HPD_STATE));
	} else if (typec->state.mode == TYPEC_STATE_USB) {
		dev_info(typec->dev, "typec port(%d): alt mode(%s) \n oritation(%s)\n power role(%s), usb_data_role(%s)\n",
			typec->id,
			usb_mode,
			orientation == TYPEC_ORIENTATION_REVERSE ? oritation_reverse : oritation_normal,
			typec->caps.type == TYPEC_PORT_SRC ? power_source:power_sink,
			role == USB_ROLE_DEVICE ? data_device : data_host);
	} else if (typec->state.mode == TYPEC_STATE_SAFE) {
		dev_info(typec->dev, "typec port(%d): disconnect state \n", typec->id);
	}

	typec->mode = typec->state.mode;
	typec->hpd_status = dp_data.status;

	if ((typec->data_status[1] == 0) && (typec->data_status[2] == 0)) {
		dev_info(typec->dev, "typec port(%d): unregister partner\n", typec->id);
		typec_unregister_partner(typec->partner);
		typec->partner = NULL;
	} else {
		if (NULL == typec->partner) {
			dev_info(typec->dev, "typec port(%d): register partner\n", typec->id);
			typec->partner = typec_register_partner(typec->port, &typec->desc);
		}

		if (IS_ERR(typec->partner)) {
			dev_info(typec->dev, "typec port(%d): register partner error \n", typec->id);
		}
	}
	return ret;
}

static void rts5453h_typec_register_port_altmodes(struct rts5453h *typec)
{
	/* All PD capable CrOS devices are assumed to support DP altmode. */
	typec->p_altmode[ALTMODE_DP].svid = USB_TYPEC_DP_SID;
	typec->p_altmode[ALTMODE_DP].mode = USB_TYPEC_DP_MODE;
	typec->state.alt = NULL;
	typec->state.mode = TYPEC_STATE_USB;
	typec->state.data = NULL;

	typec->partner = NULL;
	typec->desc.accessory = TYPEC_ACCESSORY_NONE;
	typec->desc.usb_pd = 0;
}

static int rts5453h_event_handler(struct rts5453h *rts, u8 clear_irq)
{
	int ret = 0;
	u8 data_control[3] = {0};

	mutex_lock(&rts->lock);
	if (clear_irq) {
		data_control[0] = 2;
		data_control[1] = I2C_INT_ACK_BIT;
		data_control[2] = 0;

		ret = rts5453h_block_write(rts, RTS_DATA_CONTROL, data_control, 3);
		if (ret != 0 ) {
				goto i2c_err;
		}
	}

	ret = rts5453h_block_read(rts, RTS_DATA_STATUS, rts->data_status, 6);
	if (ret != 0 ) {
		goto i2c_err;
	} else {
		dev_info(rts->dev, "typec port(%d):data status = 0x%x , 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
			rts->id, rts->data_status[0], rts->data_status[1], rts->data_status[2], rts->data_status[3], rts->data_status[4], rts->data_status[5]);
	}

	rts5453h_typec_port_update(rts);

	if (rts->data_status[2] & HPD_IRQ) {
		data_control[0] = 2;
		data_control[1] = 0;
		data_control[2] = HPD_IRQ_ACK;
		ret = rts5453h_block_write(rts, RTS_DATA_CONTROL, data_control, 3);
		if (ret != 0 ) {
			goto i2c_err;
		}
	}

i2c_err:
	if (ret != 0 ) {
		dev_err(rts->dev, "typec port(%d):i2c transfer error = %d, disable irq\n", rts->id, ret);
		ret = -ESHUTDOWN;
	}

	mutex_unlock(&rts->lock);
	return ret;
}

static int rts5453h_typec_parse_port_props(struct typec_capability *cap,
						struct fwnode_handle *fwnode,
						struct device *dev)
{
	const char *buf;
	int ret;
	memset(cap, 0, sizeof(*cap));
	ret = fwnode_property_read_string(fwnode, "power-role", &buf);
	if (ret) {
		dev_err(dev, "power-role not found: %d\n", ret);
		return ret;
	}
	ret = typec_find_port_power_role(buf);
	if (ret < 0)
		return ret;
	cap->type = ret;
	ret = fwnode_property_read_string(fwnode, "data-role", &buf);
	if (ret) {
		dev_err(dev, "data-role not found: %d\n", ret);
		return ret;
	}
	ret = typec_find_port_data_role(buf);
	if (ret < 0)
		return ret;
	cap->data = ret;
	ret = fwnode_property_read_string(fwnode, "try-power-role", &buf);
	if (ret) {
		dev_err(dev, "try-power-role not found: %d\n", ret);
		return ret;
	}
	ret = typec_find_power_role(buf);
	if (ret < 0)
		return ret;
	cap->prefer_role = ret;
	cap->fwnode = fwnode;
	return 0;
}

static int rts5453h_typec_get_switch_handles(struct rts5453h *typec,
					 struct fwnode_handle *fwnode,
					 struct device *dev)
{
	typec->mux = typec->port->mux;
	typec->ori_sw = typec->port->sw;
	typec->role_sw = fwnode_usb_role_switch_get(fwnode);
	if (IS_ERR(typec->role_sw)) {
		dev_err(dev, "USB role switch handle is error.\n");
		return PTR_ERR(typec->role_sw);
	}
	if (!typec->role_sw || !typec->mux || !typec->ori_sw)
		dev_warn(dev, "mux or switch or role switch has not found\n");

	return 0;
}

static void rts5453h_unregister_ports(struct rts5453h *typec)
{
	if (typec->port) {
		typec->state.alt = NULL;
		typec->state.mode = TYPEC_STATE_USB;
		typec->state.data = NULL;
		usb_role_switch_set_role(typec->role_sw, USB_ROLE_NONE);
		typec_switch_set(typec->ori_sw, TYPEC_ORIENTATION_NONE);
		typec_mux_set(typec->mux, &typec->state);
		usb_role_switch_put(typec->role_sw);
		typec_unregister_port(typec->port);
	}
}

static irqreturn_t rts5453h_irq_handle(int irq, void *data)
{
	struct rts5453h *rts = (struct rts5453h *)data;
	int ret;

	ret = rts5453h_event_handler(rts, 1);

	if (ret == -ESHUTDOWN)
		disable_irq_nosync(irq);

	return IRQ_HANDLED;
}

static int rts5453h_init_ports(struct rts5453h *typec)
{
	struct device *dev = typec->dev;
	struct fwnode_handle *fwnode;

	int ret;
	int nports;

	nports = device_get_child_node_count(dev);
	if (nports == 0) {
		dev_err(dev, "No port entries found.\n");
		return -ENODEV;
	}

	if (nports > 1) {
		dev_err(dev, "More ports listed than can be supported.\n");
		return -EINVAL;
	}

	device_for_each_child_node(dev, fwnode) {
		ret = rts5453h_typec_parse_port_props(&typec->caps, fwnode, dev);
		if (ret < 0)
			goto unregister_ports;

		typec->port = typec_register_port(dev, &typec->caps);
		if (IS_ERR(typec->port)) {
			dev_err(dev, "Failed to register port\n");
			ret = PTR_ERR(typec->port);
			goto unregister_ports;
		}

		ret = rts5453h_typec_get_switch_handles(typec, fwnode, dev);
		if (ret) {
			dev_err(dev, "No switch control for port \n");
			goto unregister_ports;
		}
	}

	return ret;

unregister_ports:
	rts5453h_unregister_ports(typec);
	return ret;
}

static int rts5453h_probe(struct i2c_client *client)
{
	struct rts5453h *rts;
	int ret;

	dev_dbg(&client->dev, "IRQ %d supplied\n", client->irq);
	rts = devm_kzalloc(&client->dev, sizeof(*rts), GFP_KERNEL);
	if (!rts)
		return -ENOMEM;

	ret = device_property_read_u32(&client->dev, "id", &rts->id);
	if (ret < 0)
		return ret;

	rts5453h_typec_register_port_altmodes(rts);

	mutex_init(&rts->lock);

	rts->dev = &client->dev;
	rts->regmap = devm_regmap_init_i2c(client, &rts5453h_regmap_config);

	if (IS_ERR(rts->regmap))
		return PTR_ERR(rts->regmap);
	if (i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		rts->i2c_protocol = true;
	} else {
		dev_err(&client->dev, "i2c check functionality failed.");
		return -ENODEV;
	}

	i2c_set_clientdata(client, rts);

	ret = rts5453h_init_ports(rts);
	if (ret)
		return -EPROBE_DEFER;

	ret = rts5453h_event_handler(rts, 0);
	if (ret) {
		dev_err(&client->dev, "typec event handling error %d", ret);
		return ret;
	}
	/*
	* TBD Check do anything about in init schedule like enable interrupt
	* or check the firmware running status before request irq
	*/
	if (client->irq) {
		irq_set_status_flags(client->irq, IRQ_DISABLE_UNLAZY);
		ret = devm_request_threaded_irq(&client->dev,
						client->irq, NULL,
						rts5453h_irq_handle,
						IRQF_TRIGGER_LOW | IRQF_SHARED | IRQF_ONESHOT,
						dev_name(&client->dev), rts);

		if (ret < 0) {
			dev_err(&client->dev, "request irq %d failed %d", client->irq, ret);
		}
	}

	device_set_wakeup_capable(&client->dev, true);

	return ret;
}

static void rts5453h_remove(struct i2c_client *client)
{
	struct rts5453h *rts = i2c_get_clientdata(client);
	if (rts->partner) {
		typec_unregister_partner(rts->partner);
		rts->partner = NULL;
	}

	if (rts->role_sw) {
		usb_role_switch_put(rts->role_sw);
	}

	if (rts->port) {
		typec_unregister_port(rts->port);
	}
}

static const struct of_device_id rts5453h_of_match[] = {
	{ .compatible = "realtek,rts5453h", },
	{}
};

static const struct acpi_device_id rts5453h_acpi_match[] = {
	{ "CIXH200D" },
	{ },
};
MODULE_DEVICE_TABLE(acpi, rts5453h_acpi_match);

static const struct i2c_device_id rts5453h_id[] = {
	{ "rts5453h" },
	{ }
};

#ifdef CONFIG_PM_SLEEP
static int rts5453h_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);

	disable_irq(client->irq);
	if (device_may_wakeup(dev)) {
		enable_irq_wake(client->irq);
		dev_info(&client->dev, "enable wake");
	}

	return 0;
}

static int rts5453h_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);

	if (device_may_wakeup(dev)) {
		disable_irq_wake(client->irq);
		dev_info(&client->dev, "enable irq");
	}
	enable_irq(client->irq);

	return 0;
}

static int rts5453h_restore(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct rts5453h *rts = i2c_get_clientdata(client);
	int ret = 0;

	if (device_may_wakeup(dev)) {
		disable_irq_wake(client->irq);
		dev_info(&client->dev, "enable irq");
	}
	enable_irq(client->irq);

	/* Re-trigger PD event if PD event occurs during std */
	ret = rts5453h_event_handler(rts, 0);
	if (ret) {
		dev_err(&client->dev, "typec event handling error %d", ret);
		return ret;
	}

	return ret;
}
#endif

static void rts5453h_shutdown(struct i2c_client *client)
{
	if (client->irq) {
		disable_irq(client->irq);
		if (device_may_wakeup(&client->dev)) {
			enable_irq_wake(client->irq);
			dev_info(&client->dev, "enable wake");
		}
	}
}

static const struct dev_pm_ops rts5453h_pm_ops = {
        .suspend = rts5453h_suspend,
        .resume = rts5453h_resume,
        .freeze = rts5453h_suspend,
        .thaw = rts5453h_resume,
        .restore = rts5453h_restore,
};

MODULE_DEVICE_TABLE(i2c, rts5453h_id);
static struct i2c_driver rts5453h_i2c_driver = {
	.driver = {
		.name = "rts5453h",
		.of_match_table = rts5453h_of_match,
		.acpi_match_table = rts5453h_acpi_match,
		.pm = &rts5453h_pm_ops
	},
	.probe_new = rts5453h_probe,
	.remove = rts5453h_remove,
	.id_table = rts5453h_id,
	.shutdown = rts5453h_shutdown
};

module_i2c_driver(rts5453h_i2c_driver);
MODULE_AUTHOR("Chao Zeng <Chao.Zeng@cixtech.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("REALTEK RTS5453H USB PD Controller Driver");
