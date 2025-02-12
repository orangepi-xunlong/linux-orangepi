// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef _KY_DPU_REG_H_
#define _KY_DPU_REG_H_

enum REG_FILED_TYPE {
	FIELD_DEF,  //reg field is rw
	FIELD_W1C,  //reg field is write 1 cleared
	FIELD_W1S,  //reg field is write 1 set
	FIELD_W0C,  //reg field is write 0 cleared
};

#define dpu_read_reg(hwdev, module_name, module_base, field) \
	({ u32 __v = (((volatile module_name *)(module_base + hwdev->base))->field); \
	__v; }) \

#define dpu_write_reg_common(hwdev, module_name, module_base, field, data, type) \
{ \
	volatile module_name *module = (module_name *)(module_base + hwdev->base); \
	/* Ensure register value is as expected */ \
	module->field = data; \
}

#define dpu_write_reg(hwdev, module_name, module_base, field, data) \
	dpu_write_reg_common(hwdev, module_name, module_base, field, data, FIELD_DEF)

#define dpu_write_reg_w1c(hwdev, module_name, module_base, field, data) \
	dpu_write_reg_common(hwdev, module_name, module_base, field, data, FIELD_W1C)

#define dpu_write_reg_w1s(hwdev, module_name, module_base, field, data) \
	dpu_write_reg_common(hwdev, module_name, module_base, field, data, FIELD_W1S)

#define dpu_write_reg_w0c(hwdev, module_name, module_base, field, data) \
	dpu_write_reg_common(hwdev, module_name, module_base, field, data, FIELD_W0C)

#endif
