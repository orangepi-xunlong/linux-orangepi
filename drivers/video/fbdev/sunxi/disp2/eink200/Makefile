obj-$(CONFIG_EINK200_SUNXI) += sunxi_eink.o

sunxi_eink-objs := eink_driver.o
sunxi_eink-objs += eink_sys_source.o
sunxi_eink-objs += eink_manager.o buffer_mgr.o pipeline_mgr.o index_mgr.o \
	timing_ctrl_mgr.o fmt_convert_mgr.o waveform_mgr.o

sunxi_eink-objs += panels/panel_ctrl.o panels/default_eink.o

sunxi_eink-objs += lowlevel/de_wb.o lowlevel/eink200.o
