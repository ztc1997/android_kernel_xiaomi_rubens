// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 *
 * Author: Shih-Fang Chuang <shih-fang.chuang@mediatek.com>
 *
 */

 // Standard C header file

// kernel header file
#include <linux/device.h>
#include <linux/of_address.h>
#include <linux/dma-iommu.h>
#include <linux/pm_runtime.h>
#include <linux/remoteproc.h>

// mtk imgsys local header file

// Local header file
#include "mtk_imgsys-traw.h"

/********************************************************************
 * Global Define
 ********************************************************************/
#define TRAW_INIT_ARRAY_COUNT	10

#define TRAW_MAX_ADDR_OFST	0xB620
#define TRAW_CTL_ADDR_OFST	0x1ED0
#define TRAW_DATA_ADDR_OFST	0x8000


#define TRAW_HW_SET		2



/********************************************************************
 * Global Variable
 ********************************************************************/
const struct mtk_imgsys_init_array
			mtk_imgsys_traw_init_ary[TRAW_INIT_ARRAY_COUNT] = {
	{0x00A0, 0x80000000}, /* TRAWCTL_INT1_EN */
	{0x121C, 0x80000100}, /* IMGI_T1A_REG_ORIRDMA_CON0 */
	{0x1220, 0x11000100}, /* IMGI_T1A_REG_ORIRDMA_CON1 */
	{0x1224, 0x11000100}, /* IMGI_T1A_REG_ORIRDMA_CON2 */
	{0x12DC, 0x80000080}, /* IMGBI_T1A_REG_ORIRDMA_CON0 */
	{0x12E0, 0x10800080}, /* IMGBI_T1A_REG_ORIRDMA_CON1 */
	{0x12E4, 0x10800080}, /* IMGBI_T1A_REG_ORIRDMA_CON2 */
	{0x133C, 0x80000080}, /* IMGCI_T1A_REG_ORIRDMA_CON0 */
	{0x1340, 0x10800080}, /* IMGCI_T1A_REG_ORIRDMA_CON1 */
	{0x1344, 0x10800080}, /* IMGCI_T1A_REG_ORIRDMA_CON2 */

};

static struct TRAWDmaDebugInfo g_DMADbgIfo[] = {
	{"IMGI", TRAW_ORI_RDMA_DEBUG},
	{"IMGI_UFD", TRAW_ORI_RDMA_UFD_DEBUG},
	{"UFDI", TRAW_ORI_RDMA_DEBUG},
	{"IMGBI", TRAW_ORI_RDMA_DEBUG},
	{"IMGBI_UFD", TRAW_ORI_RDMA_UFD_DEBUG},
	{"IMGCI", TRAW_ORI_RDMA_DEBUG},
	{"IMGCI_UFD", TRAW_ORI_RDMA_UFD_DEBUG},
	{"SMTI_T1", TRAW_ULC_RDMA_DEBUG},
	{"SMTI_T2", TRAW_ULC_RDMA_DEBUG},
	{"SMTI_T3", TRAW_ULC_RDMA_DEBUG},
	{"SMTI_T4", TRAW_ULC_RDMA_DEBUG},
	{"SMTI_T5", TRAW_ULC_RDMA_DEBUG},
	{"CACI", TRAW_ULC_RDMA_DEBUG},
	{"TNCSTI_T1", TRAW_ULC_RDMA_DEBUG},
	{"TNCSTI_T2", TRAW_ULC_RDMA_DEBUG},
	{"TNCSTI_T3", TRAW_ULC_RDMA_DEBUG},
	{"TNCSTI_T4", TRAW_ULC_RDMA_DEBUG},
	{"TNCSTI_T5", TRAW_ULC_RDMA_DEBUG},
	{"YUVO_T1", TRAW_ORI_WDMA_DEBUG},
	{"YUVBO_T1", TRAW_ORI_WDMA_DEBUG},
	{"TIMGO", TRAW_ORI_WDMA_DEBUG},
	{"YUVCO", TRAW_ORI_WDMA_DEBUG},
	{"YUVDO", TRAW_ORI_WDMA_DEBUG},
	{"YUVO_T2", TRAW_ULC_WDMA_DEBUG},
	{"YUVBO_T2", TRAW_ULC_WDMA_DEBUG},
	{"YUVO_T3", TRAW_ULC_WDMA_DEBUG},
	{"YUVBO_T3", TRAW_ULC_WDMA_DEBUG},
	{"YUVO_T4", TRAW_ULC_WDMA_DEBUG},
	{"YUVBO_T4", TRAW_ULC_WDMA_DEBUG},
	{"YUVO_T5", TRAW_ORI_WDMA_DEBUG},
	{"TNCSO", TRAW_ULC_WDMA_DEBUG},
	{"TNCSBO", TRAW_ULC_WDMA_DEBUG},
	{"TNCSHO", TRAW_ULC_WDMA_DEBUG},
	{"TNCSYO", TRAW_ULC_WDMA_DEBUG},
	{"SMTO_T1", TRAW_ULC_WDMA_DEBUG},
	{"SMTO_T2", TRAW_ULC_WDMA_DEBUG},
	{"SMTO_T3", TRAW_ULC_WDMA_DEBUG},
	{"SMTO_T4", TRAW_ULC_WDMA_DEBUG},
	{"SMTO_T5", TRAW_ULC_WDMA_DEBUG},
	{"TNCSTO_T1", TRAW_ULC_WDMA_DEBUG},
	{"TNCSTO_T2", TRAW_ULC_WDMA_DEBUG},
	{"TNCSTO_T3", TRAW_ULC_WDMA_DEBUG}
};

static unsigned int g_RegBaseAddr = TRAW_A_BASE_ADDR;

static unsigned int ExeDbgCmd(struct mtk_imgsys_dev *a_pDev,
			void __iomem *a_pRegBA,
			unsigned int a_DdbSel,
			unsigned int a_DbgOut,
			unsigned int a_DbgCmd)
{
	unsigned int DbgData = 0;
	unsigned int DbgOutReg = g_RegBaseAddr + a_DbgOut;
	void __iomem *pDbgSel = (void *)(a_pRegBA + a_DdbSel);
	void __iomem *pDbgPort = (void *)(a_pRegBA + a_DbgOut);

	iowrite32(a_DbgCmd, pDbgSel);
	DbgData = (unsigned int)ioread32(pDbgPort);
	pr_info("[0x%08X](0x%08X,0x%08X)\n",
		a_DbgCmd, DbgOutReg, DbgData);

	return DbgData;
}

static void imgsys_traw_dump_dma(struct mtk_imgsys_dev *a_pDev,
				void __iomem *a_pRegBA,
				unsigned int a_DdbSel,
				unsigned int a_DbgOut)
{
	unsigned int Idx = 0;
	unsigned int DbgCmd = 0;
	unsigned int DmaDegInfoSize = sizeof(struct TRAWDmaDebugInfo);
	unsigned int DebugCnt = sizeof(g_DMADbgIfo)/DmaDegInfoSize;
	enum TRAWDmaDebugType DbgTy = TRAW_ORI_RDMA_DEBUG;

	/* Dump DMA Debug Info */
	for (Idx = 0; Idx < DebugCnt; Idx++) {
		/* state_checksum */
		DbgCmd = TRAW_IMGI_STATE_CHECKSUM + Idx;
		ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
		/* line_pix_cnt_tmp */
		DbgCmd = TRAW_IMGI_LINE_PIX_CNT_TMP + Idx;
		ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
		/* line_pix_cnt */
		DbgCmd = TRAW_IMGI_LINE_PIX_CNT + Idx;
		ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);

		DbgTy = g_DMADbgIfo[Idx].DMADebugType;

		/* important_status */
		if (DbgTy == TRAW_ULC_RDMA_DEBUG ||
			DbgTy == TRAW_ULC_WDMA_DEBUG) {
			DbgCmd = TRAW_IMGI_IMPORTANT_STATUS + Idx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
			DbgCmd);
		}

		/* smi_debug_data (case 0) or cmd_data_cnt */
		if (DbgTy == TRAW_ORI_RDMA_DEBUG ||
			DbgTy == TRAW_ULC_RDMA_DEBUG ||
			DbgTy == TRAW_ULC_WDMA_DEBUG) {
			DbgCmd = TRAW_IMGI_SMI_DEBUG_DATA_CASE0 + Idx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
			DbgCmd);
		}

		/* ULC_RDMA or ULC_WDMA */
		if (DbgTy == TRAW_ULC_RDMA_DEBUG ||
			DbgTy == TRAW_ULC_WDMA_DEBUG) {
			DbgCmd = TRAW_IMGI_TILEX_BYTE_CNT + Idx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
				DbgCmd);
			DbgCmd = TRAW_IMGI_TILEY_CNT + Idx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
				DbgCmd);
		}

		/* smi_dbg_data(case 0) or burst_line_cnt or input_v_cnt */
		if (DbgTy == TRAW_ORI_WDMA_DEBUG ||
			DbgTy == TRAW_ULC_RDMA_DEBUG ||
			DbgTy == TRAW_ULC_WDMA_DEBUG) {
			DbgCmd = TRAW_IMGI_BURST_LINE_CNT + Idx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
				DbgCmd);
		}

		/* ORI_RDMA */
		if (DbgTy == TRAW_ORI_RDMA_DEBUG) {
			DbgCmd = TRAW_IMGI_FIFO_DEBUG_DATA_CASE1 + Idx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
				DbgCmd);
			DbgCmd = TRAW_IMGI_FIFO_DEBUG_DATA_CASE3 + Idx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
				DbgCmd);
		}

		/* ORI_WDMA */
		if (DbgTy == TRAW_ORI_WDMA_DEBUG) {
			DbgCmd = TRAW_YUVO_T1_FIFO_DEBUG_DATA_CASE1 + Idx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
				DbgCmd);
			DbgCmd = TRAW_YUVO_T1_FIFO_DEBUG_DATA_CASE3 + Idx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
				DbgCmd);
		}

		/* xfer_y_cnt */
		if (DbgTy == TRAW_ULC_WDMA_DEBUG) {
			DbgCmd = TRAW_IMGI_XFER_Y_CNT + Idx;
			ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut,
				DbgCmd);
		}

	}

}

static void imgsys_traw_dump_cq(struct mtk_imgsys_dev *a_pDev,
				void __iomem *a_pRegBA,
				unsigned int a_DdbSel,
				unsigned int a_DbgOut)
{
	unsigned int DbgCmd = 0;
	void __iomem *pCQEn = (void *)(a_pRegBA + TRAW_DIPCQ_CQ_EN);


	/* arx/atx/drx/dtx_state */
	DbgCmd = 0x00000005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* Thr(0~3)_state */
	DbgCmd = 0x00010005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);

	/* Set DIPCQ_CQ_EN[28] to 1 */
	iowrite32(0x10000000, pCQEn);
	/* cqd0_checksum0 */
	DbgCmd = 0x00000005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* cqd0_checksum1 */
	DbgCmd = 0x00010005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* cqd0_checksum2 */
	DbgCmd = 0x00020005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* cqd1_checksum0 */
	DbgCmd = 0x00040005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* cqd1_checksum1 */
	DbgCmd = 0x00050005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* cqd1_checksum2 */
	DbgCmd = 0x00060005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* cqa0_checksum0 */
	DbgCmd = 0x00080005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* cqa0_checksum1 */
	DbgCmd = 0x00090005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* cqa0_checksum2 */
	DbgCmd = 0x000A0005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* cqa1_checksum0 */
	DbgCmd = 0x000C0005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* cqa1_checksum1 */
	DbgCmd = 0x000D0005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* cqa1_checksum2 */
	DbgCmd = 0x000E0005;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);

}

static void imgsys_traw_dump_drzh2n(struct mtk_imgsys_dev *a_pDev,
				void __iomem *a_pRegBA,
				unsigned int a_DdbSel,
				unsigned int a_DbgOut)
{
	unsigned int DbgCmd = 0;


	/* drzh2n_t1 line_pix_cnt_tmp */
	DbgCmd = 0x0002C001;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t1 line_pix_cnt */
	DbgCmd = 0x0003C001;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t1 handshake signal */
	DbgCmd = 0x0004C001;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t2 line_pix_cnt_tmp */
	DbgCmd = 0x0002C101;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t2 line_pix_cnt */
	DbgCmd = 0x0003C101;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t2 handshake signal */
	DbgCmd = 0x0004C101;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t3 line_pix_cnt_tmp */
	DbgCmd = 0x0002C501;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t3 line_pix_cnt */
	DbgCmd = 0x0003C501;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t3 handshake signal */
	DbgCmd = 0x0004C501;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t4 line_pix_cnt_tmp */
	DbgCmd = 0x0002C601;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t4 line_pix_cnt */
	DbgCmd = 0x0003C601;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t4 handshake signal */
	DbgCmd = 0x0004C601;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t5 line_pix_cnt_tmp */
	DbgCmd = 0x0002C701;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t5 line_pix_cnt */
	DbgCmd = 0x0003C701;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t5 handshake signal */
	DbgCmd = 0x0004C701;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t6 line_pix_cnt_tmp */
	DbgCmd = 0x0002C801;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t6 line_pix_cnt */
	DbgCmd = 0x0003C801;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* drzh2n_t6 handshake signal */
	DbgCmd = 0x0004C801;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* rzh1n2t_t1 checksum */
	DbgCmd = 0x0001C201;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* rzh1n2t_t1 tile line_pix_cnt */
	DbgCmd = 0x0003C201;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* rzh1n2t_t1 tile protocal */
	DbgCmd = 0x0008C201;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);

}

static void imgsys_traw_dump_smto(struct mtk_imgsys_dev *a_pDev,
				void __iomem *a_pRegBA,
				unsigned int a_DdbSel,
				unsigned int a_DbgOut)
{
	unsigned int DbgCmd = 0;


	/* smto_t3 line_pix_cnt_tmp */
	DbgCmd = 0x0002C401;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* smto_t3 line_pix_cnt */
	DbgCmd = 0x0003C401;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* smto_t3 handshake signal */
	DbgCmd = 0x0004C401;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);

}

static void imgsys_traw_dump_cac(struct mtk_imgsys_dev *a_pDev,
				void __iomem *a_pRegBA,
				unsigned int a_DdbSel,
				unsigned int a_DbgOut)
{
	unsigned int DbgCmd = 0;


	/* checksum_st */
	DbgCmd = 0x00010801;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* checksum_line_pix_cnt_tmp */
	DbgCmd = 0x00020801;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* checksum_line_pix_cnt */
	DbgCmd = 0x00030801;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* in_interface */
	DbgCmd = 0x00040801;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	/* ou_interface */
	DbgCmd = 0x00050801;
	ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);

}


static void imgsys_traw_dump_dl(struct mtk_imgsys_dev *a_pDev,
				void __iomem *a_pRegBA,
				unsigned int a_DdbSel,
				unsigned int a_DbgOut)
{
	unsigned int DbgCmd = 0;
	unsigned int DbgData = 0;
	unsigned int DbgLineCnt = 0, DbgRdy = 0, DbgReq = 0;
	unsigned int DbgLineCntReg = 0;


	/* wpe_wif_t1_debug */
	/* sot_st,eol_st,eot_st,sof,sot,eol,eot,req,rdy,7b0,checksum_out */
	DbgCmd = 0x00000006;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgRdy = ((DbgData & 0x800000) > 0) ? 1 : 0;
	DbgReq = ((DbgData & 0x1000000) > 0) ? 1 : 0;
	pr_info("[wpe_wif_t1_debug]checksum(0x%X),rdy(%d) req(%d)\n",
		DbgData & 0xFFFF, DbgRdy, DbgReq);
	/* line_cnt[15:0],  pix_cnt[15:0] */
	DbgCmd = 0x00000007;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCnt = (DbgData & 0xFFFF0000) / 0xFFFF;
	pr_info("[wpe_wif_t1_debug]pix_cnt(0x%X),line_cnt(0x%X)\n",
		DbgData & 0xFFFF, DbgLineCnt);
	/* line_cnt_reg[15:0], pix_cnt_reg[15:0] */
	DbgCmd = 0x00000008;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCntReg = (DbgData & 0xFFFF0000) / 0xFFFF;
	pr_info("[wpe_wif_t1_debug]pix_cnt_reg(0x%X),line_cnt_reg(0x%X)\n",
		DbgData & 0xFFFF, DbgData & 0xFFFF0000);

	/* wpe_wif_t2_debug */
	/* sot_st,eol_st,eot_st,sof,sot,eol,eot,req,rdy,7b0,checksum_out */
	DbgCmd = 0x00000106;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgRdy = ((DbgData & 0x800000) > 0) ? 1 : 0;
	DbgReq = ((DbgData & 0x1000000) > 0) ? 1 : 0;
	pr_info("[wpe_wif_t2_debug]checksum(0x%X),rdy(%d) req(%d)\n",
		DbgData & 0xFFFF, DbgRdy, DbgReq);
	/* line_cnt[15:0],  pix_cnt[15:0] */
	DbgCmd = 0x00000107;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCnt = (DbgData & 0xFFFF0000) / 0xFFFF;
	pr_info("[wpe_wif_t2_debug]pix_cnt(0x%X),line_cnt(0x%X)\n",
		DbgData & 0xFFFF, DbgLineCnt);
	/* line_cnt_reg[15:0], pix_cnt_reg[15:0] */
	DbgCmd = 0x00000108;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCntReg = (DbgData & 0xFFFF0000) / 0xFFFF;
	pr_info("[wpe_wif_t2_debug]pix_cnt_reg(0x%X),line_cnt_reg(0x%X)\n",
		DbgData & 0xFFFF, DbgLineCntReg);

	/* traw_dip_d1_debug */
	/* sot_st,eol_st,eot_st,sof,sot,eol,eot,req,rdy,7b0,checksum_out */
	DbgCmd = 0x00000206;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgRdy = ((DbgData & 0x800000) > 0) ? 1 : 0;
	DbgReq = ((DbgData & 0x1000000) > 0) ? 1 : 0;
	pr_info("[traw_dip_d1_debug]checksum(0x%X),rdy(%d) req(%d)\n",
		DbgData & 0xFFFF, DbgRdy, DbgReq);
	/* line_cnt[15:0],  pix_cnt[15:0] */
	DbgCmd = 0x00000207;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCnt = (DbgData & 0xFFFF0000) / 0xFFFF;
	pr_info("[traw_dip_d1_debug]pix_cnt(0x%X),line_cnt(0x%X)\n",
		DbgData & 0xFFFF, DbgLineCnt);
	/* line_cnt_reg[15:0], pix_cnt_reg[15:0] */
	DbgCmd = 0x00000208;
	DbgData = ExeDbgCmd(a_pDev, a_pRegBA, a_DdbSel, a_DbgOut, DbgCmd);
	DbgLineCntReg = (DbgData & 0xFFFF0000) / 0xFFFF;
	pr_info("[traw_dip_d1_debug]pix_cnt_reg(0x%X),line_cnt_reg(0x%X)\n",
		DbgData & 0xFFFF, DbgLineCntReg);


}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Public Functions
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void imgsys_traw_set_initial_value(struct mtk_imgsys_dev *imgsys_dev)
{
	void __iomem *trawRegBA = 0L;
	void __iomem *ofset = NULL;
	unsigned int i = 0, HwIdx = 0, RegMap = REG_MAP_E_TRAW;

	pr_info("%s: +\n", __func__);

	for (HwIdx = 0; HwIdx < TRAW_HW_SET; HwIdx++) {
		/* iomap registers */
		RegMap += HwIdx;
		trawRegBA = of_iomap(imgsys_dev->dev->of_node, RegMap);
		for (i = 0 ; i < TRAW_INIT_ARRAY_COUNT ; i++) {
			ofset = trawRegBA + mtk_imgsys_traw_init_ary[i].ofset;
			writel(mtk_imgsys_traw_init_ary[i].val, ofset);
		}
	}

	pr_info("%s: -\n", __func__);
}
EXPORT_SYMBOL(imgsys_traw_set_initial_value);

void imgsys_traw_debug_dump(struct mtk_imgsys_dev *imgsys_dev,
							unsigned int engine)
{
	void __iomem *trawRegBA = 0L;
	unsigned int i;
	unsigned int DMADdbSel = TRAW_DMA_DBG_SEL;
	unsigned int DMADbgOut = TRAW_DMA_DBG_PORT;
	unsigned int CtlDdbSel = TRAW_CTL_DBG_SEL;
	unsigned int CtlDbgOut = TRAW_CTL_DBG_PORT;
	unsigned int RegMap = REG_MAP_E_TRAW;
	char DbgStr[128];

	pr_info("%s: +\n", __func__);

	/* ltraw */
	if (engine & IMGSYS_ENG_LTR) {
		RegMap = REG_MAP_E_LTRAW;
		g_RegBaseAddr = TRAW_B_BASE_ADDR;
	}
	/* traw */
	else
		g_RegBaseAddr = TRAW_A_BASE_ADDR;

	/* iomap registers */
	trawRegBA = of_iomap(imgsys_dev->dev->of_node, RegMap);
	if (!trawRegBA) {
		dev_info(imgsys_dev->dev, "%s Unable to ioremap regmap(%d)\n",
			__func__, RegMap);
		dev_info(imgsys_dev->dev, "%s of_iomap fail, devnode(%s).\n",
			__func__, imgsys_dev->dev->of_node->name);
	}
	/* DL debug data */
	imgsys_traw_dump_dl(imgsys_dev, trawRegBA, CtlDdbSel, CtlDbgOut);

	/* Traw or Ltraw ctrl registers */
	for (i = 0x0; i <= TRAW_CTL_ADDR_OFST; i += 16) {
		if (sprintf(DbgStr, "[0x%08X] 0x%08X 0x%08X 0x%08X 0x%08X",
			(unsigned int)(g_RegBaseAddr + i),
			(unsigned int)ioread32((void *)(trawRegBA + i)),
			(unsigned int)ioread32((void *)(trawRegBA + i + 4)),
			(unsigned int)ioread32((void *)(trawRegBA + i + 8)),
			(unsigned int)ioread32((void *)(trawRegBA + i + 12))) > 0)
			pr_info("%s\n", DbgStr);
	}
	/* Traw or Ltraw data registers */
	for (i = TRAW_DATA_ADDR_OFST; i <= TRAW_MAX_ADDR_OFST; i += 16) {
		if (sprintf(DbgStr, "[0x%08X] 0x%08X 0x%08X 0x%08X 0x%08X",
			(unsigned int)(g_RegBaseAddr + i),
			(unsigned int)ioread32((void *)(trawRegBA + i)),
			(unsigned int)ioread32((void *)(trawRegBA + i + 4)),
			(unsigned int)ioread32((void *)(trawRegBA + i + 8)),
			(unsigned int)ioread32((void *)(trawRegBA + i + 12))) > 0)
			pr_info("%s\n", DbgStr);
	}

	/* DMA debug data */
	imgsys_traw_dump_dma(imgsys_dev, trawRegBA, DMADdbSel, DMADbgOut);
	/* CQ debug data */
	imgsys_traw_dump_cq(imgsys_dev, trawRegBA, CtlDdbSel, CtlDbgOut);
	/* DRZH2N debug data */
	imgsys_traw_dump_drzh2n(imgsys_dev, trawRegBA, CtlDdbSel, CtlDbgOut);
	/* SMTO debug data */
	imgsys_traw_dump_smto(imgsys_dev, trawRegBA, CtlDdbSel, CtlDbgOut);
	/* CAC debug data */
	imgsys_traw_dump_cac(imgsys_dev, trawRegBA, CtlDdbSel, CtlDbgOut);


	pr_info("%s: -\n", __func__);
}
EXPORT_SYMBOL(imgsys_traw_debug_dump);