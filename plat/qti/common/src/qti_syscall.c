/*
 * Copyright (c) 2018, ARM Limited and Contributors. All rights reserved.
 * Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <common/debug.h>
#include <common/runtime_svc.h>
#include <tools_share/uuid.h>
#include <qti_secure_io_cfg.h>
#include <qtiseclib_interface.h>
#include <qtiseclib_cb_interface.h>
#include <arch_helpers.h>

/*----------------------------------------------------------------------------
 * SIP service - SMC function IDs for SiP Service queries
 * -------------------------------------------------------------------------*/
/*
 * Syscall's to allow Non Secure world accessing peripheral/IO memory
 * those are secure/proteced BUT not required to be secure.
 */
#define	QTI_SIP_SVC_SECURE_IO_READ_ID                   U(0x02000501)
#define	QTI_SIP_SVC_SECURE_IO_WRITE_ID                  U(0x02000502)
#define	QTI_SIP_SVC_SECURE_IO_READ_PARAM_ID             U(0x1)
#define	QTI_SIP_SVC_SECURE_IO_WRITE_PARAM_ID            U(0x2)

#define QTI_SIP_IS_SMC_AVAILABLE_ID                     U(0x02000601)
#define QTI_DUMP_SET_CPU_CTX_BUF_ID                     U(0x02000302)
#define QTI_SIP_TMEL_FUSE_READ_MULTIPLE_ROWS_ID         U(0x02000822)
#define QTI_SIP_TMEL_FUSE_READ_MULTIPLE_ROWS_PARAM_ID   U(0x22)

/* Syscall for Secure Auth */
#define QTI_SIP_SECURE_AUTH_ID                          U(0x0200011F)
#define QTI_SIP_SECURE_AUTH_V2_ID                       U(0x02000124)
#define QTI_SIP_SECURE_AUTH_PARAM_ID                    U(0x00000405)
#define QTI_SIP_SECURE_AUTH_V2_PARAM_ID                 U(0x00004407)
/* Syscall for Verify rootFS Hash */
#define QTI_SIP_VERIFY_FS_HASH_ID                       U(0x02000123)
#define QTI_SIP_VERIFY_FS_HASH_PARAM_ID                 U(0x00000885)

#define	FUNCID_OEN_NUM_MASK  ((FUNCID_OEN_MASK << FUNCID_OEN_SHIFT)\
                                |(FUNCID_NUM_MASK << FUNCID_NUM_SHIFT) )

#define QTI_MASK_BITS(h,l)     ((0xffffffff >> (32 - ((h - l) + 1))) << l)
#define QTI_SYSCALL_NUM_ARGS(r1)   (r1 & QTI_MASK_BITS(3,0))

/* QTI SiP Service UUID */
DEFINE_SVC_UUID2(qti_sip_svc_uid,
                0x43864748, 0x217f, 0x41ad, 0xaa, 0x5a,
                0xba, 0xe7, 0x0f, 0xa5, 0x52, 0xaf);

/*
 * This function is used to check if the SMC is supported
*/
static bool qti_is_smc_available(uint32_t smc_id)
{
    /**
    SMC Masks :
         Bit 31      - IRQ / Atomic / Fast SMC
         Bit 30      - ARCH
         Bit (29-24) - Owner ID
         Bit (23-16) - Reserved
         Bit (15-0)  - Func ID
    **/
    /*Skipping IRQ and ARCH*/
    smc_id &= 0x3FFFFFFF;
    
    switch(smc_id)
    {
        case QTI_SIP_SVC_SECURE_IO_READ_ID:
        case QTI_SIP_SVC_SECURE_IO_WRITE_ID:
        case QTI_SIP_IS_SMC_AVAILABLE_ID:
        case QTI_DUMP_SET_CPU_CTX_BUF_ID:
        case QTI_SIP_TMEL_FUSE_READ_MULTIPLE_ROWS_ID:
        case QTI_SIP_SECURE_AUTH_ID:
        case QTI_SIP_SECURE_AUTH_V2_ID:
        case QTI_SIP_VERIFY_FS_HASH_ID:
                return true;
        default:
                return false;
    }
}

static bool qti_is_secure_io_access_allowed(u_register_t addr)
{
        int i = 0;

        for (i = 0; i < ARRAY_SIZE(qti_secure_io_allowed_regs); i++)
        {
                if ((uintptr_t)addr == qti_secure_io_allowed_regs[i])
                {
                        return true;
                }
        }

  return false;
}

/*
 * This function handles QTI specific syscalls. Currently only SiP calls are present.
 * Both FAST & YIELD type call land here.
 */
static uintptr_t qti_sip_handler(uint32_t smc_fid,
                                 u_register_t x1,
                                 u_register_t x2,
                                 u_register_t x3,
                                 u_register_t x4,
                                 void *cookie, void *handle, u_register_t flags)
{
    uint32_t l_smc_fid = smc_fid & FUNCID_OEN_NUM_MASK;
    int ret;

    if (SMC_32 == GET_SMC_CC(smc_fid)) {
        x1 = (uint32_t)x1;
        x2 = (uint32_t)x2;
        x3 = (uint32_t)x3;
        x4 = (uint32_t)x4;
    }

    uint32_t nargs=0;
    /*
     * Extract the lower 4 bits from the parameter ID,
     * as the total number of arguments is represented in these last 4 bits.
     */
    nargs = QTI_SYSCALL_NUM_ARGS(x1) ;
    u_register_t *regs = (u_register_t *)read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X5);
    if(nargs > 4)
    {
        uint32_t indirect_params_num  = nargs - 3;
        if(SMC_32 == GET_SMC_CC(smc_fid)){
                inv_dcache_range((uintptr_t)regs, sizeof(uint32_t)*indirect_params_num);
        }
        else{
                inv_dcache_range((uintptr_t)regs, sizeof(uint64_t)*indirect_params_num);
        }
    }

    switch (l_smc_fid) {
        case QTI_SIP_TMEL_FUSE_READ_MULTIPLE_ROWS_ID:
        {
            if (QTI_SIP_TMEL_FUSE_READ_MULTIPLE_ROWS_PARAM_ID == x1) {
                    SMC_RET2(handle, SMC_OK, qtiseclib_read_tmel_fuse_multiple_rows((uint32_t *)x2, (uint32_t) x3));
            }
            else
                    SMC_RET1(handle, SMC_UNK);
        }
        case QTI_SIP_SVC_SECURE_IO_READ_ID:
        {
            if ((QTI_SIP_SVC_SECURE_IO_READ_PARAM_ID == x1) &&
                qti_is_secure_io_access_allowed(x2)) {
                    SMC_RET2(handle, SMC_OK, *((volatile uint32_t*)x2));
            }
            SMC_RET1(handle, SMC_UNK);
        }
        case QTI_SIP_SVC_SECURE_IO_WRITE_ID:
        {
            if ((QTI_SIP_SVC_SECURE_IO_WRITE_PARAM_ID == x1) &&
                qti_is_secure_io_access_allowed(x2)) {
                    *((volatile uint32_t *)x2) = x3;
                    SMC_RET2(handle, SMC_OK, SMC_OK);
            }
            SMC_RET1(handle, SMC_UNK);
        }
        case QTI_SIP_IS_SMC_AVAILABLE_ID:
        {
                SMC_RET2(handle, SMC_OK, qti_is_smc_available(x2));
        }
        case QTI_DUMP_SET_CPU_CTX_BUF_ID:
        {
                ret = qtiseclib_set_cpu_ctx_buf(x2, x3);
                SMC_RET2(handle, SMC_OK, ret);
        }
        case QTI_SIP_SECURE_AUTH_ID:
        {
            uint32_t x5, x6;
            x5 = (uint32_t) regs[0];
            if (SMC_32 == GET_SMC_CC(smc_fid)) {
                x6 = (uint32_t) (regs[0] >> 32);
            } else {
                x6 = (uint32_t) regs[1];
            }
            if (QTI_SIP_SECURE_AUTH_PARAM_ID == x1) {
                SMC_RET1(handle, sec_img_auth_using_tme_l((uint32_t)x2,
                         (uint32_t)x3, (uint32_t)x4, (uint32_t *)(uintptr_t)x5, x6, handle));
            }
            SMC_RET1(handle, SMC_UNK);
        }
        case QTI_SIP_SECURE_AUTH_V2_ID:
        {
            u_register_t x5, x6, x7, x8;
            x5 = (uint32_t) regs[0];
            /* Retrieve indirect args. */
            if (SMC_32 == GET_SMC_CC(smc_fid)) {
                    x6 = *((uint32_t *)regs + 1);
                    x7 = *((uint32_t *)regs + 2);
                    x8 = *((uint32_t *)regs + 3);
            } else {
                    x6 = *((uint64_t *)regs + 1);
                    x7 = *((uint64_t *)regs + 2);
                    x8 = *((uint64_t *)regs + 3);
            }
            if (QTI_SIP_SECURE_AUTH_V2_PARAM_ID == x1) {
                SMC_RET1(handle, sec_img_auth_v2_using_tme_l((uint32_t)x2,
                         (uint32_t)x3, (uint32_t)x4, (uint32_t *)(uintptr_t)x5, x6, x7, x8, handle));
            }
            SMC_RET1(handle, SMC_UNK);
        }
        case QTI_SIP_VERIFY_FS_HASH_ID:
        {
            uint32_t x5, x6;
            x5 = (uint32_t) regs[0];
            if (SMC_32 == GET_SMC_CC(smc_fid)) {
                x6 = (uint32_t) (regs[0] >> 32);
            } else {
                x6 = (uint32_t) regs[1];
            }
            if (QTI_SIP_VERIFY_FS_HASH_PARAM_ID == x1) {
                SMC_RET1(handle, sec_img_auth_fs_hash((uint32_t)x2,
                         (uint32_t)x3, (uint32_t)x4, x5, x6, handle));
            }
            SMC_RET1(handle, SMC_UNK);
        }
        default:
        {
            QTISECLIB_CB_ERROR("x0 = 0%x, x1 = 0x%p, x2 = 0x%p, x3 = 0x%p\n",
                               l_smc_fid, (void *) x1, (void *) x2, (void *) x3);
            SMC_RET1(handle, SMC_UNK);
        }
    }
        return (uintptr_t) handle;
}

/* Define a runtime service descriptor for both fast & yield SiP calls */
DECLARE_RT_SVC(qti_sip_fast_svc, OEN_SIP_START,
               OEN_SIP_END, SMC_TYPE_FAST, NULL, qti_sip_handler);

DECLARE_RT_SVC(qti_sip_yield_svc, OEN_SIP_START,
               OEN_SIP_END, SMC_TYPE_YIELD, NULL, qti_sip_handler);
