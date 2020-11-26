#ifndef __VIRTUALDUMMY_H_
#define __VIRTUALDUMMY_H_

#define VIRTUAL_DUMMY_DRIVER_DATA(_name)            \
{                                                   \
	.probe  = regulator_virtual_consumer_probe, \
	.remove = regulator_virtual_consumer_remove,\
	.driver = {                                 \
		.name = _name,                      \
	},                                          \
}

#define VIRTUAL_DUMMY_DEVICE_DATA(_name, _ldoname)  \
{                                                   \
	.name = _name,                              \
	.id = -1,                                   \
	.dev = {                                    \
		.platform_data = _ldoname,         \
	},                                          \
}

#endif /* __VIRTUALDUMMY_H_ */
