/* SPDX-License-Identifier: GPL-2.0+ */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM tsn

#if !defined(_TRACE_TSN_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_TSN_H

#include <sunxi-tsn.h>
#include <linux/tracepoint.h>

#include <linux/if_ether.h>
#include <linux/if_vlan.h>
/* #include <linux/skbuff.h> */
DECLARE_EVENT_CLASS(tsn_buffer_template,

	TP_PROTO(struct tsn_link *link,
		size_t bytes),

	TP_ARGS(link, bytes),

	TP_STRUCT__entry(
		__field(u64, stream_id)
		__field(size_t, size)
		__field(size_t, bsize)
		__field(void *, buffer)
		__field(void *, head)
		__field(void *, tail)
		__field(void *, end)
		),

	TP_fast_assign(
		__entry->stream_id = link->stream_id;
		__entry->size = bytes;
		__entry->bsize = link->used_buffer_size;
		__entry->buffer = link->buffer;
		__entry->head = link->head;
		__entry->tail = link->tail;
		__entry->end = link->end;
		),

	TP_printk("stream_id=%llu, copy=%zd, buffer: %zd, avail=%zd, [buffer=%p, head=%p, tail=%p, end=%p]",
		__entry->stream_id, __entry->size, __entry->bsize,
		(__entry->head - __entry->tail) % __entry->bsize,
		__entry->buffer,    __entry->head, __entry->tail, __entry->end)
);

DEFINE_EVENT(tsn_buffer_template, tsn_buffer_write,
	TP_PROTO(struct tsn_link *link, size_t bytes),
	TP_ARGS(link, bytes)
);

DEFINE_EVENT(tsn_buffer_template, tsn_buffer_write_net,
	TP_PROTO(struct tsn_link *link, size_t bytes),
	TP_ARGS(link, bytes)
);

DEFINE_EVENT(tsn_buffer_template, tsn_buffer_read,
	TP_PROTO(struct tsn_link *link, size_t bytes),
	TP_ARGS(link, bytes)
);

DEFINE_EVENT(tsn_buffer_template, tsn_buffer_read_net,
	TP_PROTO(struct tsn_link *link, size_t bytes),
	TP_ARGS(link, bytes)
);


DECLARE_EVENT_CLASS(tsn_buffer_update,

	TP_PROTO(struct tsn_link *link,
		size_t reported_avail),

	TP_ARGS(link, reported_avail),

	TP_STRUCT__entry(
		__field(u64, stream_id)
		__field(size_t, bsize)
		__field(void *, head)
		__field(void *, tail)
		__field(size_t, reported_left)
		__field(size_t, low_water)
		),

	TP_fast_assign(
		__entry->stream_id = link->stream_id;
		__entry->bsize = link->used_buffer_size;
		__entry->head = link->head;
		__entry->tail = link->tail;
		__entry->reported_left = reported_avail;
		__entry->low_water = link->low_water_mark;
		),

	TP_printk("stream_id=%llu, buffer_size=%zd, avail=%zd, reported=%zd, low_water=%zd",
		__entry->stream_id, __entry->bsize,
		(__entry->head - __entry->tail) % __entry->bsize,
		__entry->reported_left, __entry->low_water)
);

/* Bytes will be "reported left", i.e. how much more space we have in
 * the buffer before we wrap.
 */
DEFINE_EVENT(tsn_buffer_update, tsn_refill,
	TP_PROTO(struct tsn_link *link, size_t bytes),
	TP_ARGS(link, bytes)
);

DEFINE_EVENT(tsn_buffer_update, tsn_buffer_drain,
	TP_PROTO(struct tsn_link *link, size_t bytes),
	TP_ARGS(link, bytes)
);

TRACE_EVENT(tsn_send_batch,

	TP_PROTO(struct tsn_link *link,
		int num_send,
		u64 ts_base_ns,
		u64 ts_delta_ns),

	TP_ARGS(link, num_send, ts_base_ns, ts_delta_ns),

	TP_STRUCT__entry(
		__field(u64, stream_id)
		__field(int, seqnr)
		__field(int, num_send)
		__field(u64, ts_base_ns)
		__field(u64, ts_delta_ns)
		),

	TP_fast_assign(
		__entry->stream_id   = link->stream_id;
		__entry->seqnr	     = (int)link->last_seqnr;
		__entry->ts_base_ns  = ts_base_ns;
		__entry->ts_delta_ns = ts_delta_ns;
		__entry->num_send    = num_send;
		),

	TP_printk("stream_id=%llu, seqnr=%d, num_send=%d, ts_base_ns=%llu, ts_delta_ns=%llu",
		__entry->stream_id, __entry->seqnr, __entry->num_send, __entry->ts_base_ns, __entry->ts_delta_ns)
);

TRACE_EVENT(tsn_rx_handler,

	TP_PROTO(struct tsn_link *link,
		const struct ethhdr *ethhdr,
		u64 sid),

	TP_ARGS(link, ethhdr, sid),

	TP_STRUCT__entry(
		__field(char *, name)
		__field(u16, proto)
		__field(u64, sid)
		__field(u64, link_sid)
		),
	TP_fast_assign(
		__entry->name  = link->nic->name;
		__entry->proto = ethhdr->h_proto;
		__entry->sid   = sid;
		__entry->link_sid = link->stream_id;
		),

	TP_printk("name=%s, proto: 0x%04x, stream_id=%llu, link->sid=%llu",
		__entry->name, ntohs(__entry->proto), __entry->sid, __entry->link_sid)
);

TRACE_EVENT(tsn_du,

	TP_PROTO(struct tsn_link *link,
		size_t bytes),

	TP_ARGS(link, bytes),

	TP_STRUCT__entry(
		__field(u64, link_sid)
		__field(size_t, bytes)
		),
	TP_fast_assign(
		__entry->link_sid = link->stream_id;
		__entry->bytes = bytes;
		),

	TP_printk("stream_id=%llu,bytes=%zu",
		__entry->link_sid, __entry->bytes)
);

DECLARE_EVENT_CLASS(tsn_buffer_mgmt_class,

	TP_PROTO(struct tsn_link *link, size_t size),

	TP_ARGS(link, size),


	TP_STRUCT__entry(
		__field(u64,  stream_id)
		__field(size_t, size)
		),

	TP_fast_assign(
		__entry->stream_id = link->stream_id;
		__entry->size = size;
		),

	TP_printk("stream_id=%llu,buffer_size=%zu",
		__entry->stream_id, __entry->size)

);

DEFINE_EVENT(tsn_buffer_mgmt_class, tsn_set_buffer,
	TP_PROTO(struct tsn_link *link, size_t size),
	TP_ARGS(link, size)
);

DEFINE_EVENT(tsn_buffer_mgmt_class, tsn_free_buffer,
	TP_PROTO(struct tsn_link *link, size_t size),
	TP_ARGS(link, size)
);


/* TODO: too long, need cleanup.
 */
TRACE_EVENT(tsn_pre_tx,

	TP_PROTO(struct tsn_link *link, struct sk_buff *skb, size_t bytes),

	TP_ARGS(link, skb, bytes),

	TP_STRUCT__entry(
		__field(u64, stream_id)
		__field(u32, vlan_tag)
		__field(size_t, bytes)
		__field(size_t, data_len)
		__field(unsigned int, headlen)
		__field(u16, protocol)
		__field(u16, prot_native)
		__field(int, tx_idx)
		__field(u16, mac_len)
		__field(u16, hdr_len)
		__field(u16, vlan_tci)
		__field(u16, mac_header)
		__field(unsigned int, tail)
		__field(unsigned int, end)
		__field(unsigned int, truesize)
		),

	TP_fast_assign(
		__entry->stream_id = link->stream_id;
		__entry->vlan_tag = (skb_vlan_tag_present(skb) ? skb_vlan_tag_get(skb) : 0);
		__entry->bytes = bytes;
		__entry->data_len = skb->data_len;
		__entry->headlen = skb_headlen(skb);
		__entry->protocol = vlan_get_protocol(skb);
		__entry->prot_native = skb->protocol;
		__entry->tx_idx = skb_get_queue_mapping(skb);

		__entry->mac_len = skb->mac_len;
		__entry->hdr_len = skb->hdr_len;
		__entry->vlan_tci = skb->vlan_tci;
		__entry->mac_header = skb->mac_header;
		__entry->tail = (unsigned int)skb->tail;
		__entry->end  = (unsigned int)skb->end;
		__entry->truesize = skb->truesize;
		),

	TP_printk("stream_id=%llu,vlan_tag=0x%04x,data_size=%zd,data_len=%zd,headlen=%u,proto=0x%04x (0x%04x),tx_idx=%d,mac_len=%u,hdr_len=%u,vlan_tci=0x%02x,mac_header=0x%02x,tail=%u,end=%u,truesize=%u",
		__entry->stream_id,
		__entry->vlan_tag,
		__entry->bytes,
		__entry->data_len,
		__entry->headlen,
		ntohs(__entry->protocol),
		ntohs(__entry->prot_native),
		__entry->tx_idx,
		__entry->mac_len,
		__entry->hdr_len,
		__entry->vlan_tci,
		__entry->mac_header,
		__entry->tail,
		__entry->end,
		__entry->truesize)
	);

TRACE_EVENT(tsn_post_tx_set,

	TP_PROTO(struct tsn_link *link, size_t sent),

	TP_ARGS(link, sent),

	TP_STRUCT__entry(
		__field(u64, stream_id)
		__field(size_t, sent)
		),

	TP_fast_assign(
		__entry->stream_id = link->stream_id;
		__entry->sent = sent;
		),

	TP_printk("stream_id=%llu,sent=%zu", __entry->stream_id, __entry->sent)

	);

TRACE_EVENT(tsn_update_net_time,

	TP_PROTO(struct tsn_link *link),

	TP_ARGS(link),

	TP_STRUCT__entry(
		__field(u64, stream_id)
		__field(u64, ts_ns)
		__field(u64, delta_ns)
		__field(u64, delta_avg_ns)
		__field(u64, sent_total)
		),

	TP_fast_assign(
		__entry->stream_id = link->stream_id;
		__entry->ts_ns = link->ts_net_ns;
		__entry->delta_ns = link->ts_delta_ns;
		__entry->delta_avg_ns = link->ts_exp_avg;
		__entry->sent_total = link->frames_sent;
		),

	TP_printk("stream_id=%llu,ts=%llu,ts_delta=%llu,ts_delta_avg=%llu,sent_total=%llu",
		__entry->stream_id,
		__entry->ts_ns,
		__entry->delta_ns,
		__entry->delta_avg_ns,
		__entry->sent_total)
	);

#endif	/* _TRACE_TSN_H || TRACE_HEADER_MULTI_READ */

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE sunxi-trace-tsn
#include <trace/define_trace.h>
