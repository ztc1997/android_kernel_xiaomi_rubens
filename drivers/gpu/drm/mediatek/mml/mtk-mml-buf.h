/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 * Author: Dennis-YC Hsieh <dennis-yc.hsieh@mediatek.com>
 */
#ifndef __MTK_MML_BUF_H__
#define __MTK_MML_BUF_H__

#include <linux/of.h>

struct mml_file_buf;

/* mml_buf_get - get dma_buf instance from fd for later get iova
 *
 * buf:	the mml buffer struct
 * fd:	fd array from client
 * cnt:	count of fd array
 *
 * Note: Call mml_buf_put for same mml_file_buf later to release it.
 */
void mml_buf_get(struct mml_file_buf *buf, int32_t *fd, u8 cnt);

/* mml_buf_iova_get - get iova by device and dmabuf
 *
 * dev:	dma device, such as mml_rdma or mml_wrot
 * buf:	mml buffer structure to store buffer for planes
 *
 * Return: 0 success; error no if fail
 *
 * Note: Should be call from dma component. And this api may take time to sync
 * cache FROM CPU TO DMA.
 */
int mml_buf_iova_get(struct device *dev, struct mml_file_buf *buf);

/* mml_buf_iova_free - unmap and detach instance when get iova.
 *
 * buf:	mml buffer structure to store buffer for planes
 *
 * Note: iova will not reset to 0 but will unmap and should not use anymore,
 * except for debug dump. This API may take time to sync cache FROM DMA TO CPU
 */
void mml_buf_iova_free(struct mml_file_buf *buf);

/* mml_buf_put - release instance from mml_buf_get
 *
 * buf: the mml buffer struct
 */
void mml_buf_put(struct mml_file_buf *buf);


#endif