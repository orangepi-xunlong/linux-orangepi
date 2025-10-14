.. SPDX-License-Identifier: GPL-2.0

Introduction
============

This document is an extension of the pin control subsystem in Linux [1] and describes
the pin control related ACPI properties supported in Linux kernel.

On some platform, certains peripheral buses or devices could be connected to same
external pin header with a different pin configuration requirement. A simple example of
this would be a board where pins for I2C data and clock is multiplexed and shared with
GPIO pins for a display controller. Pins would require different
configuration in these two modes. For example, I2C functionality would require pin
bias to be set to pull up with pull strength of 10K Ohms and for GPIO functionality
pin bias needs to be set to pull down with pull strength of 20K Ohms,
input Schmitt-trigger enabled and a slew rate of 3.

ACPI 6.2 version [2] introduced following resources to be able to describe different
pin functions and configurations required for devices.

- PinFunction
- PinConfig
- PinGroup
- PinGroupFunction
- PinGroupConfig

OSPM will have to handle the above resources and select the pin function and configuration
through vendor specific interfaces (e.g: memory mapped registers) for the devices to be
fully functional.

Example 1 : I2C controller SDA/SCL muxed with display controller GPIO pin
=========================================================================

.. code-block:: text

	//
	// Description: GPIO
	//
	Device (GPI0)
	{
		Name (_HID, "PNPFFFE")
		Name (_UID, 0x0)
		Method (_STA)
		{
			Return(0xf)
		}
		Method (_CRS, 0x0, NotSerialized)
		{
			Name (RBUF, ResourceTemplate()
			{
				Memory32Fixed(ReadWrite, 0x4FE00000, 0x20)
				Interrupt(ResourceConsumer, Level, ActiveHigh, Shared) {0x54}
			})
			Return(RBUF)
		}
	}

	//
	// Description: I2C controller 1
	//
	Device (I2C1)
	{
		Name (_HID, "PNPFFFF")
		Name (_UID, 0x0)
		Method (_STA)
		{
			Return(0xf)
		}
		Method (_CRS, 0x0, NotSerialized)
		{
			Name (RBUF, ResourceTemplate()
			{
				Memory32Fixed(ReadWrite, 0x4F800000, 0x20)
				Interrupt(ResourceConsumer, Level, ActiveHigh, Shared) {0x55}
				PinFunction(Exclusive, PullDefault, 0x5, "\\_SB.GPI0", 0, ResourceConsumer, ) {2, 3}
				// Configure 10k Pull up for I2C SDA/SCL pins
				PinConfig(Exclusive, 0x01, 10000, "\\_SB.GPI0", 0, ResourceConsumer, ) {2, 3}
			})
			Return(RBUF)
		}
	}

	//
	// Description: Physical display panel
	//
	Device (SDIO)
	{
		Name (_HID, "PNPFFFD")
		Name (_UID, 0x0)
		Method (_STA)
		{
			Return(0xf)
		}
		Method (_CRS, 0x0, NotSerialized)
		{
			Name (RBUF, ResourceTemplate()
			{
				Memory32Fixed(ReadWrite, 0x4F900000, 0x20)
				Interrupt(ResourceConsumer, Level, ActiveHigh, Shared) {0x57}
				GpioIo(Shared, PullDefault, 0, 0, IoRestrictionNone, "\\_SB.GPI0",) {2, 3}
				// Configure 20k Pull down
				PinConfig(Exclusive, 0x02, 20000, "\\_SB.GPI0", 0, ResourceConsumer, ) {2, 3}
				// Enable Schmitt-trigger
				PinConfig(Exclusive, 0x0D, 1, "\\_SB.GPI0", 0, ResourceConsumer, ) {2, 3}
				// Set slew rate to custom value 3
				PinConfig(Exclusive, 0x0B, 3, "\\_SB.GPI0", 0, ResourceConsumer, ) {2, 3}
			})
			Return(RBUF)
		}
	}


Example 2 : Pin muxing and configuration described with pin groups
==================================================================

The configuration is similar to example 1 but described using pin group resources

.. code-block:: text

	//
	// Description: GPIO
	//
	Device (GPI0)
	{
		Name (_HID, "PNPFFFE")
		Name (_UID, 0x0)
		Method (_STA)
		{
			Return(0xf)
		}
		Method (_CRS, 0x0, NotSerialized)
		{
			Name (RBUF, ResourceTemplate()
			{
				Memory32Fixed(ReadWrite, 0x4FE00000, 0x20)
				Interrupt(ResourceConsumer, Level, ActiveHigh, Shared) {0x54}
				PinGroup("group1", ResourceProducer) {2, 3}

			})
			Return(RBUF)
		}
	}

	//
	// Description: I2C controller 1
	//
	Device (I2C1)
	{
		Name (_HID, "PNPFFFF")
		Name (_UID, 0x0)
		Method (_STA)
		{
			Return(0xf)
		}
		Method (_CRS, 0x0, NotSerialized)
		{
			Name (RBUF, ResourceTemplate()
			{
				Memory32Fixed(ReadWrite, 0x4F800000, 0x20)
				Interrupt(ResourceConsumer, Level, ActiveHigh, Shared) {0x55}
				// Set function I2C1 for SDA/SCL pins
				PinGroupFunction(Exclusive, 0x5, "\\_SB.GPI0, 0, "group1", ResourceConsumer, )
				// Configure 10k Pull up for SDA/SCL pins
				PinGroupConfig(Exclusive, 0x01, 10000, "\\_SB.GPI0 ", 0, "group1", ResourceConsumer, )
			})
			Return(RBUF)
		}
	}

	//
	// Description: Physical display panel
	//
	Device (DISP)
	{
		Name (_HID, "PNPFFFD")
		Name (_UID, 0x0)
		Method (_STA)
		{
			Return(0xf)
		}
		Method (_CRS, 0x0, NotSerialized)
		{
			Name (RBUF, ResourceTemplate()
			{
				Memory32Fixed(ReadWrite, 0x4F900000, 0x20)
				Interrupt(ResourceConsumer, Level, ActiveHigh, Shared) {0x57}
				// Set function GPIO for pin group group1
				PinGroupFunction(Exclusive, 0x1, "\\_SB.GPI0 ", 0, "group1",
				˓ResourceConsumer, )
				// Configure 20k Pull down
				PinGroupConfig (Exclusive, 0x02, 20000, "\\_SB.GPI0 ", 0, "group1",
				˓ResourceConsumer, )
				//Enable Schmitt-trigger
				PinGroupConfig (Exclusive, 0x0D, 1, "\\_SB.GPI0 ", 0, "group1",
				˓ResourceConsumer, )
				//Set slew rate to custom value 3
				PinGroupConfig (Exclusive, 0x0B, 3, "\\_SB.GPI0 ", 0, "group1",
				˓ResourceConsumer, )
			})
			Return(RBUF)
			}
		}
	}

Notes for pin controller device driver developers
=================================================

This section contains few examples and guidelines for device driver developers to
add bindings to handle ACPI pin resources.

Pin control devices can add callbacks for following pinctrl_ops to handle ACPI
pin resources.

.. code-block:: c

	struct pinctrl_ops {
		...
		int (*acpi_node_to_map)(struct pinctrl_dev *pctldev,
					struct pinctrl_acpi_resource *resource,
					struct pinctrl_map **map, unsigned *num_maps);
		void (*acpi_free_map)(struct pinctrl_dev *pctldev,
					struct pinctrl_map *map, unsigned num_maps);
		...
	}

Following example demonstrate how the pinctrl_acpi_resource struct can be mapped
to generic pinctrl_map.

.. code-block:: c

	int example_acpi_node_to_map(struct pinctrl_dev *pctldev,
					struct pinctrl_acpi_resource *resource,
					struct pinctrl_map **map,
					unsigned *num_maps_out)
	{

		...
		new_map = devm_kzalloc(pctldev->dev, sizeof(struct pinctrl_map),
				GFP_KERNEL);

		switch (info->type) {
		case PINCTRL_ACPI_PIN_FUNCTION:
			new_map->type = PIN_MAP_TYPE_MUX_GROUP;
			new_map->data.mux.group = example_pinctrl_find_pin_group(
							info->function.function_number,
							info->function.pins, info->function.npins);
			new_map->data.mux.function = pinctrl_find_function(info->function.function_number);
			break;
		case PINCTRL_ACPI_PIN_CONFIG:
			new_map->type = PIN_MAP_TYPE_CONFIGS_PIN;
			new_map->data.configs.group_or_pin = pin_get_name(pctldev, info->config.pin);
			new_map->data.configs.configs = devm_kcalloc(
				pctldev->dev, info->config.nconfigs,
				sizeof(unsigned long), GFP_KERNEL);
			new_map->data.configs.num_configs = 0;
			list_for_each_entry(config_node, info->config.configs, node)
				new_map->data.configs.configs[new_map->data.configs.num_configs++] =
					config_node->config;
			break;
		}

		...
	}

Pin controller will have to map function numbers from ACPI to internal function numbers
and select appropriate group for pin muxing. Multiple pinctrl_map might need to generated
if more than one group needs to be activated. Above example just assumes all of the pins
belongs to a single group.

Multiple configurations might need to be applied for a pin and ACPI could have multiple
resources to define them. E.g:

.. code-block:: text

	// Configure 20k Pull down
	PinConfig(Exclusive, 0x02, 20000, "\\_SB.GPI0", 0, ResourceConsumer, ) {2, 3}
	// Enable Schmitt-trigger
	PinConfig(Exclusive, 0x0D, 1, "\\_SB.GPI0", 0, ResourceConsumer, ) {2, 3}
	// Set slew rate to custom value 3
	PinConfig(Exclusive, 0x0B, 3, "\\_SB.GPI0", 0, ResourceConsumer, ) {2, 3}

ACPI pin controller will combine the configurations at the pin level and will invoke
acpi_node_to_map to map them to struct pinctrl_map. The above ACPI resources would
generate two struct pinctrl_acpi_resource descriptors, one for each pin, with list
of configs to apply for each pin.

ACPI pin resources can be described at group level as described in example 2 above.
There is no change to the internal pinctrl ACPI interface due to this. ACPI pinctrl
subsystem will resolve all of the groups defined in AML to pins using PinGroup resources.

References
==========

[1] Documentation/driver-api/pin-control.rst

[2] ACPI Specifications, Version 6.2 - Section 19.6.102 to 19.6.106
	https://uefi.org/sites/default/files/resources/ACPI_6_2.pdf
