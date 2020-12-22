#
# Makefile for sensor chip drivers.
#

obj-$(CONFIG_THERMAL)		+= thermal_sys.o

# governors
obj-$(CONFIG_FAIR_SHARE)	+= fair_share.o
obj-$(CONFIG_STEP_WISE)		+= step_wise.o
obj-$(CONFIG_USER_SPACE)	+= user_space.o

# cpufreq cooling
obj-$(CONFIG_CPU_THERMAL)	+= cpu_cooling.o
obj-$(CONFIG_CPU_BUDGET_THERMAL)+= cpu_budget_cooling.o
# platform thermal drivers
obj-$(CONFIG_SPEAR_THERMAL)	+= spear_thermal.o
obj-$(CONFIG_RCAR_THERMAL)	+= rcar_thermal.o
obj-$(CONFIG_SUNXI_THERMAL)	+= sunxi-thermal.o
obj-$(CONFIG_SUNXI_BATTERY)	+= sunxi-battery.o
obj-$(CONFIG_SUNXI_THERMAL_DYNAMIC)	+= sunxi-thermal-bind.o
obj-$(CONFIG_SUNXI_THERMAL)	+= sunxi-temperature.o
obj-$(CONFIG_SUNXI_CPUFREQ_COOLING)	+= sunxi-cpu-cooling.o
obj-$(CONFIG_SUNXI_BUDGET_COOLING)	+= sunxi-cpu-budget-cooling.o
