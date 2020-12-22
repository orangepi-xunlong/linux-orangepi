#ifndef  __edp_type__h
#define  __edp_type__h

#define EDP_RUINT8(addr)        (*((volatile unsigned char  *)(addr)))
#define EDP_WUINT8(addr,v)      (*((volatile unsigned char *)(addr)) = (unsigned char)(v))
#define EDP_RUINT16(addr)       (*((volatile unsigned short *)(addr)))
#define EDP_WUINT16(addr,v)     (*((volatile unsigned short *)(addr)) = (unsigned short)(v))
#define EDP_RUINT32(addr)       (*((volatile unsigned long *)(addr)))
#define EDP_WUINT32(addr,v)     (*((volatile unsigned long *)(addr)) = (unsigned long)(v))

#define EDP_SBIT8(addr, v)      (*((volatile unsigned char *)(addr)) |=  (unsigned char)(v))
#define EDP_CBIT8(addr, v)      (*((volatile unsigned char *)(addr)) &= ~(unsigned char)(v))
#define EDP_SBIT16(addr, v)     (*((volatile unsigned short *)(addr)) |=  (unsigned short)(v))
#define EDP_CBIT16(addr, v)     (*((volatile unsigned short *)(addr)) &= ~(unsigned short)(v))
#define EDP_SBIT32(addr, v)     (*((volatile unsigned long *)(addr)) |=  (unsigned long)(v))
#define EDP_CBIT32(addr, v)     (*((volatile unsigned long *)(addr)) &= (~(unsigned long)(v)))

/*****************************************************/
/*    Bit Position Definition                        */
/*****************************************************/
#define BIT0       0x00000001
#define BIT1       0x00000002
#define BIT2       0x00000004
#define BIT3       0x00000008
#define BIT4       0x00000010
#define BIT5       0x00000020
#define BIT6       0x00000040
#define BIT7       0x00000080
#define BIT8       0x00000100
#define BIT9       0x00000200
#define BIT10      0x00000400
#define BIT11      0x00000800
#define BIT12      0x00001000
#define BIT13      0x00002000
#define BIT14      0x00004000
#define BIT15      0x00008000
#define BIT16      0x00010000
#define BIT17      0x00020000
#define BIT18      0x00040000
#define BIT19      0x00080000
#define BIT20      0x00100000
#define BIT21      0x00200000
#define BIT22      0x00400000
#define BIT23      0x00800000
#define BIT24      0x01000000
#define BIT25      0x02000000
#define BIT26      0x04000000
#define BIT27      0x08000000
#define BIT28      0x10000000
#define BIT29      0x20000000
#define BIT30      0x40000000
#define BIT31      0x80000000

struct video_timming
{
	unsigned int pclk;
	unsigned int x;
	unsigned int y;
	unsigned int ht;
	unsigned int hbp;
	unsigned int hpsw;
	unsigned int vt;
	unsigned int vbp;
	unsigned int vpsw;
	unsigned int fps;
};

struct sink_info
{
	unsigned int dp_rev;
	unsigned int dp_enhanced_frame_cap;
	unsigned int dp_max_link_rate;
	unsigned int dp_max_lane_count;
	unsigned int eDP_capable;
};

struct training_info
{
	unsigned int swing_lv;
	unsigned int preemp_lv;
	unsigned int postcur2_lv;
};

enum edp_int
{
	LINE0 = BIT31,
	LINE1 = BIT30,
	FIFO_EMPTY = BIT29,
};

struct edp_para
{
	unsigned int lane_count;
	unsigned int start_delay;
	unsigned int bit_rate;
	unsigned int swing_level;
};

typedef void (*edp_print_string)(const char *c);
typedef void (*edp_print_value)(unsigned int val);

extern int edp_set_base_address(unsigned int address);
extern int edp_set_print_str_func(edp_print_string print);
extern int edp_set_print_val_func(edp_print_value print);
extern int edp_enable(struct edp_para *para, struct video_timming *tmg);
extern void edp_disable(void);
extern void edp_set_start_delay(unsigned int delay);
extern unsigned int edp_get_start_delay(void);
extern void edp_int_enable(enum edp_int intterupt);
extern void edp_int_disable(enum edp_int intterupt);
extern unsigned int edp_get_int_status(enum edp_int intterupt);
extern void edp_clr_int_status(enum edp_int intterupt);

#endif     //  ifndef __edp_type__h
