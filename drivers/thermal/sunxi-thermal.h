#ifndef _SUNXI_THERMAL_H
#define _SUNXI_THERMAL_H

#define ACTIVE_INTERVAL		(500)
#define IDLE_INTERVAL		(500)
#define MAX_TRIP_COUNT		(8)

/**
 * struct freq_clip_table
 * @freq_clip_max: maximum frequency allowed for this cooling state.
 * @temp_level: Temperature level at which the temperature clipping will
 *	happen.
 * @mask_val: cpumask of the allowed cpu's where the clipping will take place.
 *
 * This structure is required to be filled and passed to the
 * cpufreq_cooling_unregister function.
 */
struct freq_clip_table {
	unsigned int freq_clip_max;
	unsigned int freq_clip_min;
	unsigned int temp_level;
	const struct cpumask *mask_val;
};

struct	thermal_trip_point_conf {
	int trip_val[MAX_TRIP_COUNT];
	int trip_count;
};

struct	thermal_cooling_conf {
	struct freq_clip_table freq_data[MAX_TRIP_COUNT];
	int freq_clip_count;
};

struct thermal_sensor_conf {
	char name[THERMAL_NAME_LENGTH];
	int (*read_temperature)(int index);
	struct thermal_trip_point_conf *trip_data;
	struct thermal_cooling_conf *cooling_data;
	void *private_data;
	int trend;
};
#define TREND_SAMPLE_MAX    8
struct sunxi_thermal_zone_trend_info
{
    unsigned short temp100[TREND_SAMPLE_MAX];
    unsigned int cnt;
    unsigned int index;
    int a0;
    int a1;
    int trend;
};

struct sunxi_thermal_zone {
	int id;
	char name[THERMAL_NAME_LENGTH];
	struct thermal_sensor_conf *sunxi_ths_sensor_conf;
	enum thermal_device_mode mode;
	struct thermal_zone_device *therm_dev;
	bool bind;
    struct sunxi_thermal_zone_trend_info* ptrend;
};

extern int sunxi_ths_register_thermal(struct sunxi_thermal_zone *ths_zone);
extern void sunxi_ths_unregister_thermal(struct sunxi_thermal_zone *ths_zone);
#ifdef CONFIG_SUNXI_THERMAL_DYNAMIC
extern int sunxi_ths_register_thermal_dynamic(struct sunxi_thermal_zone *ths_zone);
extern void sunxi_ths_unregister_thermal_dynamic(struct sunxi_thermal_zone *ths_zone);
extern int sunxi_thermal_register_thermalbind(struct sunxi_thermal_zone *ths_zone);
extern int sunxi_thermal_unregister_thermalbind(struct sunxi_thermal_zone *ths_zone);
#endif

#endif /* _SUNXI_THERMAL_H */
