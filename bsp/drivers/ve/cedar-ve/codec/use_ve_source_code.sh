#!/bin/bash

source ../../../../../.buildconfig
CEDAR_VE_DIR=${LICHEE_BSP_DIR}/drivers/ve/cedar-ve
makefile_path=${CEDAR_VE_DIR}/Makefile

sed -i 's/include \$(src)\/codec\/Makefile/include \$(src)\/codecb\/Makefile/g' "$makefile_path"
cp ${CEDAR_VE_DIR}/codecb/libcedarc/script/Makefile_rt-media ${LICHEE_BSP_DIR}/drivers/rt-media/Makefile
cp ${CEDAR_VE_DIR}/codecb/libcedarc/build/build_to_kernel/Makefile ${CEDAR_VE_DIR}/codecb/Makefile
ln -fs ${CEDAR_VE_DIR}/codecb/libcedarc/include/vcodec_base.h ${LICHEE_BSP_DIR}/include/uapi/media/vcodec_base.h
ln -fs ${CEDAR_VE_DIR}/codecb/libcedarc/include/vencoder_base.h ${LICHEE_BSP_DIR}/include/uapi/media/vencoder_base.h
