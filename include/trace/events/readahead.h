#undef TRACE_SYSTEM
#define TRACE_SYSTEM readahead

#if !defined(_TRACE_READAHEAD_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_READAHEAD_H

#include <linux/tracepoint.h>

TRACE_EVENT(do_open_exec,

	    TP_PROTO(struct inode *inode),

	    TP_ARGS(inode),

	    TP_STRUCT__entry(__field(dev_t, dev) __field(ino_t, ino)),

	    TP_fast_assign(__entry->dev = inode->i_sb->s_dev;
			   __entry->ino = inode->i_ino;),

	    TP_printk("%d %d %lu", MAJOR(__entry->dev), MINOR(__entry->dev),
		      (unsigned long)__entry->ino));

TRACE_EVENT(do_fs_read,

	    TP_PROTO(struct inode *inode, unsigned long pos, size_t count),

	    TP_ARGS(inode, pos, count),

	    TP_STRUCT__entry(__field(dev_t, dev) __field(ino_t, ino)
				 __field(unsigned long, pos)
				     __field(size_t, count)),

	    TP_fast_assign(__entry->dev = inode->i_sb->s_dev;
			   __entry->ino = inode->i_ino; __entry->pos = pos;
			   __entry->count = count;),

	    TP_printk("%d %d %lu %lu %zu", MAJOR(__entry->dev),
		      MINOR(__entry->dev), __entry->ino, __entry->pos,
		      __entry->count));

TRACE_EVENT(do_file_map,

	    TP_PROTO(struct inode *inode, unsigned long pageshift,
		     unsigned long pagesize),

	    TP_ARGS(inode, pageshift, pagesize),

	    TP_STRUCT__entry(__field(dev_t, dev) __field(ino_t, ino)
				 __field(unsigned long, pageshift)
				     __field(unsigned long, pagesize)),

	    TP_fast_assign(__entry->dev = inode->i_sb->s_dev;
			   __entry->ino = inode->i_ino;
			   __entry->pageshift = pageshift;
			   __entry->pagesize = pagesize;),

	    TP_printk("%d %d %lu %lu %lu", MAJOR(__entry->dev),
		      MINOR(__entry->dev), (unsigned long)__entry->ino,
		      __entry->pageshift, __entry->pagesize));

#endif
#include <trace/define_trace.h>
