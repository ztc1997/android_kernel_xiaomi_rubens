/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __SCP_EXCEP_H__
#define __SCP_EXCEP_H__

#include <linux/sizes.h>
#include <linux/arm-smccc.h>
#include <linux/soc/mediatek/mtk_sip_svc.h>
#include "scp_helper.h"
#include "scp_ipi_pin.h"

#define AED_LOG_PRINT_SIZE	SZ_16K
#define SCP_LOCK_OFS	0xE0
#define SCP_TCM_LOCK_BIT	(1 << 20)

enum scp_excep_id {
	EXCEP_RESET,
	EXCEP_BOOTUP,
	EXCEP_RUNTIME,
	SCP_NR_EXCEP,
};

struct scp_status_reg {
	uint32_t status;
	uint32_t pc;
	uint32_t lr;
	uint32_t sp;
	uint32_t pc_latch;
	uint32_t lr_latch;
	uint32_t sp_latch;
};

enum MTK_TINYSYS_SCP_KERNEL_OP {
	MTK_TINYSYS_SCP_KERNEL_OP_DUMP_START = 0,
	MTK_TINYSYS_SCP_KERNEL_OP_DUMP_POLLING,
	MTK_TINYSYS_SCP_KERNEL_OP_RESET_SET,
	MTK_TINYSYS_SCP_KERNEL_OP_RESET_RELEASE,
	MTK_TINYSYS_SCP_KERNEL_OP_NUM,
};

extern void scp_dump_last_regs(void);
extern void scp_aed(enum SCP_RESET_TYPE type, enum scp_core_id id);
extern void scp_aed_reset(enum scp_excep_id type, enum scp_core_id id);
extern void scp_aed_reset_inplace(enum scp_excep_id type,
		enum scp_core_id id);
extern void scp_get_log(enum scp_core_id id);
extern char *scp_pickup_log_for_aee(void);
extern void aed_scp_exception_api(const int *log, int log_size,
		const int *phy, int phy_size, const char *detail,
		const int db_opt);
extern void scp_excep_cleanup(void);
extern uint32_t memorydump_size_probe(struct platform_device *pdev);
enum { r0, r1, r2, r3, r12, lr, pc, psr};
extern int scp_ee_enable;
extern int scp_reset_counts;

extern struct scp_status_reg *c0_m;
extern struct scp_status_reg *c0_t1_m;
extern struct scp_status_reg *c1_m;
extern struct scp_status_reg *c1_t1_m;

typedef enum MDUMP {
	MDUMP_DUMMY,
	MDUMP_L2TCM,
	MDUMP_L1C,
	MDUMP_REGDUMP,
	MDUMP_TBUF,
	MDUMP_DRAM,
	MDUMP_TOTAL
} MDUMP_t;

#endif