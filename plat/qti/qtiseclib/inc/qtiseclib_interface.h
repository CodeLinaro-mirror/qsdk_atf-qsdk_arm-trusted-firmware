/*
 * Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*
 * Changes from Qualcomm Innovation Center are provided under the following license:
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __QTISECLIB_INTERFACE_H__
#define __QTISECLIB_INTERFACE_H__

#include <stdint.h>
#include <stdbool.h>
#include <qtiseclib_defs.h>

/*----------------------------------------------------------------------------
 * QTISECLIB Published API's.
 * -------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Assembly API's
 * -------------------------------------------------------------------------*/

/*
 * CPUSS common reset handler for all CPU wake up (both cold & warm boot).
 * Executes on all core. This API assume serialization across CPU
 * already taken care before invoking.
 *
 * Clobbers: x0 - x17, x30
 */
void qtiseclib_cpuss_reset_asm(uint32_t bl31_cold_boot_state);

/*
 * Execute CPU (Kryo3 gold) specific reset handler / system initialization.
 * This takes care of executing required CPU errata's.
 *
 * Clobbers: x0 - x16
 */
void qtiseclib_kryo3_gold_reset_asm(void);

/*
 * Execute CPU (Kryo3 silver) specific reset handler / system initialization.
 * This takes care of executing required CPU errata's.
 *
 * Clobbers: x0 - x16
 */
void qtiseclib_kryo3_silver_reset_asm(void);

/*----------------------------------------------------------------------------
 * C Api's
 * -------------------------------------------------------------------------*/
void qtiseclib_bl31_platform_setup(void);
void qtiseclib_get_entrypoint_param(uint64_t, entry_point_info_t *);
void qtiseclib_invoke_isr(uint32_t irq, void *handle);
void qtiseclib_panic(void);
int qtiseclib_prng_get_data(uint8_t *out, uint32_t out_len);

int qtiseclib_mem_assign( u_register_t	IPAinfo_hyp,
						  u_register_t	IPAinfolistsize,
						  u_register_t	sourceVMlist_hyp,
						  u_register_t	srcVMlistsize,
						  u_register_t	destVMlist_hyp,
						  u_register_t	dstVMlistsize,
						  u_register_t	spare);

void qtiseclib_oem_register_wifi_interrupt(int irq);
void qtiseclib_Clock_Init(void);
int qtiseclib_psci_init(void);
int qtiseclib_psci_node_power_on(u_register_t mpidr);
void qtiseclib_psci_node_on_finish(const uint8_t * states);
void qtiseclib_psci_cpu_standby(uint8_t pwr_state);
void qtiseclib_psci_node_power_off(const uint8_t * states);
void qtiseclib_psci_node_suspend(const uint8_t * states);
void qtiseclib_psci_node_suspend_finish(const uint8_t * states);
void qtiseclib_save_dev_data_addr( void* data);
__attribute__((noreturn)) void qtiseclib_psci_system_off(void);
__attribute__((noreturn)) void qtiseclib_psci_system_reset(void);
void qtiseclib_disable_cluster_coherency(uint8_t state);
int qtiseclib_psci_validate_power_state(unsigned int pwr_state, uint8_t * req_state);
int qtiseclib_config_reset_debug(uint32_t, uint32_t);
int qtiseclib_set_cpu_ctx_buf(uintptr_t addr, uint32_t size);
int qtiseclib_protect_mem_subsystem(uint32_t subsystem_id, uintptr_t phy_base, uint32_t size);
int qtiseclib_clear_protect_mem_subsystem(uint32_t subsystem_id, uintptr_t phy_base, uint32_t size, uint32_t auth_key);
int qtiseclib_get_diag(char* buf, size_t buf_size);
uint64_t qtiseclib_get_ddr_size();
int qtiseclib_secure_boot_check(char * secure_boot_val, uint32_t input_len);

bool TmeFuseApiRunTests(void);
bool is_dload_magic_set(void);
int qtiseclib_dpr_addr_send_tmel(uint32_t addr, uint32_t size);
void qtiseclib_get_smem_targ_info(uint64_t *addr, uint64_t *size);
void qtiseclib_get_smem_base_addr(uint64_t *addr, uint64_t *size);
void qtiseclib_set_image_version(char *major, char *minor, char *atf_commit);


int qtiseclib_qfprom_fuse_secdat(uint32_t *);
int qtiseclib_get_feature_version(uint32_t feature_id);


uint32_t qtiseclib_test_get_xpu_err_count(void);
void qtiseclib_test_clear_xpu_err_count(void);
int qti_test_stack_protection(void);
uint32_t qtiseclib_get_secure_state(void);

void mon_configure_clusterthreadsid_for_nsworld(bool is_reserved_cpu_core);
void mon_configure_clusterthreadsid_for_secureworld(void);
void mon_configure_clusterbusqos(void);

int qtiseclib_read_tmel_fuse_multiple_rows(uint32_t *addr, uint32_t size);

#ifdef UNIT_TEST_DEVICE_ATTESTATION_AND_PROVISIONING
int32_t tmel_qwes_init_attestation(uint32_t m3EphPubKeyBuff,
								   uint32_t m3EphPubKeyBuffsize,
								   uint32_t *m3EphPubKeySize,
								   void *rsp);
int32_t tmel_qwes_get_attestation_report(uint32_t attestReqBuff,
										 uint32_t attestReqBuffSize,
										 uint32_t extClaimBuff,
										 uint32_t extClaimBuffSize,
										 uint32_t attestRespBuff,
										 uint32_t attestRespBuffsize,
										 uint32_t *attestRespSize,
										 void *rsp);
int32_t tmel_qwes_provision_device(uint32_t provReqBuff,
								   uint32_t provReqBuffSize,
								   uint32_t provRespBuff,
								   uint32_t provRespBuffsize,
								   uint32_t *provRespSize,
								   void *rsp);
int32_t tmel_qwes_bindings_check(uint32_t bindCheckReqBuff,
								 uint32_t bindCheckReqBuffSize,
								 uint32_t bindCheckRespBuff,
								 uint32_t bindCheckRespBuffsize,
								 uint32_t *bindCheckRespSize,
								 void *rsp);
#endif /* UNIT_TEST_DEVICE_ATTESTATION_AND_PROVISIONING */

int32_t sec_img_auth_using_tme_l(uint32_t  elfAddr,
				uint32_t  buffSize,
				uint32_t  sec_img_id,
				void *regionbuf,
				uint32_t  regionbufsize,
				void *rsp);
int32_t sec_img_auth_fs_hash(uint32_t sw_type, uintptr_t img_addr, uint32_t img_size,
				uintptr_t hash_addr, uint32_t hash_size);

#endif /* __QTISECLIB_INTERFACE_H__ */
