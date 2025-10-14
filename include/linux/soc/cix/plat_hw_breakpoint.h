#ifndef __PLAT_HW_BREAKPOINT_H
#define __PLAT_HW_BREAKPOINT_H

#include <linux/sched.h>
#include <asm/ptrace.h>

#define HW_BP_TRACE_DEPTH 20

typedef struct hw_bp_value {
	u64 old;
	u64 new;
	long old_flag;
	long new_flag;
} hw_bp_value;

typedef struct hw_trigger_times {
	u64 read;
	u64 write;
	u64 exec;
} hw_trigger_times;

typedef struct hw_bp_callback_data {
	u32 type;
	u64 addr;
	hw_trigger_times times;
	hw_bp_value value; /*bp value*/
	ulong u_stack[HW_BP_TRACE_DEPTH];
	u32 u_stack_size;
	ulong k_stack[HW_BP_TRACE_DEPTH];
	u32 k_stack_size;
	int cpu;
	int pid;
	char comm[TASK_COMM_LEN];
} hw_bp_callback_data;

typedef void (*hw_bp_callback)(const hw_bp_callback_data *attr,
			       const struct pt_regs *regs);

typedef struct hw_bp_attr {
	u32 type; /*bp type*/
	u64 addr; /*The addr of the bp expected to be monitored*/
	u64 start_addr; /*The starting address of the actual monitoring*/
	u64 end_addr; /*The end address of the actual monitoring*/
	u64 len; /*The length of the bp expected to be monitored*/
	u64 real_len; /*LBN len*/
	u32 mask; /*addr mask*/
	hw_trigger_times times; /*trigger times*/
	hw_bp_value value; /*bp value*/
	hw_bp_callback handler; /*user handler*/
	void *rule; /*trigger rule*/
	u64 disabled : 1, //63bit
	    access_type : 8, //55~62
	    reserved : 55; //0~50bit
} hw_bp_attr;

/*struct of get info*/
typedef struct hw_bp_report {
	u32 type; /*bp type*/
	u64 addr; /*The addr of the bp expected to be monitored*/
	u64 len; /*The length of the bp expected to be monitored*/
	u32 mask;
	hw_trigger_times times; /*trigger times*/
} hw_bp_report;

typedef struct hw_bp_info_list {
	struct list_head list; /*list*/
	hw_bp_report *attr; /*bp attr. attr[cpu_id]*/
	int cpu_mask; /*success install of cpu*/
	int cpu_num; /*total cpu num*/
} hw_bp_info_list;

typedef struct hw_bp_ctrl_reg {
	u32 reserved2 : 3, //29~31bit,
		mask : 5, //24~28bit, addr mask，mask=0b11111: (mask2^0b11111 the low bit addr), support 8~2G range
		reserved1 : 3, //21~23bit,
		wt : 1, //20bit, watchpoint type, Unlinked(0)/linked(1) data address match.
		lbn : 4, //16~19bit, WT is only required to be set when setting, which is related to link breakpoints
		ssc : 2, //14,15bit, Security state control, which controls what state will listen for breakpoint events
		hmc : 1, //13bit, Use in conjunction with the above fields
		len : 8, //5~12bit, LBN of len, Each bit represents 1 byte and a maximum of 8 bytes
		type : 2, //3~4bit， bp type wp/bp
		privilege : 2, //1~2bit, The EL level at the time of the last breakpoint setting is used with SSC and HMC
		enabled : 1; //0bit, bp enable
} hw_bp_ctrl_reg;

typedef struct hw_bp_vc {
	u64 address;
	hw_bp_ctrl_reg ctrl;
	u64 trigger;
	u8 access_type;
} hw_bp_vc;

typedef enum hw_bp_condition_type {
	CONDITION_TYPE_NONE = 0,
	CONDITION_TYPE_EQUAL,
	CONDITION_TYPE_NOT_EQUAL,
	CONDITION_TYPE_LESS_THAN,
	CONDITION_TYPE_LESS_THAN_EQUAL,
	CONDITION_TYPE_GREATER_THAN,
	CONDITION_TYPE_GREATER_THAN_EQUAL,
	CONDITION_TYPE_RANGE,
	CONDITION_TYPE_NOT_RANGE,
	CONDITION_TYPE_MAX,
} hw_bp_condition_type;

typedef enum hw_bp_value_type {
	VALUE_TYPE_OLD = 0,
	VALUE_TYPE_NEW,
	VALUE_TYPE_PC,
	VALUE_TYPE_SP,
	VALUE_TYPE_X30,
	VALUE_TYPE_X29,
	VALUE_TYPE_X28,
	VALUE_TYPE_X27,
	VALUE_TYPE_X26,
	VALUE_TYPE_X25,
	VALUE_TYPE_X24,
	VALUE_TYPE_X23,
	VALUE_TYPE_X22,
	VALUE_TYPE_X21,
	VALUE_TYPE_X20,
	VALUE_TYPE_X19,
	VALUE_TYPE_X18,
	VALUE_TYPE_X17,
	VALUE_TYPE_X16,
	VALUE_TYPE_X15,
	VALUE_TYPE_X14,
	VALUE_TYPE_X13,
	VALUE_TYPE_X12,
	VALUE_TYPE_X11,
	VALUE_TYPE_X10,
	VALUE_TYPE_X9,
	VALUE_TYPE_X8,
	VALUE_TYPE_X7,
	VALUE_TYPE_X6,
	VALUE_TYPE_X5,
	VALUE_TYPE_X4,
	VALUE_TYPE_X3,
	VALUE_TYPE_X2,
	VALUE_TYPE_X1,
	VALUE_TYPE_X0,
	VALUE_TYPE_MAX,
} hw_bp_value_type;

typedef struct hw_bp_condition {
	u64 mask;
	u64 condition_value[2];
	hw_bp_condition_type condition;
	hw_bp_value_type value_type;
	u64 bp_type;
} hw_bp_condition;

typedef struct hw_bp_trigger {
	struct list_head list;
	hw_bp_condition rule;
	int magic;
} hw_bp_trigger;

/*install/uninstall*/
int hw_bp_install_from_addr(u64 addr, int len, int type,
			    hw_bp_callback handler);
void hw_bp_uninstall_from_addr(u64 addr);
int hw_bp_install_from_symbol(char *name, int len, int type,
			      hw_bp_callback handler);
void hw_bp_uninstall_from_symbol(char *name);
/*get install bp info*/
hw_bp_info_list *hw_get_bp_infos(void);
void hw_free_bp_infos(hw_bp_info_list *info);
/*add trigger rule*/
int hw_add_contion(u64 addr, hw_bp_condition *cond);
void hw_del_contion(u64 addr, int magic);

#endif
