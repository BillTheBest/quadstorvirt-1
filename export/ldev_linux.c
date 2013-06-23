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

#include <scsi/scsi.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_tcq.h>
#include "ldev_linux.h"
#include "exportdefs.h"
#include "missingdefs.h"
#include "devq.h"

static const char * ldev_info(struct Scsi_Host *host);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0))
static int ldev_queuecommand(struct Scsi_Host *host, struct scsi_cmnd *SCpnt);
#else
static int ldev_queuecommand(struct scsi_cmnd *SCpnt, void (*done) (struct scsi_cmnd *));
#endif
static int ldev_abort(struct scsi_cmnd *SCpnt);
static int ldev_reset(struct scsi_cmnd *SCpnt);
static int ldev_new_device_cb(struct tdisk *newdevice);
static int ldev_remove_device_cb(struct tdisk *removedevice, int, void *);
static void ldev_disable_device_cb(struct tdisk *removedevice, int, void *);

struct qs_interface_cbs icbs = {
	.new_device = ldev_new_device_cb,
	.remove_device = ldev_remove_device_cb,
	.disable_device = ldev_disable_device_cb,
	.interface = TARGET_INT_LOCAL,
};

static struct scsi_host_template driver_template = {
		.name =				LDEV_NAME,
		.info =				ldev_info,
		.queuecommand =			ldev_queuecommand,
		.can_queue =			256,
		.this_id =			LDEV_HOST_ID,
		.sg_tablesize =			SG_ALL,
		.cmd_per_lun =			32,
		.max_sectors =			0xffff,
		.unchecked_isa_dma =		0,
		.use_clustering =		ENABLE_CLUSTERING,
		.module		=		THIS_MODULE,
		.eh_bus_reset_handler =		ldev_reset,
		.eh_device_reset_handler =	ldev_reset,
		.eh_host_reset_handler =	ldev_reset,
		.eh_abort_handler =		ldev_abort,
};

static const char *
ldev_info(struct Scsi_Host *host)
{
	return LDEV_NAME;
}

static int
ldev_new_device_cb(struct tdisk *newdevice)
{
	struct Scsi_Host *host;
	int retval;
	struct ldev_priv *priv;

	host = scsi_host_alloc(&driver_template, sizeof(struct ldev_priv));
	if (unlikely(!host)) {
		DEBUG_INFO("Unable to alloc a new scsi host\n");
		return -1;
	}
	host->max_cmd_len = 16;

	priv = (struct ldev_priv *)(host->hostdata);
	memset(priv, 0, sizeof(struct ldev_priv));
	priv->device = newdevice;
	priv->devq = devq_init(host->host_no, "ldev");

	if (unlikely(!priv->devq)) {
		scsi_host_put(host);	
		return -1;
	}

	retval = scsi_add_host(host, NULL);
	if (unlikely(retval != 0)) {
		DEBUG_INFO("scsi_add host failed\n");
		devq_exit(priv->devq);
		scsi_host_put(host);
		return -1;
	}

	(*icbs.device_set_hpriv)(newdevice, host);
	(*icbs.device_set_vhba_id)(newdevice, host->host_no, TARGET_INT_LOCAL);
	return host->host_no;
}

static void
ldev_disable_device_cb(struct tdisk *removedevice, int tid, void *hpriv)
{
	struct Scsi_Host *host = NULL;
	struct ldev_priv *priv;

	host = hpriv;
	if (unlikely(!host))
		return;

	priv = (struct ldev_priv *)(host->hostdata);
	atomic_set(&priv->disabled, 1);
}

static int
ldev_remove_device_cb(struct tdisk *removedevice, int tid, void *hpriv)
{
	struct Scsi_Host *host = NULL;
	struct ldev_priv *priv;
	struct qs_devq *devq;

	host = hpriv;
	if (unlikely(!host))
		return -1;

	priv = (struct ldev_priv *)(host->hostdata);
	atomic_set(&priv->disabled, 1);
	while (atomic_read(&priv->pending_cmds) > 0)
		msleep(1000);
	devq = priv->devq;
	scsi_remove_host(host);
	scsi_host_put(host);
	devq_exit(devq);
	return 0;
}


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25))
static void 
copy_out_request_buffer2(__u8 *dataptr, int dxfer_len, struct scsi_cmnd *SCpnt)
{
	int i;
	__u32 offset = 0;
	struct scatterlist *sgentry;

	if (!dxfer_len)
		return;

	if (scsi_sg_count(SCpnt) == 0) {
		memcpy(SCpnt->request_buffer, dataptr, dxfer_len);
		return;
	}

	sgentry = scsi_sglist(SCpnt);
	for (i = 0; i < scsi_sg_count(SCpnt); i++) { 
		__u8 *out_addr;
		int min;

		min = min_t(int, (dxfer_len - offset), sgentry->length);

		out_addr = page_address(sg_page(sgentry))+sgentry->offset;
		memcpy(out_addr, dataptr+offset, min); 

		offset += min; 
		sgentry++;
	}
	DEBUG_BUG_ON(offset != dxfer_len);
	return;
}
#else
static void 
copy_out_request_buffer2(__u8 *dataptr, int dxfer_len, struct scsi_cmnd *SCpnt)
{
	int min_len = min_t(int, dxfer_len, scsi_bufflen(SCpnt));

	if (!min_len)
		return;

	sg_copy_from_buffer(scsi_sglist(SCpnt), scsi_sg_count(SCpnt), dataptr, min_len);
}
#endif

static void 
copy_out_request_buffer(struct pgdata **pglist, int pglist_cnt, struct scsi_cmnd *SCpnt, __u32 dxfer_len)
{
	int i;
	int offset = 0;
	int sgoffset;
	int pgoffset;
	int j;
	struct scatterlist *sgentry;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25))
	if (scsi_sg_count(SCpnt) == 0) {
		int min_len;

		for (i = 0; i < pglist_cnt; i++) { 
			struct pgdata *pgtmp = pglist[i];

			min_len = min_t(int, (dxfer_len - offset), pgtmp->pg_len);
			memcpy(SCpnt->request_buffer+offset, page_address(pgtmp->page) + pgtmp->pg_offset, min_len); 
			offset += min_len;
			if (offset == dxfer_len)
				break;
		}
		return;
	}
#endif

	i = 0;
	j = 0;
	sgoffset = 0;
	pgoffset = 0;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25))
	sgentry = scsi_sglist(SCpnt);
	while (j < scsi_sg_count(SCpnt)) {
#else
	for_each_sg(scsi_sglist(SCpnt), sgentry, scsi_sg_count(SCpnt), j) {
#endif
		__u8 *in_addr;
		int min;
		int in_avail;
		struct pgdata *pgtmp;
		__u8 *out_addr;
		int out_avail;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,25))
again:
#endif
		pgtmp = pglist[i];
		out_avail = min_t(int, (dxfer_len - offset), pgtmp->pg_len - pgoffset); 
		DEBUG_BUG_ON(pgoffset > pgtmp->pg_len);

		DEBUG_BUG_ON(sgoffset > sgentry->length);
		in_avail = sgentry->length - sgoffset;
		min = min_t(int, in_avail, out_avail);

		out_addr = page_address(pgtmp->page)+ pgtmp->pg_offset + pgoffset;
		in_addr = page_address(sg_page(sgentry))+sgentry->offset+sgoffset;
		memcpy(in_addr, out_addr, min);

		sgoffset += min;
		pgoffset += min;
		out_avail -= min;
		offset += min;

		if (sgoffset == sgentry->length) {
			sgoffset = 0;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25))
			j++;
			sgentry++;
#endif
		}

 		if (offset == dxfer_len)
			break;

		if (pgoffset == pgtmp->pg_len) {
			pgoffset = 0;
			i++;
			if (i == pglist_cnt)
				break;
		}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,25))
		if (sgoffset)
			goto again;
#endif
	}
}

static void 
ldev_send_ccb(void *ccb_void)
{
	struct qsio_scsiio *ctio = (struct qsio_scsiio *)ccb_void;
	struct vhba_priv *vhba_priv = &ctio->ccb_h.priv.vpriv;
	struct scsi_cmnd *SCpnt = vhba_priv->SCpnt;
	struct Scsi_Host *host = SCpnt->device->host;
	struct ldev_priv *priv = (struct ldev_priv *)(host->hostdata);
	struct ccb_list ctio_list;

	STAILQ_INIT(&ctio_list);
	(*icbs.device_remove_ctio)(ctio, &ctio_list);
	DEBUG_BUG_ON(!atomic_read(&priv->pending_cmds));
	atomic_dec(&priv->pending_cmds);
	if (ctio->scsi_status == SCSI_STATUS_OK) {
		SCpnt->result = DID_OK;	
	}
	else if (ctio->scsi_status == SCSI_STATUS_CHECK_COND) {
		DEBUG_BUG_ON(!ctio->sense_data);
		memcpy(SCpnt->sense_buffer, ctio->sense_data, ctio->sense_len);
		SCpnt->result = (DRIVER_SENSE << 24) | SAM_STAT_CHECK_CONDITION;
	}
	else if (ctio->scsi_status == SCSI_STATUS_BUSY) {
		SCpnt->result = (DID_OK << 16) | ctio->scsi_status;
	}
	else {
		SCpnt->result = (DID_ERROR << 16) | ctio->scsi_status;
	}

	if (ctio->pglist_cnt > 0) {
		if (!ctio_norefs(ctio))
			copy_out_request_buffer((struct pgdata **)ctio->data_ptr, ctio->pglist_cnt, SCpnt, ctio->dxfer_len);
	}
	else if (ctio->dxfer_len > 0) {
		copy_out_request_buffer2(ctio->data_ptr, ctio->dxfer_len, SCpnt);
	}

	if (SCpnt->sc_data_direction == DMA_FROM_DEVICE) {
		int resid = scsi_get_resid(SCpnt);

		if (resid)
			resid -= ctio->dxfer_len;
		else
			resid = scsi_bufflen(SCpnt) - ctio->dxfer_len;
		scsi_set_resid(SCpnt, resid); 
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0))
	SCpnt->scsi_done(SCpnt);
#else
	vhba_priv->done(SCpnt);
#endif
	(*icbs.ctio_free_all)(ctio);
	(*icbs.device_queue_ctio_list)(&ctio_list);
}

void 
ldev_proc_cmd(struct qsio_scsiio *ctio)
{
	struct tdisk *device;
	struct vhba_priv *vhba_priv = &ctio->ccb_h.priv.vpriv;
	struct scsi_cmnd *SCpnt = vhba_priv->SCpnt;
	struct Scsi_Host *host = SCpnt->device->host;
	struct ldev_priv *priv = (struct ldev_priv *)(host->hostdata);

	device = priv->device;
	ctio->ccb_h.tdisk = device;
	ctio->ccb_h.queue_fn = ldev_send_ccb;
	atomic_inc(&priv->pending_cmds);
	(*icbs.device_queue_ctio)(device, ctio);
}

uint64_t t_prt;
static struct qsio_scsiio *
construct_ctio(struct scsi_cmnd *SCpnt)
{
	struct qsio_scsiio *ctio;
	__u8 *cmnd;	
	struct ldev_priv *priv;
	struct Scsi_Host *host;
	__u8 tag[2];

	host = SCpnt->device->host;
	priv = (struct ldev_priv *)(host->hostdata);
	DEBUG_INFO("SCpnt Target %d lun %d\n", SCpnt->device->id, SCpnt->device->lun);
	ctio = (*icbs.ctio_new)(Q_NOWAIT_INTR);
	if (unlikely(!ctio))
		return NULL;

	if (unlikely(!t_prt))
		t_prt = (*icbs.get_tprt)();

	ctio->i_prt[0] = LDEV_HOST_ID;
	ctio->t_prt[0] = t_prt;
	ctio->init_int = TARGET_INT_LOCAL;
	ctio->r_prt = LDEV_RPORT_START; 
	cmnd = SCpnt->cmnd;
	memcpy(ctio->cdb, SCpnt->cmnd, 16); 
	ctio->ccb_h.flags = QSIO_DIR_OUT;
	ctio->ccb_h.queue_fn = ldev_send_ccb;
	ctio->ccb_h.tdisk = priv->device;

	if (scsi_populate_tag_msg(SCpnt, tag)) {
		switch (tag[0]) {
		case MSG_HEAD_OF_QUEUE_TASK:
		case MSG_ORDERED_TASK:
			break;
		default:
			tag[0] = MSG_SIMPLE_TASK;
			break;
		}
		ctio->task_attr = tag[0];
		ctio->task_tag = tag[1];
	}
	else {
		ctio->task_attr = MSG_SIMPLE_TASK; 
	}
	return ctio;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0))
static int ldev_queuecommand(struct Scsi_Host *host, struct scsi_cmnd *SCpnt)
#else
static int ldev_queuecommand(struct scsi_cmnd *SCpnt, void (*done) (struct scsi_cmnd *))
#endif
{
	struct qsio_scsiio *ctio;
	struct vhba_priv *vhba_priv;
	struct ldev_priv *priv;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0))
	struct Scsi_Host *host = SCpnt->device->host;
#endif

	priv = (struct ldev_priv *)(host->hostdata);

	if (atomic_read(&priv->disabled)) {
		SCpnt->result = DID_ERROR << 16;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0))
		SCpnt->scsi_done(SCpnt);
#else
		done(SCpnt);
#endif
		return 0;
	}

	if (SCpnt->device->id) {
		SCpnt->result = DID_BAD_TARGET << 16;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0))
		SCpnt->scsi_done(SCpnt);
#else
		done(SCpnt);
#endif
		return 0;
	}

	DEBUG_INFO("Entered ldev_queuecomman\n");
	ctio = construct_ctio(SCpnt);

	if (!ctio) {
		DEBUG_INFO("Unable to construct ctio\n");
		SCpnt->result = DID_ERROR << 16;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0))
		SCpnt->scsi_done(SCpnt);
#else
		done(SCpnt);
#endif
		return 0;
	}

	vhba_priv = &ctio->ccb_h.priv.vpriv;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0))
	vhba_priv->done = done;
#endif
	vhba_priv->SCpnt = SCpnt;

	devq_insert_ccb(priv->devq, (struct qsio_hdr *)ctio);
	return 0;
}

static int
ldev_abort(struct scsi_cmnd *SCpnt)
{
	return FAILED;
}

static int
ldev_reset(struct scsi_cmnd *SCpnt)
{
	struct Scsi_Host *host = SCpnt->device->host;
	struct ldev_priv *priv = (struct ldev_priv *)(host->hostdata);

	while (atomic_read(&priv->pending_cmds) > 0)
		msleep(1000);

	return SUCCESS;
}

static int
ldev_init(void)
{
	int retval;

	retval = device_register_interface(&icbs);
	if (unlikely(retval != 0)) {
		return -1;
	}

	return 0;
}

static void
ldev_exit(void)
{
	device_unregister_interface(&icbs);
	return;
}

module_init(ldev_init);
module_exit(ldev_exit);
