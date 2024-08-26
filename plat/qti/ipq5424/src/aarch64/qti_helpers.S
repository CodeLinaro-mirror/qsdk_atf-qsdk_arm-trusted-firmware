/*
 * Copyright (c) 2013-2018, ARM Limited and Contributors. All rights reserved.
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <arch.h>
#include <asm_macros.S>
#include <drivers/arm/gicv2.h>
#include <drivers/arm/gicv3.h>
#include <platform_def.h>
#include <drivers/console.h>
#include <console_macros.S>
#include <qti_uart_console.h>

	.globl	plat_my_core_pos
	.globl	plat_qti_core_pos_by_mpidr
	.globl	plat_reset_handler
	.globl	plat_panic_handler
	.globl	qti_diag_register
	.globl  qti_sysdbg_reset_check

	.extern sysdbg_reset_check
	.extern sysdbg_entry_handler

	/* -----------------------------------------------------
	 *  unsigned int plat_qti_core_pos_by_mpidr(uint64_t mpidr)
	 *  Helper function to calculate the core position.
	 *  With this function:
	 *  CorePos = (ClusterId * 4) + CoreId
	 *  - In ARM v8   (MPIDR_EL1[24]=0)
	 *    ClusterId = MPIDR_EL1[15:8]
	 *    CoreId    = MPIDR_EL1[7:0]
	 *  - In ARM v8.1 (MPIDR_EL1[24]=1)
	 *    ClusterId = MPIDR_EL1[23:15]
	 *    CoreId    = MPIDR_EL1[15:8]
	 *  Clobbers: x0 & x1.
	 * -----------------------------------------------------
	 */
func plat_qti_core_pos_by_mpidr
	mrs	x1, mpidr_el1
	tst	x1, #MPIDR_MT_MASK
	beq	plat_qti_core_pos_by_mpidr_no_mt
	/* Right shift mpidr by one affinity level when MT=1. */
	lsr	x0, x0, #MPIDR_AFFINITY_BITS
plat_qti_core_pos_by_mpidr_no_mt:
	and	x1, x0, #MPIDR_CPU_MASK
	and	x0, x0, #MPIDR_CLUSTER_MASK
	add	x0, x1, x0, LSR #6
	ret
endfunc plat_qti_core_pos_by_mpidr

	/* --------------------------------------------------------------------
	 * void plat_panic_handler(void)
	 * calls SDI and reset system
	 * --------------------------------------------------------------------
	 */
func plat_panic_handler
	msr	spsel, #0
	bl	plat_set_my_stack
	b	qtiseclib_panic
endfunc plat_panic_handler

	/* -----------------------------------------------------
	 *  unsigned int plat_my_core_pos(void)
	 *  This function uses the plat_qti_calc_core_pos()
	 *  definition to get the index of the calling CPU
	 *  Clobbers: x0 & x1.
	 * -----------------------------------------------------
	 */
func plat_my_core_pos
	mrs	x0, mpidr_el1
	b	plat_qti_core_pos_by_mpidr
endfunc plat_my_core_pos

func plat_reset_handler
	mov	x18, x30						/* save the lr */

	/* Serialize CPUSS boot setup. Multi core enter simultaneously. */
	ldr	x0, =g_qti_cpuss_boot_lock
	bl	spin_lock

	/* Execuete CPUSS boot set up on every core. */
	ldr	w0, g_qti_bl31_cold_booted		/* pass cold boot status. */
	bl	qtiseclib_cpuss_reset_asm

	ldr	x0, =g_qti_cpuss_boot_lock
	bl	spin_unlock

	mov	x30, x18
	ret
endfunc plat_reset_handler

	/*----------------------------------------------------
	* Register diag region as console. So that we could
	* use console related apis like printf, putchar etc.
	* with diag region alongside UART
	*-----------------------------------------------------
	*/
func qti_diag_register
	str	x1, [x0, #QTI_CONSOLE_T_UART_BASE_OFF]		/* Save UART base. */
	finish_console_register diag putc=1, getc=0, flush=0
endfunc qti_diag_register

func console_diag_putc
	stp     x29, x30, [sp,#-16]!
	mov     x29, sp
	bl log_put
	mov	w0, #0x0
	ldp     x29, x30, [sp],#16
	ret
endfunc	console_diag_putc

func_in_section qti_sysdbg_reset_check, QTI_SYSDBG_IMEM_CODE
    mov     x25, x30
    bl      sysdbg_reset_check
    cmp     x0, #0
    bne     sysdbg_entry_handler
    mov     x30, x25
    ret
endfunc qti_sysdbg_reset_check

/*--------------------------------------------------------------------
* Configure CLUSTERBUSQOS_EL1 as 0x120 during cold/warm boot of the ARM cores
*--------------------------------------------------------------------*/
    .global mon_configure_clusterbusqos
.type mon_configure_clusterbusqos,%function
mon_configure_clusterbusqos:
    mov     x0, xzr   /* Iniliaize CHI bus QoS field for scheme IDs 0-7 as 0 */
    orr     x0, x0, #0x00000100  /* Set CHI bus QoS field for scheme ID 2 (Bits 4-7) as 1 */
    orr     x0, x0, #0x00000020  /* Set CHI bus QoS field for scheme ID 1 (Bits 0-3) as 2. */
    msr     s3_0_c15_c4_4, x0  /* Write x0 into CLUSTERBUSQOS_EL1. */
//    mrs     x1, s3_0_c15_c4_4  /* Read CLUSTERBUSQOS_EL1 into x1 for testing */
    ret

/*--------------------------------------------------------------------
* When jumping to NS World (NS = 1),
* Configure CLUSTERTHREADSID_EL1 as 0x0 for cores 0,1,2 (USED_CPU_CORES).
* Configure CLUSTERTHREADSID_EL1 as 0x1 for core 3 (RESERVED_CPU_CORE).
* void mon_configure_clusterthreadsid_for_nsworld(is_reserved_cpu_core)
*--------------------------------------------------------------------*/
    .global mon_configure_clusterthreadsid_for_nsworld
.type mon_configure_clusterthreadsid_for_nsworld,%function
mon_configure_clusterthreadsid_for_nsworld:
    cmp     x0, 0x1  /* check if curr cpu is reserved cpu core */
    beq     reserved_core_config_scid
    mov     x0, xzr  /* set scheme ID as 0x0 for this thread. scheme ID bits are 0-2, 3-31 is RAZ */
    msr     s3_0_c15_c4_0, x0  /* Write x0 into CLUSTERTHREADSID_EL1 */
//    mrs     x1, s3_0_c15_c4_0  /* Read CLUSTERTHREADSID_EL1 into x1 for testing */
    ret
reserved_core_config_scid:
    mov     x0, xzr ;
    orr     x0, x0, #0x1  /* set scheme ID as 0x1 for this thread. scheme ID bits are 0-2, 3-31 is RAZ */
    msr     s3_0_c15_c4_0, x0  /* Write x0 into CLUSTERTHREADSID_EL1 */
//    mrs     x1, s3_0_c15_c4_0  /* Read CLUSTERTHREADSID_EL1 into x1 for testing */
    ret

/*--------------------------------------------------------------------
* When entering to Secure World (NS = 0),
* Configure CLUSTERTHREADSID_EL1 as 0x2 for all cores.
* void mon_configure_clusterthreadsid_for_secureworld(void)
*--------------------------------------------------------------------*/
    .global mon_configure_clusterthreadsid_for_secureworld
.type mon_configure_clusterthreadsid_for_secureworld,%function
mon_configure_clusterthreadsid_for_secureworld:
    mov     x6, xzr ;
    orr     x6, x6, #0x2  /* set scheme ID as 0x2 for this thread. scheme ID bits are 0-2, 3-31 is RAZ */
    msr     s3_0_c15_c4_0, x6  /* Write x6 into CLUSTERTHREADSID_EL1 */
//    mrs     x5, s3_0_c15_c4_0  /*Read CLUSTERTHREADSID_EL1 into x5 for testing */
    ret