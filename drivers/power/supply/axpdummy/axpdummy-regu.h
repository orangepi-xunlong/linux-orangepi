#ifndef __AXPDUMMY_REGU_H_
#define __AXPDUMMY_REGU_H_

#define AXPDUMMY_REGU_INFO(_name, _id)     \
{                                          \
	.desc = {                          \
		.name = _name,            \
		.type = REGULATOR_VOLTAGE, \
		.id = _id,                 \
		.owner = THIS_MODULE,      \
	},                                 \
}

#define REGU_INIT_DATA(_name)                             \
{                                                         \
	.constraints = {                                  \
		.name = _name,                           \
		.min_uV = INT_MIN,                        \
		.max_uV = INT_MAX,                        \
		.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE\
				| REGULATOR_CHANGE_STATUS,\
	},                                                \
}

extern int axpdummy_ldo_count;

#endif /* __AXPDUMMY_REGU_H_ */
