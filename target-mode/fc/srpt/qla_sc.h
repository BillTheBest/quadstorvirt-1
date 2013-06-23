/* 
 * Copyright (C) Shivaram Upadhyayula <shivaram.u@quadstor.com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 as published by the Free Software Foundation
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301, USA.
 */

#ifndef QUADSTOR_QLA_SC_H_
#define QUADSTOR_QLA_SC_H_

#include <linuxdefs.h>
#include <exportdefs.h>
#include <missingdefs.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_transport_fc.h>
#include "ib_srpt.h"

typedef struct se_cmd tgtcmd_t;

static inline struct fcbridge *
notify_fcbridge(struct qsio_immed_notify *notify)
{
	DEBUG_BUG_ON(!notify->ccb_h.priv.qpriv.fcbridge);
	return notify->ccb_h.priv.qpriv.fcbridge;
}

static inline struct se_cmd *
notify_cmd(struct qsio_immed_notify *notify)
{
	DEBUG_BUG_ON(!notify->ccb_h.priv.qpriv.qcmd);
	return notify->ccb_h.priv.qpriv.qcmd;
}

static inline struct fcbridge *
ctio_fcbridge(struct qsio_scsiio *ctio)
{
	DEBUG_BUG_ON(!ctio->ccb_h.priv.qpriv.fcbridge);
	return ctio->ccb_h.priv.qpriv.fcbridge;
}

static inline tgtcmd_t *
ctio_cmd(struct qsio_scsiio *ctio)
{
	DEBUG_BUG_ON(!ctio->ccb_h.priv.qpriv.qcmd);
	return ctio->ccb_h.priv.qpriv.qcmd;
}

static inline int
ctio_direction(struct qsio_scsiio *ctio)
{
	if (!ctio->dxfer_len)
		return DMA_NONE;

	if (ctio->ccb_h.flags & QSIO_DATA_DIR_IN)
		return DMA_FROM_DEVICE;
	else if (ctio->ccb_h.flags & QSIO_DATA_DIR_OUT)
		return DMA_TO_DEVICE;
	else {
		DEBUG_BUG_ON(1);
		return DMA_NONE;
	}
}

void qla_sc_detach_fcbridge(void);
void qla_end_ccb(void *ccb_void);
void fcbridge_intr_insert(struct fcbridge *fcbridge, struct qsio_hdr *ccb_h);
void fcbridge_intr_remove(struct fcbridge *fcbridge, struct qsio_hdr *ccb_h);
void __local_ctio_free_all(struct qsio_scsiio *ctio);
void __ctio_free_data(struct qsio_scsiio *ctio);
void __ctio_free_all(struct qsio_scsiio *ctio, int local_pool);
struct qsio_scsiio * __local_ctio_new(allocflags_t flags);
int __ctio_queue_cmd(struct qsio_scsiio *ctio);
void fcbridge_free_initiator(uint64_t i_prt[], uint64_t t_prt[]);
int fcbridge_i_prt_valid(struct fcbridge *fcbridge, uint64_t i_prt[]);
void fcbridge_get_tport(struct fcbridge *fcbridge, uint64_t wwpn[]);

static inline void
print_wwn(unsigned char *wwn, char *str)
{
	int i;
	printk(KERN_INFO "%s: ", str);
	for (i = 0; i < 8; i++) {
		if (i != 7)
			printk("%02x:", (int)wwn[i]);
		else
			printk("%02x\n", (int)wwn[i]);
	}
}

static inline char *
se_data_dir(int dir)
{
	switch(dir) {
	case DMA_NONE:
		return "dma_none";
	case DMA_FROM_DEVICE:
		return "dma_from_device";
	case DMA_TO_DEVICE:
		return "dma_to_device";
	default:
		break;
	}
	return "unknown";
}

#endif
