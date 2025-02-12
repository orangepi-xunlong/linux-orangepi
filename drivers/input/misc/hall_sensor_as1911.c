#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/property.h>
#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/of_gpio.h>

/**
 * struct hall_sensor
 *
 * This struct is used by the hall_sensor driver to setup GPIO lines and to
 * detect state changes using interrupts.
 */
struct hall_sensor {
	int gpio;
	int irq;
	struct input_dev *input;
	int lid_open;  // Indicates the lid state: 1 means the lid is open, 0 means the lid is closed
};

static irqreturn_t hall_sensor_isr(int irq, void *dev_id)
{
	struct hall_sensor *sensor = dev_id;
	int state = gpio_get_value(sensor->gpio);

	if (sensor->lid_open != state) {
		sensor->lid_open = state;

		// Report the lid state
		input_report_switch(sensor->input, SW_LID, state);

		// Send KEY_POWER event
		input_report_key(sensor->input, KEY_POWER, 1);
		input_sync(sensor->input);
		input_report_key(sensor->input, KEY_POWER, 0);
		input_sync(sensor->input);
	}

	return IRQ_HANDLED;
}

static int hall_sensor_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct hall_sensor *sensor;
	int err;
	int detect_pin;

	sensor = devm_kzalloc(dev, sizeof(*sensor), GFP_KERNEL);
	if (!sensor)
		return -ENOMEM;

	detect_pin = of_get_named_gpio(dev->of_node, "detect-gpio", 0);
	if (detect_pin < 0)
		return -EINVAL;

	err = devm_gpio_request_one(dev, detect_pin, GPIOF_IN, "detect-gpio");
	if (err)
		return err;

	sensor->gpio = detect_pin;

	sensor->irq = gpio_to_irq(detect_pin);
	if (sensor->irq < 0)
		return sensor->irq;

	sensor->input = devm_input_allocate_device(dev);
	if (!sensor->input)
		return -ENOMEM;

	sensor->input->name = pdev->name;
	sensor->input->id.bustype = BUS_HOST;

	input_set_drvdata(sensor->input, sensor);

	input_set_capability(sensor->input, EV_SW, SW_LID);
	input_set_capability(sensor->input, EV_KEY, KEY_POWER);

	err = devm_request_threaded_irq(
		dev, sensor->irq, NULL, hall_sensor_isr,
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
		"hall_sensor", sensor);
	if (err)
		return err;

	sensor->lid_open = gpio_get_value(sensor->gpio);

	err = input_register_device(sensor->input);
	if (err)
		return err;

	return 0;
}

static const struct of_device_id hall_sensor_of_match[] = {
	{
		.compatible = "ky,hall-sensor-as1911",
	},
	{},
};
MODULE_DEVICE_TABLE(of, hall_sensor_of_match);

static struct platform_driver hall_sensor_device_driver = {
	.probe = hall_sensor_probe,
	.driver = {
		.name = "hall-sensor-as1911",
		.of_match_table = hall_sensor_of_match,
	}
};
module_platform_driver(hall_sensor_device_driver);

MODULE_AUTHOR("goumin");
MODULE_DESCRIPTION("Hall sensor driver with interrupt support");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:hall-sensor");
