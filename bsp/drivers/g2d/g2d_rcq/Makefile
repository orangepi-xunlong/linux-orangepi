# SPDX-License-Identifier: GPL-2.0

rcq_dir = g2d_rcq
rcq_obj = ${rcq_dir}/g2d.o ${rcq_dir}/g2d_top.o\
		  ${rcq_dir}/g2d_debug.o


ccflags-y += -I $(srctree)/drivers/hwspinlock
ifeq (${CONFIG_G2D_MIXER},y)
rcq_obj += ${rcq_dir}/g2d_mixer.o ${rcq_dir}/g2d_ovl_v.o\
	   ${rcq_dir}/g2d_ovl_u.o ${rcq_dir}/g2d_rcq.o	\
	   ${rcq_dir}/g2d_scal.o  ${rcq_dir}/g2d_wb.o	\
	   ${rcq_dir}/g2d_bld.o

endif

ifeq (${CONFIG_G2D_ROTATE},y)
rcq_obj += ${rcq_dir}/g2d_rotate.o
endif
