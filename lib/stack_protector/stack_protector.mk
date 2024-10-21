#
# Copyright (c) 2017-2019, ARM Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#

# Boolean macro to be used in C code
STACK_PROTECTOR_ENABLED := 0

ifeq (${ENABLE_STACK_PROTECTOR},0)
  ENABLE_STACK_PROTECTOR := none
endif

ifneq (${ENABLE_STACK_PROTECTOR},none)
  STACK_PROTECTOR_ENABLED := 1
  BL_COMMON_SOURCES	+=	lib/stack_protector/stack_protector.c	\
				lib/stack_protector/${ARCH}/asm_stack_protector.S

  ifeq (${ENABLE_STACK_PROTECTOR},default)
    TF_CFLAGS		+=	-fstack-protector
  else
    TF_CFLAGS		+=	-fstack-protector-${ENABLE_STACK_PROTECTOR}
  endif
endif

STACK_PROTECTOR_LIB_INCLUDES	:=	-Iinclude/plat/qti/qtiseclib/inc/


$(eval $(call add_define,STACK_PROTECTOR_ENABLED))
