obj-$(CONFIG_TV_DECODER_DISPLAY) += decd.o

decd-objs := drv_dec.o dec.o dec_reg.o dma_buf.o \
	dec_video_info.o frame_manager/dec_frame_manager.o video_buffer.o \
	dec_debug.o afbd_wb.o

decd-$(CONFIG_SYNC_FILE) += dec_fence.o
