/*
 * Copyright (c) 2019, The Linux Foundation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*
 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __QTI_SECURE_IO_CFG_H__
#define __QTI_SECURE_IO_CFG_H__

#include <stdint.h>

/*
 * List of peripheral/IO memory areas that are protected from
 * non-secure world but not required to be secure.
 */

static const uintptr_t qti_secure_io_allowed_regs[] = {
	0x195C100, /* TCSR_BOOT_MISC_DETECT */
	0x195C158, /* TCSR_BOOT_INFO */
};

#endif /* __QTI_SECURE_IO_CFG_H__ */

