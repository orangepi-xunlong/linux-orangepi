/* SPDX-License-Identifier: GPL-2.0 */
/*
 * OpenFirmware regulator support routines
 *
 */
struct regulator_desc;

struct fwnode_regulator_match {
	const char *name;
	void *driver_data;
	struct regulator_init_data *init_data;
	struct fwnode_handle *fwnode;
	const struct regulator_desc *desc;
};


extern struct regulator_init_data
	*fwnode_get_regulator_init_data(struct device *dev,
				    struct fwnode_handle *fwnode,
				    const struct regulator_desc *desc);
extern int fwnode_regulator_match(struct device *dev, struct fwnode_handle *fwnode,
			      struct fwnode_regulator_match *matches,
			      unsigned int num_matches);
