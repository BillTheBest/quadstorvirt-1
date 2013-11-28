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

#ifndef QUADSTOR_VDEVDEFS_H_
#define QUADSTOR_VDEVDEFS_H_ 1

#include "tdisk.h"

#define VENDOR_ID_QUADSTOR		"QUADSTOR"
#define PRODUCT_ID_QUADSTOR_SAN		"VDISK"
#define PRODUCT_ID_QUADSTOR_FCBRIDGE	"FCSCBRIDGE"
#define PRODUCT_REVISION_QUADSTOR	"2.0"

#define STANDARD_INQUIRY_LEN		36 
#define STANDARD_INQUIRY_LEN_MC		56
#define STANDARD_INQUIRY_LEN_SPC2	96
#define STANDARD_INQUIRY_LEN_SPC3	96

#define AERC_MASK		0x80
#define TRMTSK_MASK		0x40
#define NORM_ACA_MASK		0x20
#define HISUP_MASK		0x10

/* supported versions */
#define ANSI_VERSION_SCSI3	0x03 /* SPC */
#define ANSI_VERSION_SCSI3_SPC2	0x04 /* SPC2 */
#define ANSI_VERSION_SCSI3_SPC3	0x05 /* SPC3 */
#define ANSI_VERSION_SCSI3_SPC4	0x06 /* SPC4 */
#define RESPONSE_DATA		0x02 /* SPC-3  and SPC2-3*/

#define READ_WRITE_ERROR_RECOVERY_PAGE			0x01 
#define DISCONNECT_RECONNECT_PAGE			0x02
#define CACHING_MODE_PAGE				0x08
#define CONTROL_MODE_PAGE				0x0A
#define LOGICAL_BLOCK_PROVISIONING_MODE_PAGE		0x1C

#define DEVICE_CONFIGURATION_PAGE			0x10
#define INFORMATIONAL_EXCEPTIONS_CONTROL_PAGE		0x1C

#define VITAL_PRODUCT_DATA_PAGE				0x00
#define UNIT_SERIAL_NUMBER_PAGE				0x80
#define DEVICE_IDENTIFICATION_PAGE			0x83
#define EXTENDED_INQUIRY_VPD_PAGE			0x86
#define THIRD_PARTY_COPY_VPD_PAGE			0x8F
#define BLOCK_LIMITS_VPD_PAGE				0xB0
#define BLOCK_DEVICE_CHARACTERISTICS_VPD_PAGE		0xB1
#define LOGICAL_BLOCK_PROVISIONING_VPD_PAGE		0xB2

#define FIRMWARE_REVISION_PAGE				0xC0
#define VENDOR_SPECIFIC_PAGE2				0xC1

#define ELEMENT_ADDRESS_ASSIGNMENT_PAGE			0x1D
#define TRANSPORT_GEOMETRY_DESCRIPTOR_PAGE		0x1E
#define DEVICE_CAPABILITIES_PAGE			0x1F

/* both */
#define ALL_PAGES					0x3F

#define CODE_SET_ASCII		0x02

/* Identifier types */
#define UNIT_IDENTIFIER_VENDOR_SPECIFIC		0x00
#define UNIT_IDENTIFIER_T10_VENDOR_ID		0x01
#define UNIT_IDENTIFIER_EUI64			0x02
#define UNIT_IDENTIFIER_NAA			0x03

#define PERSISTENT_RESERVE_MAX_REGISTRATIONS	32
#define SERVICE_ACTION_READ_KEYS		0x00
#define SERVICE_ACTION_READ_RESERVATIONS	0x01
#define SERVICE_ACTION_READ_CAPABILITIES	0x02
#define SERVICE_ACTION_READ_FULL		0x03

#define SERVICE_ACTION_REGISTER			0x00
#define SERVICE_ACTION_RESERVE			0x01
#define SERVICE_ACTION_RELEASE			0x02
#define SERVICE_ACTION_CLEAR			0x03
#define SERVICE_ACTION_PREEMPT			0x04
#define SERVICE_ACTION_PREEMPT_ABORT		0x05
#define SERVICE_ACTION_REGISTER_IGNORE		0x06

#define RESERVATION_TYPE_READ_SHARED		0x00
#define RESERVATION_TYPE_WRITE_EXCLUSIVE	0x01
#define RESERVATION_TYPE_READ_EXCLUSIVE		0x02
#define RESERVATION_TYPE_EXCLUSIVE_ACCESS	0x03
#define RESERVATION_TYPE_SHARED_ACCESS		0x04
#define RESERVATION_TYPE_WRITE_EXCLUSIVE_RO	0x05
#define RESERVATION_TYPE_EXCLUSIVE_ACCESS_RO	0x06
#define RESERVATION_TYPE_WRITE_EXCLUSIVE_AR	0x07
#define RESERVATION_TYPE_EXCLUSIVE_ACCESS_AR	0x08

enum {
	RESERVATION_TYPE_RESERVE = 0x01,
	RESERVATION_TYPE_PERSISTENT = 0x02,
};

#define READ_BIT(a, bit) ((a >> bit) & 0x1)
#define READ_24(a, b, c) (((a) << 16) | ((b) << 8) | (c))
#define READ_NIBBLE_LOW(a) ((a) & 0xF)
#define READ_NIBBLE_HIGH(a) (((a >> 4) & 0xF))
#include "gdevq.h"

static inline void
device_send_notify(struct qsio_immed_notify *notify)
{
	notify->ccb_h.flags = QSIO_SEND_STATUS | QSIO_TYPE_NOTIFY;
	(*notify->ccb_h.queue_fn)(notify);
	return;
}

#define PRIME 0x9e37fffffffc0001UL

static inline unsigned long
hashblock(unsigned long val, int bits, uint32_t sector_shift)
{
	unsigned int mod = (1U << bits) - 1;

	if (sector_shift != LBA_SHIFT && !(val & 0x7))
		val >>= 3;
	return (val % mod);
}

static inline void device_set_vhba_id(struct tdisk *tdisk, int tid, int interface)
{
	if (interface == TARGET_INT_ISCSI)
		tdisk->iscsi_tid = tid;
	else if (interface == TARGET_INT_LOCAL)
		tdisk->vhba_id = tid;
}

static inline void device_set_hpriv(struct tdisk *tdisk, void *hpriv)
{
	tdisk->hpriv = hpriv;
}

static inline uint32_t
device_lba_shift(struct tdisk *tdisk)
{
	return tdisk->lba_shift;
}

static inline uint64_t
device_end_lba(struct tdisk *tdisk)
{
	return tdisk->end_lba;
}

#define __device_tid(bs)	((bs + 1))

static inline uint16_t
__get_lun(uint16_t bus)
{
	uint16_t lun = 0;

	lun = __device_tid(bus);
	return lun;
}

static inline void
__write_lun(uint32_t bus, uint8_t *ptr)
{
	uint64_t lun;
	lun = __get_lun(bus);
	ptr[1] = (lun & 0xFF);
	if (lun < 256)
		ptr[0] = 0;
	else
		ptr[0] = 0x40 | ((lun >> 8) & 0x3F);
	ptr[2] = ptr[3] = ptr[4] = ptr[5] = ptr[6] = ptr[7] = 0;
}

static inline uint32_t
bus_from_lun(uint16_t ilun)
{
	uint16_t lun = (ilun & 0x3FFF);
	uint32_t bus;

	bus = (lun - 1);
	return bus;
}

static inline uint32_t
device_tid(struct tdisk *tdisk)
{
	return __device_tid(tdisk->bus);
}

struct sense_info {
	struct qs_sense_data sense_data;
	SLIST_ENTRY(sense_info) s_list;
};

static inline int
iid_equal(uint64_t first_i[], uint64_t first_t[], int8_t first_f, uint64_t second_i[], uint64_t second_t[], int8_t second_f)
{
	if (port_equal(first_i, second_i) && port_equal(first_t, second_t) && first_f == second_f)
		return 1;
	else
		return 0;
}

#ifdef TAG_CMD_DEBUG
static inline void
debug_tag(struct qsio_scsiio *ctio)
{
	uint32_t id = (ticks % 3);

	switch (id) {
		case 0:
			id = MSG_HEAD_OF_QUEUE_TASK;
			break;
		case 1:
			id = MSG_ORDERED_TASK;
			break;
		default:
			id = MSG_SIMPLE_TASK;
			break;
	}
	ctio->task_attr = id;
}
#endif

static inline int 
istate_queue_cmd(struct initiator_state *istate, struct qsio_scsiio *ctio, int node_master)
{
	int exec = 0;

	mtx_lock(istate->istate_lock);
	while (!node_master && atomic16_read(&istate->blocked)) {
		mtx_unlock(istate->istate_lock);
		wait_on_chan(istate->istate_wait, !atomic16_read(&istate->blocked));
		mtx_lock(istate->istate_lock);
	}

	if (ctio->task_attr == MSG_SIMPLE_TASK) {
		if (!istate->ordered && !istate->head) {
			ctio->queued = 1; /* ACA would mean that this is blocked however */ 
			exec = 1;
		}
	}
	else if (ctio->task_attr == MSG_HEAD_OF_QUEUE_TASK) {
		ctio->queued = 1; /* ACA would mean that this is blocked however */ 
		istate->head++;
		exec = 1;
	}
	else {
		if (!istate->queued) {
			ctio->queued = 1; /* ACA would mean that this is blocked however */ 
			istate->ordered++;
			exec = 1;
		}
	}

	if (!exec)
		istate->pending++;
	else
		istate->queued++;
	TAILQ_INSERT_TAIL(&istate->queue_list, ctio, ta_list);
	mtx_unlock(istate->istate_lock);
	return exec;
}

static inline void
istate_abort_task_set(struct initiator_state *istate)
{
	struct qsio_scsiio *iter;

	mtx_lock(istate->istate_lock);
	if (TAILQ_EMPTY(&istate->queue_list)) {
		mtx_unlock(istate->istate_lock);
		return;
	}

	atomic16_set(&istate->blocked, 1);
	TAILQ_FOREACH(iter, &istate->queue_list, ta_list) {
		if (iter->ccb_h.flags & QSIO_IN_DEVQ)
			continue;
		iter->ccb_h.flags |= QSIO_CTIO_ABORTED;
	}
	mtx_unlock(istate->istate_lock);
}
 
static inline void
istate_abort_tasks(struct initiator_state *istate, uint64_t i_prt[], uint64_t t_prt[], uint8_t init_int, int block)
{
	struct qsio_scsiio *iter;

	mtx_lock(istate->istate_lock);
	if (TAILQ_EMPTY(&istate->queue_list)) {
		mtx_unlock(istate->istate_lock);
		return;
	}

	if (block)
		atomic16_set(&istate->blocked, 1);

	TAILQ_FOREACH(iter, &istate->queue_list, ta_list) {
		if (iter->ccb_h.flags & QSIO_IN_DEVQ)
			continue;
		iter->ccb_h.flags |= QSIO_CTIO_ABORTED;
		if (!port_equal(iter->i_prt, i_prt) || !port_equal(iter->t_prt, t_prt) || iter->init_int != init_int)
			iter->ccb_h.flags |= QSIO_SEND_ABORT_STATUS;
	}
	mtx_unlock(istate->istate_lock);
}

static inline int
istate_task_exists(struct initiator_state *istate, uint32_t task_tag)
{
	struct qsio_scsiio *iter;
	int task_found = 0;

	mtx_lock(istate->istate_lock);
	TAILQ_FOREACH(iter, &istate->queue_list, ta_list) {
		if (iter->task_tag != task_tag)
			continue;
		task_found = 1;
		break;
	}
	mtx_unlock(istate->istate_lock);
	return task_found;
}

static inline int
istate_abort_task(struct initiator_state *istate, uint64_t i_prt[], uint64_t t_prt[], uint8_t init_int, uint32_t task_tag, int *task_exists)
{
	struct qsio_scsiio *iter;
	int task_found = 0;

	debug_info("aborting task for tag %x\n", task_tag);
	mtx_lock(istate->istate_lock);
	if (TAILQ_EMPTY(&istate->queue_list)) {
		mtx_unlock(istate->istate_lock);
		return 0;
	}

	atomic16_set(&istate->blocked, 1);
	TAILQ_FOREACH(iter, &istate->queue_list, ta_list) {
		debug_info("iter task tag %x task tag %x in devq %d\n", iter->task_tag, task_tag, iter->ccb_h.flags & QSIO_IN_DEVQ);
		if (iter->task_tag != task_tag)
			continue;

		if (iter->ccb_h.flags & QSIO_IN_DEVQ) {
			*task_exists = 1;
			break;
		}

		debug_info("aborting task with tag %x\n", task_tag);
		iter->ccb_h.flags |= QSIO_CTIO_ABORTED;
		if (!port_equal(iter->i_prt, i_prt) || !port_equal(iter->t_prt, t_prt) || iter->init_int != init_int)
			iter->ccb_h.flags |= QSIO_SEND_ABORT_STATUS;
		task_found = 1;
		break;
	}
	mtx_unlock(istate->istate_lock);
	return task_found;
}
 
static inline void 
istate_remove_cmd(struct initiator_state *istate, struct qsio_scsiio *ctio, struct ccb_list *task_list)
{
	struct qsio_scsiio *iter;
	int done = 0;
	uint32_t ordered = 0, queued = 0;

	mtx_lock(istate->istate_lock);
	if (ctio->queued) {
		istate->queued--;
		switch (ctio->task_attr) {
			case MSG_HEAD_OF_QUEUE_TASK:
				istate->head--;
				break;
			case MSG_ORDERED_TASK:
			case MSG_ACA_TASK:
				istate->ordered--;
				break;
		}
	}

	if (!istate->pending || (TAILQ_FIRST(&istate->queue_list) != ctio)) { /* Head of queue */
		TAILQ_REMOVE(&istate->queue_list, ctio, ta_list);
		if (TAILQ_EMPTY(&istate->queue_list))
			chan_wakeup(istate->istate_wait);
		mtx_unlock(istate->istate_lock);
		return;
	}

	TAILQ_REMOVE(&istate->queue_list, ctio, ta_list);

	TAILQ_FOREACH(iter, &istate->queue_list, ta_list) {
		if (iter->queued) {
			queued++;
			if (iter->task_attr != MSG_SIMPLE_TASK)
				ordered++;
			continue;
		}

		switch (iter->task_attr) {
			case MSG_SIMPLE_TASK:
				if (ordered) {
					done = 1;
					break;
				}
				iter->queued = 1;
				istate->pending--;
				istate->queued++;
				queued++;
				STAILQ_INSERT_TAIL(task_list, (struct qsio_hdr *)iter, c_list);
				break;
			case MSG_HEAD_OF_QUEUE_TASK:
				done = 1;
				break;
			case MSG_ORDERED_TASK:
			case MSG_ACA_TASK:
			default:
				if (queued) {
					done = 1;
					break;
				}
				iter->queued = 1;
				istate->pending--;
				istate->queued++;
				istate->ordered++;
				ordered++;
				queued++;
				STAILQ_INSERT_TAIL(task_list, (struct qsio_hdr *)iter, c_list);
				break;
		}
		if (done)
			break;
	}
	mtx_unlock(istate->istate_lock);
}

int device_allocate_buffers(struct qsio_scsiio *ctio, uint32_t num_blocks, allocflags_t flags);
int device_allocate_buffers_nopage(struct qsio_scsiio *ctio, uint32_t num_blocks, allocflags_t flags);

int device_allocate_cmd_buffers(struct qsio_scsiio *ctio, allocflags_t flags);
int ctio_allocate_read_buffer(uint32_t drive_block_size, struct qsio_scsiio *ctio);
static inline void
ctio_allocate_buffer(struct qsio_scsiio *ctio, int length, allocflags_t flags)
{
	int new_length;
	int pad_mask = (ctio->init_int != TARGET_INT_ISCSI) ? 0 : 0x03;

	if (pad_mask && (length & pad_mask))
	{
		int pad = (pad_mask + 1) - (length & pad_mask);

		new_length = length + pad;
	}
	else
	{
		new_length = length;
	}
	ctio->data_ptr = malloc(new_length, M_CTIODATA, flags);
	ctio->dxfer_len = length;
	return;
}

static inline void
ctio_set_sense_info_valid(struct qsio_scsiio *ctio)
{
	struct qs_sense_data *sense;
	int sense_offset = (ctio->init_int != TARGET_INT_ISCSI) ? 0 : 2;

	sense = (struct qs_sense_data *)(ctio->sense_data+sense_offset);
	sense->error_code |= 0x80;
}

static inline void
print_buffer(char *buf, int len)
{
	int i;
	unsigned char c;
	for (i = 0; i < len; i++) {
		if (i && (i % 16) == 0)
			printf("\n");
		if (!i || (i % 16) == 0)
			printf("%02d: ", i);
		printf("%02x ", buf[i] & 0xFF);
	}
	printf("\n");

	for (i = 0; i < len; i++) {
		if (i && (i % 16) == 0)
			printf("\n");
		if (!i || (i % 16) == 0)
			printf("%02d: ", i);
		c = buf[i];
		if (c > 0x20 && c < 0x7B)
			printf("%c ", c);
		else
			printf(". ");
	}
	printf("\n");
}
static inline void
print_cdb(uint8_t *cdb)
{
	int i;

	if (!cdb[0])
		return;

	printf ("print_cdb: ");
	for (i = 0; i < 16; i++)
	{
		printf(" %x", cdb[i]);
	}
	printf ("\n");
}

extern uma_t *istate_cache;
static inline void
istate_free(struct initiator_state *istate)
{
	mtx_free(istate->istate_lock);
	wait_chan_free(istate->istate_wait);
	uma_zfree(istate_cache, istate);
}

static inline void
istate_put(struct initiator_state *istate)
{
	if (atomic_dec_and_test(&istate->refs))
		istate_free(istate);
}

static inline void
istate_get(struct initiator_state *istate)
{
	atomic_inc(&istate->refs);
}

static inline void
ctio_free(struct qsio_scsiio *ctio)
{
	if (ctio->data_ptr)
		free(ctio->data_ptr, M_CTIODATA);
	if (ctio->istate)
		istate_put(ctio->istate);
	ctio_free_sense(ctio);
	uma_zfree(ctio_cache, ctio);
}

static inline void
init_istate(struct initiator_state *iter, uint64_t i_prt[], uint64_t t_prt[], uint16_t r_prt, uint8_t init_int)
{
	SLIST_INIT(&iter->sense_list);
	TAILQ_INIT(&iter->queue_list);
	iter->istate_lock = mtx_alloc("istate lock");
	iter->istate_wait = wait_chan_alloc("istate wait");
	port_fill(iter->i_prt, i_prt);
	port_fill(iter->t_prt, t_prt);
	iter->r_prt = r_prt;
	iter->init_int = init_int;
	atomic_set(&iter->refs, 1);
}

int fc_initiator_check(uint64_t wwpn[], void *device);

static inline struct initiator_state *
__device_get_initiator_state(struct tdisk *tdisk, uint64_t i_prt[], uint64_t t_prt[], uint16_t r_prt, uint8_t init_int, int alloc, int check)
{
	struct istate_list *istate_list = &tdisk->istate_list;
	struct initiator_state *iter, *prev = NULL;

	SLIST_FOREACH(iter, istate_list, i_list) {
		if (port_equal(iter->i_prt, i_prt) && port_equal(iter->t_prt, t_prt) && iter->init_int == init_int) {
			if (iter->disallowed)
				return NULL;
			if (prev) {
				SLIST_REMOVE_AFTER(prev, i_list);
				SLIST_INSERT_HEAD(istate_list, iter, i_list);
			}
			iter->timestamp = ticks;
			istate_get(iter);
			return iter;
		}
		prev = iter;
	}

	if (!alloc)
		return NULL;

	if (check && init_int == TARGET_INT_FC) {
		int allow;

		allow = fc_initiator_check(i_prt, tdisk);
		if (allow != FC_RULE_ALLOW)
			return NULL;
	}

	iter = __uma_zalloc(istate_cache, Q_WAITOK | Q_ZERO, sizeof(*iter));
	init_istate(iter, i_prt, t_prt, r_prt, init_int);
	iter->timestamp = ticks; 
	SLIST_INSERT_HEAD(istate_list, iter, i_list);
	istate_get(iter);
	return iter;
}

static inline struct initiator_state *
device_get_initiator_state(struct tdisk *tdisk, uint64_t i_prt[], uint64_t t_prt[], uint16_t r_prt, uint8_t init_int, int alloc, int check)
{
	return __device_get_initiator_state(tdisk, i_prt, t_prt, r_prt, init_int, alloc, check);
}

static inline int
ctio_write_length(struct qsio_scsiio *ctio, struct tdisk *tdisk, uint32_t *num_blocks, int *dxfer_len)
{
	uint8_t *cdb = ctio->cdb;
	uint32_t transfer_length;
	int pglist_cnt;
	int lba_shift = tdisk->lba_shift;

	switch (cdb[0])
	{
		case WRITE_6:
		case READ_6:
			transfer_length = cdb[4];
			/* snm limit this to block limits VPD */
			if (!transfer_length)
				transfer_length = 256;
			pglist_cnt = transfer_length_to_pglist_cnt(lba_shift, transfer_length);
			break;
		case WRITE_10:
		case READ_10:
			transfer_length = be16toh(*(uint16_t *)(&cdb[7]));
			pglist_cnt = transfer_length_to_pglist_cnt(lba_shift, transfer_length);
			break;
		case WRITE_12:
		case READ_12:
			transfer_length = be32toh(*(uint32_t *)(&cdb[6]));
			pglist_cnt = transfer_length_to_pglist_cnt(lba_shift, transfer_length);
			break;
		case WRITE_16:
		case READ_16:
			transfer_length = be32toh(*(uint32_t *)(&cdb[10]));
			pglist_cnt = transfer_length_to_pglist_cnt(lba_shift, transfer_length);
			break;
		case WRITE_SAME:
		case WRITE_SAME_16:
			transfer_length = 1;
			pglist_cnt = 1;
			break;
		case EXTENDED_COPY:
			transfer_length = be32toh(*(uint32_t *)(&cdb[10]));
			pglist_cnt = (transfer_length >> LBA_SHIFT);
			if (transfer_length & LBA_MASK)
				pglist_cnt++;
			lba_shift = 0;
			break;
		case COMPARE_AND_WRITE:
			transfer_length = cdb[13];
			transfer_length <<= 1;
			pglist_cnt = transfer_length_to_pglist_cnt(lba_shift, transfer_length);
			break;
		default:
			*dxfer_len = 0;
			return -1;
	}

	*dxfer_len = (transfer_length << lba_shift);
	*num_blocks = pglist_cnt;
	return 0;
}

static inline struct sense_info *
device_get_sense(struct initiator_state *istate)
{
	struct sense_info *sinfo;

	if (SLIST_EMPTY(&istate->sense_list))
		return NULL;

	mtx_lock(istate->istate_lock);
	sinfo = SLIST_FIRST(&istate->sense_list);
	if (sinfo)
		SLIST_REMOVE_HEAD(&istate->sense_list, s_list);
	mtx_unlock(istate->istate_lock);
	return sinfo;
}

static inline int
device_has_sense(struct initiator_state *istate)
{
	return (!SLIST_EMPTY(&istate->sense_list));
}

static inline void
device_move_sense(struct qsio_scsiio *ctio, struct sense_info *sinfo)
{
	struct qs_sense_data *sense;
	int sense_offset = (ctio->init_int != TARGET_INT_ISCSI) ? 0 : 2;

	sense = &sinfo->sense_data;
	ctio_allocate_sense(ctio, SENSE_LEN(sense));
	memcpy(ctio->sense_data+sense_offset, sense, ctio->sense_len);
	free(sinfo, M_SENSEINFO);
	return;
}

static inline void
device_unblock_queues(struct tdisk *tdisk)
{
	struct initiator_state *iter;
	int retry = 1;

	while (retry) {
		retry = 0;
		tdisk_reservation_lock(tdisk);
		SLIST_FOREACH(iter, &tdisk->istate_list, i_list) {
			if (!atomic16_read(&iter->blocked))
				continue;

			if (!TAILQ_EMPTY(&iter->queue_list)) {
				retry = 1;
				continue;
			}
			atomic16_set(&iter->blocked, 0);
			chan_wakeup(iter->istate_wait);
		}
		tdisk_reservation_unlock(tdisk);
		if (retry)
			pause("psg", 300);
	}
}

static inline void
istate_sense_list_free(struct initiator_state *istate)
{
	struct sense_info *sinfo;

	while ((sinfo = SLIST_FIRST(&istate->sense_list)) != NULL) {
		SLIST_REMOVE_HEAD(&istate->sense_list, s_list);
		free(sinfo, M_SENSEINFO);
	}
}

static inline void
free_initiator_state(struct initiator_state *istate)
{
	wait_on_chan(istate->istate_wait, TAILQ_EMPTY(&istate->queue_list));
	istate_sense_list_free(istate);
	istate_put(istate);
}

static inline void
device_free_initiator_state2(struct tdisk *tdisk, uint64_t i_prt[], uint64_t t_prt[], uint8_t init_int)
{
	struct initiator_state *iter, *prev = NULL;

	tdisk_reservation_lock(tdisk);
	SLIST_FOREACH(iter, &tdisk->istate_list, i_list) {
		if (port_equal(iter->i_prt, i_prt) && port_equal(iter->t_prt, t_prt) && iter->init_int == init_int) {
			if (prev)
				SLIST_REMOVE_AFTER(prev, i_list);
			else
				SLIST_REMOVE_HEAD(&tdisk->istate_list, i_list);
			break;
		}
		prev = iter;
	}
	tdisk_reservation_unlock(tdisk);

	if (!iter)
		return;

	free_initiator_state(iter);
}

static inline void
device_remove_ctio(struct qsio_scsiio *ctio, struct ccb_list *ctio_list)
{
	struct initiator_state *istate;

	if (!ctio->ta_list.tqe_next && !ctio->ta_list.tqe_prev)
		return;

	istate = ctio->istate;
	if (!istate)
		return;

	istate_remove_cmd(istate, ctio, ctio_list);
}

struct qs_interface_cbs *device_interface_locate(int interface);

static inline void
device_queue_ctio_list(struct ccb_list *ctio_list)
{
	struct qsio_hdr *ccb_h;
	struct qs_interface_cbs *fc_icbs = NULL;
	struct qsio_scsiio *ctio;

	while ((ccb_h = STAILQ_FIRST(ctio_list)) != NULL) {
		STAILQ_REMOVE_HEAD(ctio_list, c_list);
		ctio = (struct qsio_scsiio *)ccb_h;
		if (ctio->init_int == TARGET_INT_FC) {
			if (!fc_icbs)
				fc_icbs = device_interface_locate(TARGET_INT_FC);
			debug_check(!fc_icbs);
			(*fc_icbs->ctio_exec)(ctio);
		}
		else {
			gdevq_insert_ccb(ccb_h);
		}
	}
}

int device_istate_abort_task(struct tdisk *tdisk, uint64_t i_prt[], uint64_t t_prt[], int init_int, uint32_t task_tag);
void device_istate_abort_task_set(struct tdisk *tdisk, uint64_t i_prt[], uint64_t t_prt[], int init_int);

static inline int
__device_istate_queue_ctio(struct tdisk *tdisk, struct qsio_scsiio *ctio, int node_master)
{
	struct initiator_state *istate;
	int exec;

	tdisk_reservation_lock(tdisk);
	istate = device_get_initiator_state(tdisk, ctio->i_prt, ctio->t_prt, ctio->r_prt, ctio->init_int, 1, 1);
	tdisk_reservation_unlock(tdisk);
	if (!istate)
		return -1;
	ctio->istate = istate;
	exec = istate_queue_cmd(istate, ctio, node_master);
	return exec;
}

static inline int
device_istate_queue_ctio(struct tdisk *tdisk, struct qsio_scsiio *ctio)
{
	return __device_istate_queue_ctio(tdisk, ctio, 0);
}

static inline int 
device_queue_ctio(struct tdisk *tdisk, struct qsio_scsiio *ctio)
{
	struct initiator_state *istate;
	int exec;

	tdisk_reservation_lock(tdisk);
	istate = device_get_initiator_state(tdisk, ctio->i_prt, ctio->t_prt, ctio->r_prt, ctio->init_int, 1, 1);
	tdisk_reservation_unlock(tdisk);
	if (!istate)
		return -1;

	ctio->istate = istate;
	exec = istate_queue_cmd(istate, ctio, 0);
	if (exec)
		gdevq_insert_ccb((struct qsio_hdr *)ctio);
	return exec;
}

struct reservation;
void device_add_sense(struct initiator_state *istate, uint8_t error_code, uint8_t sense_key, uint32_t info, uint8_t asc, uint8_t ascq);
int device_find_sense(struct initiator_state *istate, uint8_t sense_key, uint8_t asc, uint8_t ascq);
int device_request_sense(struct qsio_scsiio *ctio, struct initiator_state *istate);
void device_unit_attention(struct tdisk *tdisk, int all, uint64_t i_prt[], uint64_t t_prt[], uint8_t init_int, uint8_t asc, uint8_t ascq, int ignore_dup);
void device_free_all_initiators(struct istate_list *lhead);
void device_wait_all_initiators(struct istate_list *lhead);
void device_free_stale_initiators(struct istate_list *lhead);
struct logical_unit_naa_identifier;
void device_init_naa_identifier(struct logical_unit_naa_identifier *naa_identifier, char *serial_number);
struct logical_unit_identifier;
void device_init_unit_identifier(struct logical_unit_identifier *unit_identifier, char *vendor_id, char *product_id, int serial_len);
void device_target_reset(struct tdisk *tdisk, uint64_t i_prt[], uint64_t t_prt[], uint8_t init_int);
void device_free_initiator(uint64_t i_prt[], uint64_t t_prt[], int init_int, struct tdisk *tdisk);
void cbs_remove_device(struct tdisk *tdisk);
void cbs_disable_device(struct tdisk *tdisk);
void cbs_new_device(struct tdisk *tdisk, int notify_usr);
void cbs_update_device(struct tdisk *tdisk);
int get_next_device_id(void);
struct tdisk *get_device(uint32_t bus);
int target_add_fc_rule(struct fc_rule_config *spec);
int target_remove_fc_rule(struct fc_rule_config *spec);
void target_clear_fc_rules(uint32_t target_id);

struct fc_rule {
	uint64_t wwpn[2];
	uint32_t target_id;
	int rule;
	TAILQ_ENTRY(fc_rule) r_list;
};
TAILQ_HEAD(fc_rule_list, fc_rule);

#endif
