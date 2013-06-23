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

#include "coredefs.h"
#include "qs_lib.h"
#include "bdevmgr.h"
#include "cluster.h" 
#include "node_sync.h"
#include "node_ha.h"

#ifdef FREEBSD 
static void bio_meta_end_bio(bio_t *bio)
#else
static void bio_meta_end_bio(bio_t *bio, int err)
#endif
{
	struct bio_meta *bio_meta = (struct bio_meta *)bio_get_caller(bio);
#ifdef FREEBSD
	int err = bio->bio_error;
#endif

	if (unlikely(err))
	{
		atomic_set_bit(BIO_META_ERROR, &bio_meta->flags);
	}

	wait_complete(bio_meta->completion);
	bio_free_page(bio);
	g_destroy_bio(bio);
}

uint64_t bio_reads;
uint64_t bio_read_size;
uint64_t bio_writes;
uint64_t bio_write_size;
uint32_t index_lookup_writes;
uint32_t index_writes;
uint32_t bint_writes;
uint32_t amap_table_writes;
uint32_t amap_writes;
uint32_t ddlookup_writes;
uint32_t ddtable_writes;
uint32_t log_writes;
uint32_t tdisk_index_writes;
uint32_t ha_writes;
uint32_t index_lookup_reads;
uint32_t index_reads;
uint32_t bint_reads;
uint32_t amap_table_reads;
uint32_t amap_reads;
uint32_t ddlookup_reads;
uint32_t ddtable_reads;
uint32_t log_reads;
uint32_t tdisk_index_reads;
uint32_t ha_reads;

int
qs_lib_bio_lba(struct bdevint *bint, uint64_t b_start, pagestruct_t *page, int rw, int type)
{
	struct bio_meta bio_meta;
	struct tpriv priv = { 0 };
	int retval;

	bio_meta_init(&bio_meta);
	bdev_marker(bint->b_dev, &priv);
	retval = qs_lib_bio_page(bint, b_start, LBA_SIZE, page, NULL, &bio_meta, rw, type);
	bdev_start(bint->b_dev, &priv);
	if (retval == 0) {
		wait_for_bio_meta(&bio_meta);
		if (atomic_test_bit(BIO_META_ERROR, &bio_meta.flags))
			retval = -1;
	}
	bio_meta_destroy(&bio_meta);
	return retval;
}

int 
qs_lib_bio_page(struct bdevint *bint, uint64_t b_start, uint32_t size, pagestruct_t *page, void *end_io, void *priv, int rw, int type)
{
	struct bio *bio;
#ifndef FREEBSD 
	int retval;
#endif

	debug_check(node_in_standby() && !node_in_transition() && rw == QS_IO_WRITE);
#ifdef ENABLE_STATS
	if (rw == QS_IO_WRITE) {
		GLOB_INC(bio_writes, 1);
		GLOB_INC(bio_write_size, size);
		switch (type) {
			case TYPE_INDEX_LOOKUP:
				GLOB_INC(index_lookup_writes, 1);
				break;
			case TYPE_INDEX:
				GLOB_INC(index_writes, 1);
				break;
			case TYPE_BINT:
				GLOB_INC(bint_writes, 1);
				break;
			case TYPE_AMAP_TABLE:
				GLOB_INC(amap_table_writes, 1);
				break;
			case TYPE_AMAP:
				GLOB_INC(amap_writes, 1);
				break;
			case TYPE_DDLOOKUP:
				GLOB_INC(ddlookup_writes, 1);
				break;
			case TYPE_DDTABLE:
				GLOB_INC(ddtable_writes, 1);
				break;
			case TYPE_LOG:
				GLOB_INC(log_writes, 1);
				break;
			case TYPE_TDISK_INDEX:
				GLOB_INC(tdisk_index_writes, 1);
				break;
			case TYPE_HA_INDEX:
				GLOB_INC(ha_writes, 1);
				break;
		}
	}
	else {
		GLOB_INC(bio_reads, 1);
		GLOB_INC(bio_read_size, size);
		switch (type) {
			case TYPE_INDEX_LOOKUP:
				GLOB_INC(index_lookup_reads, 1);
				break;
			case TYPE_INDEX:
				GLOB_INC(index_reads, 1);
				break;
			case TYPE_BINT:
				GLOB_INC(bint_reads, 1);
				break;
			case TYPE_AMAP_TABLE:
				GLOB_INC(amap_table_reads, 1);
				break;
			case TYPE_AMAP:
				GLOB_INC(amap_reads, 1);
				break;
			case TYPE_DDLOOKUP:
				GLOB_INC(ddlookup_reads, 1);
				break;
			case TYPE_DDTABLE:
				GLOB_INC(ddtable_reads, 1);
				break;
			case TYPE_LOG:
				GLOB_INC(log_reads, 1);
				break;
			case TYPE_TDISK_INDEX:
				GLOB_INC(tdisk_index_reads, 1);
				break;
			case TYPE_HA_INDEX:
				GLOB_INC(ha_reads, 1);
				break;
		}
	}
#endif
	debug_check(!b_start);
	bio = bio_get_new(bint, end_io ? end_io : (void *)bio_meta_end_bio, priv, b_start, 1, rw);
	if (unlikely(!bio))
		return -1;

	vm_pg_ref(page);
#ifdef FREEBSD
	bio_add_page(bio, (caddr_t)vm_pg_address(page), size);
	bio->bio_caller2 = page;
	g_io_request(bio, bint->cp);
#else
	retval = bio_add_page(bio, page, size, 0);
	if (unlikely(retval != size)) {
		vm_pg_unref(page);
		g_destroy_bio(bio);
		return -1;
	}
	send_bio(bio);
#endif
	return 0;
}
