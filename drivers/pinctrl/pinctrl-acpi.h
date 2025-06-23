/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ACPI helpers for PinCtrl API
 *
 * Copyright (C) 2022 Linaro Ltd.
 */

 struct pinctrl_acpi_group_desc {
	const char *name;
	unsigned short *pins;
	unsigned int num_pins;
	unsigned char *vendor_data;
	unsigned int vendor_length;
	struct list_head list;
 };

/**
 * struct pinctrl_acpi_config_node - config node descriptor
 * @config: generic pin config value
 * @node: list node
 */
struct pinctrl_acpi_config_node {
	unsigned long config;
	struct list_head node;
};

/**
 * struct pinctrl_acpi_pin_function - pin function descriptor
 * @pins: pin array from ACPI resources
 * @npins: number of entries in @pins
 * @function_number: function number to apply for the pin
 */
struct pinctrl_acpi_pin_function {
	unsigned int *pins;
	size_t npins;
	unsigned int function_number;
};

/**
 * struct pinctrl_acpi_pin_group_config - pin config descriptor
 * @pin: pin number
 * @nconfigs: number of configs in @configs
 * @configs: config list
 */
struct pinctrl_acpi_pin_config {
	unsigned int pin;
	size_t nconfigs;
	struct list_head *configs;
};

/**
 * enum pinctrl_acpi_resource_type - pinctrl acpi resource type
 * @PINCTRL_ACPI_PIN_FUNCTION: pin function type
 * @PINCTRL_ACPI_PIN_CONFIG: pin config type
 */
enum pinctrl_acpi_resource_type {
	PINCTRL_ACPI_PIN_FUNCTION,
	PINCTRL_ACPI_PIN_CONFIG,
};

/**
 * struct pinctrl_acpi_resource - pinctrl acpi resource
 * @type: resource type
 * @function: pin function resource descriptor
 * @config: pin config resource descriptor
 */
struct pinctrl_acpi_resource {
	enum pinctrl_acpi_resource_type type;
	union {
		struct pinctrl_acpi_pin_function function;
		struct pinctrl_acpi_pin_config config;
	};
};

#ifdef CONFIG_ACPI
int pinctrl_acpi_to_map(struct pinctrl *p);
void pinctrl_acpi_free_maps(struct pinctrl *p);
int pinctrl_acpi_get_pin_groups(struct acpi_device *adev,
				struct list_head *group_desc_list);
#else
static inline int pinctrl_acpi_to_map(struct pinctrl *p)
{
	return -ENXIO;
}
static inline void pinctrl_acpi_free_maps(struct pinctrl *p)
{
}

static inline
int pinctrl_acpi_get_pin_groups(struct acpi_device *adev,
				struct list_head *group_desc_list)
{
	return 0;
}

#endif
