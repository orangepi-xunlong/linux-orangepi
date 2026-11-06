#include <vi_defines.h>
#include <linux/math64.h>

// #define ISP_PERF_MEASURE
#ifdef ISP_PERF_MEASURE
#define ISP_MEASURE_FRM	100
#define STOUS		1000000

struct isp_perf_chk {
	struct timespec64 sof_time[ISP_MEASURE_FRM];
	struct timespec64 pre_fe_eof[ISP_MEASURE_FRM];
	struct timespec64 pre_be_eof[ISP_MEASURE_FRM];
	struct timespec64 post_trig[ISP_MEASURE_FRM];
	struct timespec64 post_eof[ISP_MEASURE_FRM];

	u8 sof_end;
	u8 pre_fe_end;
	u8 pre_be_end;
	u8 post_end;
};

static struct isp_perf_chk time_chk;
#endif //ISP_PERF_MEASURE

void vi_record_sof_perf(struct sop_vi_dev *vdev, u8 raw_num, u8 chn_num)
{
#ifdef ISP_PERF_MEASURE
	if (vdev->pre_fe_sof_cnt[raw_num][chn_num] < ISP_MEASURE_FRM) {
		struct timespec64 ts;

		ktime_get_real_ts64(&ts);

		time_chk.sof_time[vdev->pre_fe_sof_cnt[raw_num][chn_num]].tv_sec = ts.tv_sec;
		time_chk.sof_time[vdev->pre_fe_sof_cnt[raw_num][chn_num]].tv_nsec = ts.tv_nsec;

		if (vdev->pre_fe_sof_cnt[raw_num][chn_num] == ISP_MEASURE_FRM - 1)
			time_chk.sof_end = true;
	}
#endif
}

void vi_record_fe_perf(struct sop_vi_dev *vdev, u8 raw_num, u8 chn_num)
{
#ifdef ISP_PERF_MEASURE
	if (vdev->pre_fe_frm_num[raw_num][chn_num] < ISP_MEASURE_FRM) {
		struct timespec64 ts;

		ktime_get_real_ts64(&ts);

		time_chk.pre_fe_eof[vdev->pre_fe_frm_num[raw_num][chn_num]].tv_sec = ts.tv_sec;
		time_chk.pre_fe_eof[vdev->pre_fe_frm_num[raw_num][chn_num]].tv_nsec = ts.tv_nsec;

		if (vdev->pre_fe_frm_num[raw_num][chn_num] == ISP_MEASURE_FRM - 1)
			time_chk.pre_fe_end = true;
	}
#endif
}

void vi_record_be_perf(struct sop_vi_dev *vdev, u8 raw_num, u8 chn_num)
{
#ifdef ISP_PERF_MEASURE
	if (vdev->pre_be_frm_num[raw_num][chn_num] < ISP_MEASURE_FRM) {
		struct timespec64 ts;

		ktime_get_real_ts64(&ts);

		time_chk.pre_be_eof[vdev->pre_be_frm_num[raw_num][chn_num]].tv_sec = ts.tv_sec;
		time_chk.pre_be_eof[vdev->pre_be_frm_num[raw_num][chn_num]].tv_nsec = ts.tv_nsec;

		if (vdev->pre_be_frm_num[raw_num][chn_num] == ISP_MEASURE_FRM - 1)
			time_chk.pre_be_end = true;
	}
#endif
}

void vi_record_post_end(struct sop_vi_dev *vdev, u8 raw_num)
{
#ifdef ISP_PERF_MEASURE
	if (vdev->postraw_frame_number[raw_num] < ISP_MEASURE_FRM) {
		struct timespec64 ts;

		ktime_get_real_ts64(&ts);

		time_chk.post_eof[vdev->postraw_frame_number[raw_num]].tv_sec = ts.tv_sec;
		time_chk.post_eof[vdev->postraw_frame_number[raw_num]].tv_nsec = ts.tv_nsec;

		if (vdev->postraw_frame_number[raw_num] == ISP_MEASURE_FRM - 1)
			time_chk.post_end = true;
	}
#endif
}

void vi_record_post_trigger(struct sop_vi_dev *vdev, u8 raw_num)
{
#ifdef ISP_PERF_MEASURE
	if (vdev->postraw_frame_number[raw_num] < ISP_MEASURE_FRM) {
		struct timespec64 ts;

		ktime_get_real_ts64(&ts);

		time_chk.post_trig[vdev->postraw_frame_number[raw_num]].tv_sec = ts.tv_sec;
		time_chk.post_trig[vdev->postraw_frame_number[raw_num]].tv_nsec = ts.tv_nsec;
	}
#endif
}

void vi_perf_record_dump(void)
{
#ifdef ISP_PERF_MEASURE
	u64 time_0 = 0, time_1 = 0;
	u32 i = 0;
	u64 sof_first = 0, sof_last = 0;
	u64 fe_eof_first = 0, fe_eof_last = 0;
	u64 be_eof_first = 0, be_eof_last = 0;
	u64 post_eof_first = 0, post_eof_last = 0;
	u64 fe_duration = 0, post_duration = 0;
	u64 tmp = 0;

	if (!(time_chk.sof_end && time_chk.pre_fe_end && time_chk.pre_be_end && time_chk.post_end))
		return;

	tmp = time_chk.sof_time[0].tv_nsec;
	do_div(tmp, 1000);
	sof_first = (time_chk.sof_time[0].tv_sec * STOUS) + tmp;

	tmp = time_chk.sof_time[ISP_MEASURE_FRM - 1].tv_nsec;
	do_div(tmp, 1000);
	sof_last = (time_chk.sof_time[ISP_MEASURE_FRM - 1].tv_sec * STOUS) + tmp;

	tmp = time_chk.pre_fe_eof[0].tv_nsec;
	do_div(tmp, 1000);
	fe_eof_first = (time_chk.pre_fe_eof[0].tv_sec * STOUS) + tmp;

	tmp = time_chk.pre_fe_eof[ISP_MEASURE_FRM - 1].tv_nsec;
	do_div(tmp, 1000);
	fe_eof_last = (time_chk.pre_fe_eof[ISP_MEASURE_FRM - 1].tv_sec * STOUS) + tmp;

	tmp = time_chk.pre_be_eof[0].tv_nsec;
	do_div(tmp, 1000);
	be_eof_first = (time_chk.pre_be_eof[0].tv_sec * STOUS) + tmp;

	tmp = time_chk.pre_be_eof[ISP_MEASURE_FRM - 1].tv_nsec;
	do_div(tmp, 1000);
	be_eof_last = (time_chk.pre_be_eof[ISP_MEASURE_FRM - 1].tv_sec * STOUS) + tmp;

	tmp = time_chk.post_eof[1].tv_nsec;
	do_div(tmp, 1000);
	post_eof_first = (time_chk.post_eof[1].tv_sec * STOUS) + tmp;

	tmp = time_chk.post_eof[ISP_MEASURE_FRM - 1].tv_nsec;
	do_div(tmp, 1000);
	post_eof_last = (time_chk.post_eof[ISP_MEASURE_FRM - 1].tv_sec * STOUS) + tmp;

	for (i = 0; i < ISP_MEASURE_FRM - 1; i++) {
		tmp = time_chk.sof_time[i].tv_nsec;
		do_div(tmp, 1000);
		time_0 = (time_chk.sof_time[i].tv_sec * STOUS) + tmp;

		tmp = time_chk.sof_time[i + 1].tv_nsec;
		do_div(tmp, 1000);
		time_1 = (time_chk.sof_time[i + 1].tv_sec * STOUS) + tmp; //us

		vi_pr(VI_ERR, "SOF_diff=%llu\n", (time_1 - time_0));
	}

	for (i = 0; i < ISP_MEASURE_FRM - 1; i++) {
		tmp = time_chk.pre_fe_eof[i].tv_nsec;
		do_div(tmp, 1000);
		time_0 = (time_chk.pre_fe_eof[i].tv_sec * STOUS) + tmp;

		tmp = time_chk.pre_fe_eof[i + 1].tv_nsec;
		do_div(tmp, 1000);
		time_1 = (time_chk.pre_fe_eof[i + 1].tv_sec * STOUS) + tmp; //us

		vi_pr(VI_ERR, "Pre_fe_diff=%llu\n", (time_1 - time_0));
	}

	for (i = 0; i < ISP_MEASURE_FRM - 1; i++) {
		tmp = time_chk.pre_be_eof[i].tv_nsec;
		do_div(tmp, 1000);
		time_0 = (time_chk.pre_be_eof[i].tv_sec * STOUS) + tmp;

		tmp = time_chk.pre_be_eof[i + 1].tv_nsec;
		do_div(tmp, 1000);
		time_1 = (time_chk.pre_be_eof[i + 1].tv_sec * STOUS) + tmp; //us

		vi_pr(VI_ERR, "Pre_be_diff=%llu\n", (time_1 - time_0));
	}

	for (i = 1; i < ISP_MEASURE_FRM - 1; i++) {
		tmp = time_chk.sof_time[i].tv_nsec;
		do_div(tmp, 1000);
		time_0 = (time_chk.sof_time[i].tv_sec * STOUS) + tmp;

		tmp = time_chk.pre_fe_eof[i].tv_nsec;
		do_div(tmp, 1000);
		time_1 = (time_chk.pre_fe_eof[i].tv_sec * STOUS) + tmp;

		fe_duration += (time_1 - time_0);
		vi_pr(VI_ERR, "Pre_fe_duration=%llu\n", (time_1 - time_0));
	}

	for (i = 1; i < ISP_MEASURE_FRM - 1; i++) {
		tmp = time_chk.post_trig[i].tv_nsec;
		do_div(tmp, 1000);
		time_0 = (time_chk.post_trig[i].tv_sec * STOUS) + tmp;

		tmp = time_chk.post_eof[i].tv_nsec;
		do_div(tmp, 1000);
		time_1 = (time_chk.post_eof[i].tv_sec * STOUS) + tmp; //us

		post_duration += (time_1 - time_0);
		vi_pr(VI_ERR, "Post_duration=%llu\n", (time_1 - time_0));
	}

	sof_last = (sof_last - sof_first);
	do_div(sof_last, ISP_MEASURE_FRM);

	fe_eof_last = (fe_eof_last - fe_eof_first);
	do_div(fe_eof_last, ISP_MEASURE_FRM);

	be_eof_last = (be_eof_last - be_eof_first);
	do_div(be_eof_last, ISP_MEASURE_FRM);

	post_eof_last = (post_eof_last - post_eof_first);
	do_div(post_eof_last, ISP_MEASURE_FRM - 1);

	do_div(fe_duration, ISP_MEASURE_FRM - 1);
	do_div(post_duration, ISP_MEASURE_FRM - 1);

	vi_pr(VI_ERR, "AVG time(us): sof(%llu) fe_eof(%llu) be_eof(%llu) post_eof(%llu)\n",
			sof_last, fe_eof_last, be_eof_last, post_eof_last);
	vi_pr(VI_ERR, "AVG time(us): fe_duration(%llu) post_duration(%llu)\n",
			fe_duration, post_duration);

	time_chk.post_end = false;
	time_chk.pre_be_end = false;
	time_chk.pre_fe_end = false;
	time_chk.sof_end = false;
#endif
}
