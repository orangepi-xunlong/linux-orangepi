/*
 * SPDX-License-Identifier: GPL-2.0+
 *
 * Add CIX SKY1 SoC Version driver
 *
 */

#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/sys_soc.h>
#include <linux/bitfield.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <linux/nvmem-consumer.h>


#define REVISION_MASK			0x3F00
#define REVISION_SHIFT_BITS		0x8
/* Revision Information */
#define REVISION_A0			0x8
#define REVISION_A1			0x9
#define REVISION_B0			0x10
#define REVISION_B1			0x11
/* Revision code types */
static const char *revision_code_types[] = {
	"A0",
	"A1",
	"B0",
	"B1",
	"Unknown",
};

#define ROADMAP_MASK			0xFFF000000
#define ROADMAP_SHIFT_BITS		0x18
/* Roadmap Information*/
#define ROADMAP_A11			0x111
#define ROADMAP_A12			0x112
#define ROADMAP_B11			0x211
#define ROADMAP_B12			0x212
#define ROADMAP_C11			0x311
#define ROADMAP_C12			0x312
/* Roadmap code types */
static const char *roadmap_code_types[] = {
	"A11",
	"A12",
	"B11",
	"B12",
	"C11",
	"C12",
	"Unknown",
};

#define MODEL_MASK			0xFFFF000000000
#define MODEL_SHIFT_BITS		0x24
/* Model Information */
#define MODEL8180			0x3100
#define MODEL2100			0x2100
#define MODEL1100			0x1100
#define MODEL0100			0x0100
/* Model code types */
static const char *model_code_types[] = {
	"8180",
	"2100",
	"1100",
	"0100",
	"Unknown",
};

#define SEGMENT_MASK			0x1F0000000000000
#define SEGMENT_SHFIT_BITS		0x34
/* Segment Information */
#define SEGMENT_A			0x1
#define SEGMENT_P			0x2
#define SEGMENT_E			0x3
#define SEGMENT_F			0x4
#define SEGMENT_H			0x5
#define SEGMENT_G			0x6
#define SEGMENT_D			0x7
#define SEGMENT_S			0x8
#define SEGMENT_X			0x9
#define SEGMENT_W			0x10
#define SEGMENT_C			0x11
/* Segment code types */
static const char *segment_code_types[] = {
	"A",
	"P",
	"E",
	"F",
	"H",
	"G",
	"D",
	"S",
	"X",
	"W",
	"C",
	"Unknown",
};

/* Firmware Offset */
#define SE_FIRMWARE_OFFSET		0x0
#define PBL_FIRMWARE_OFFSET		0x80
#define ATF_FIRMWARE_OFFSET		0x100
#define PM_FIRMWARE_OFFSET		0x180
#define TEE_FIRMWARE_OFFSET		0x200
#define UEFI_FIRMWARE_OFFSET		0x280
#define EC_FIRMWARE_OFFSET		0x300
#define PD_FIRMWARE_OFFSET		0x400

/*Firmware Board ID Information */
#define BOARD_ID_OFFSET			0x380

/* PCB_SKU Information */
#define MERAK				0x0
#define MIZAR				0x1
#define MERAK_DAP			0x2
#define PHECDA_SLT			0x3
#define	PHECDA				0x4
#define MEGREZ				0x5
#define PHECDA_DAP			0x6

/* Memory type Inforamtion */
#define Samsung_LP5_315b_4G_Single_Rank		0x0
#define Hynix_LP4x_200b_4G_Dual_Rank		0x1
#define Samsung_LP5x_315b_8G_Dual_Rank		0x2
#define Samsung_LP5_315b_8G_Dual_Rank		0x3
#define Samsung_LP5x_441b_8G_Single_Rank_Old	0x4
#define Samsung_LP5_441b_4G_Single_Rank		0x5
#define Samsung_LP5x_441b_8G_Single_Rank_New	0x6

/* Boardid information */
#define Rev_A				0x0
#define Rev_B				0x1
#define Rev_C				0x2
#define Rev_D				0x3
#define BOARD_VERSION_MASK		0x300
#define BOARD_VERSION_SHIFT_BITS	0x8

#define MAX_BYTES			0x7f

struct sky1_firmware_version {
	const char *se_version;
	const char *pbl_version;
	const char *atf_version;
	const char *pm_version;
	const char *tee_version;
	const char *uefi_version;
	const char *ec_version;
	const char *pd_version;
};

struct sky1_board_info {
	const char *board_id;
	const char *pcb_sku_id;
	const char *pmic_id;
	const char *memory_type_id;
	const char *board_revision_id;
};

struct cix_opn_info {
	const char *opn;
};

static struct sky1_firmware_version *firmware_version;
static struct sky1_board_info *board_info;
static struct cix_opn_info *opn_info;

static void *cix_sky1_firmware_parse(const volatile void __iomem *base)
{
	unsigned int len = 0;
	unsigned char *buf, *tmp0, *tmp1;
	tmp0 = (char*)base;
	tmp1 = (char*)base;

	while (*tmp0++ && len <= MAX_BYTES)
		len++;

	if (len > 0 && len <= MAX_BYTES) {
		buf = kasprintf(GFP_KERNEL, "Version %s", tmp1);
	} else {
		buf = kasprintf(GFP_KERNEL, "firmware_version_parse unknown");
	}

	return buf;
}

static void *cix_sky1_pd_firmware_parse(const volatile void __iomem *base)
{
	unsigned char pd1_maj, pd1_min, pd2_maj, pd2_min;
	unsigned char *p_src, *buf;
	p_src = (char *)base;
	pd1_maj = *p_src++;
	pd1_min = *p_src++;
	pd2_maj = *p_src++;
	pd2_min = *p_src++;

	buf = kasprintf(GFP_KERNEL, "PD1 major: 0x%x minor: 0x%x; PD2 major: 0x%x minor: 0x%x", \
			pd1_maj, pd1_min, pd2_maj, pd2_min);
	return buf;
}

static int cix_sky1_board_id_parse(const volatile void __iomem *base, struct sky1_board_info *board_info)
{
	unsigned int board_id = 0;
	unsigned int pcb_sku_id = 0, memory_type_id = 0, board_revision_id = 0;

	board_id = readw(base);
	board_info->board_id = kasprintf(GFP_KERNEL, "Board_id: 0x%x", board_id);

	/* Parse pcb_sku_id */
	if (board_id & BIT(0))
		pcb_sku_id |= BIT(0);
	if (board_id & BIT(1))
		pcb_sku_id |= BIT(1);
	if (board_id & BIT(2))
		pcb_sku_id |= BIT(2);
	if (board_id & BIT(6))
		pcb_sku_id |= BIT(3);

	switch (pcb_sku_id) {
	case MERAK:
		board_info->pcb_sku_id = kasprintf(GFP_KERNEL, "PCB SKU: MERAK");
		board_info->pmic_id = kasprintf(GFP_KERNEL, "PMIC: MPS5475x3");
		break;
	case MIZAR:
		board_info->pcb_sku_id = kasprintf(GFP_KERNEL, "PCB SKU: MIZAR");
		board_info->pmic_id = kasprintf(GFP_KERNEL, "PMIC: MPS5475x3");
		break;
	case MERAK_DAP:
		board_info->pcb_sku_id = kasprintf(GFP_KERNEL, "PCB SKU: MERAK_DAP");
		board_info->pmic_id = kasprintf(GFP_KERNEL, "PMIC: MPS5475x3");
		break;
	case PHECDA_SLT:
		board_info->pcb_sku_id = kasprintf(GFP_KERNEL, "PCB SKU: PHECDA_SLT");
		board_info->pmic_id = kasprintf(GFP_KERNEL, "PMIC: MP2845x2+DRMOS");
		break;
	case PHECDA:
		board_info->pcb_sku_id = kasprintf(GFP_KERNEL, "PCB SKU: PHECDA");
		board_info->pmic_id = kasprintf(GFP_KERNEL, "PMIC: MP2845x2+DRMOS");
		break;
	case MEGREZ:
		board_info->pcb_sku_id = kasprintf(GFP_KERNEL, "PCB SKU: MEGREZ");
		board_info->pmic_id = kasprintf(GFP_KERNEL, "PMIC: MPS5475x2+Buckx1");
		break;
	case PHECDA_DAP:
		board_info->pcb_sku_id = kasprintf(GFP_KERNEL, "PCB SKU: PHECDA_DAP");
		board_info->pmic_id = kasprintf(GFP_KERNEL, "PMIC: MP2845x2+DRMOS");
		break;
	default:
		board_info->pcb_sku_id = kasprintf(GFP_KERNEL, "PCB SKU: Reserved");
		board_info->pmic_id = kasprintf(GFP_KERNEL, "PMIC: Reserved");
		break;
	}

	/* Parse memory type */
	if (board_id & BIT(3))
		memory_type_id |= BIT(0);
	if (board_id & BIT(4))
		memory_type_id |= BIT(1);
	if (board_id & BIT(5))
		memory_type_id |= BIT(2);
	if (board_id & BIT(7))
		memory_type_id |= BIT(3);

	switch (memory_type_id) {
	case Samsung_LP5_315b_4G_Single_Rank:
		board_info->memory_type_id = kasprintf(GFP_KERNEL, "memory_type: Samsung_LP5_315b_4G_Single_Rank, vendor: SAMSUNG/K3LKBKB0BM-MGCP");
		break;
	case Hynix_LP4x_200b_4G_Dual_Rank:
		board_info->memory_type_id = kasprintf(GFP_KERNEL, "memory_type: Hynix_LP4x_200b_4G_Dual_Rank, vendor: HYNIX/H9HCNNNCPMMLXR-NEE");
		break;
	case Samsung_LP5x_315b_8G_Dual_Rank:
		board_info->memory_type_id = kasprintf(GFP_KERNEL, "memory_type: Samsung_LP5x_315b_8G_Dual_Rank, vendor: SAMSUNG/K3KL9L90QM-MGCT");
		break;
	case Samsung_LP5_315b_8G_Dual_Rank:
		board_info->memory_type_id = kasprintf(GFP_KERNEL, "memory_type: Samsung_LP5_315b_8G_Dual_Rank, vendor: SAMSUNG/K3LKCKC0BM-MGCP");
		break;
	case Samsung_LP5x_441b_8G_Single_Rank_Old:
		board_info->memory_type_id = kasprintf(GFP_KERNEL, "memory_type: Samsung_LP5x_441b_8G_Single_Rank_Old, vendor:SAMSUNG/K3KL3L30QM-JGCT(old 1z nm)");
		break;
	case Samsung_LP5_441b_4G_Single_Rank:
		board_info->memory_type_id = kasprintf(GFP_KERNEL, "memory_type: Samsung_LP5_441b_4G_Single_Rank, vendor: SAMSUNG/K3KL1L10GM-JGCT(new 1a nm)");
		break;
	case Samsung_LP5x_441b_8G_Single_Rank_New:
		board_info->memory_type_id = kasprintf(GFP_KERNEL, "memory_type: Samsung_LP5x_441b_8G_Single_Rank_New, vendor: SAMSUNG/K3KL3L30QM-JGCT(new 1a nm)");
		break;
	default:
		board_info->memory_type_id = kasprintf(GFP_KERNEL, "memory_type: Reserved, vendor: Reserved");
		break;
	}

	board_revision_id = (board_id & BOARD_VERSION_MASK) >> BOARD_VERSION_SHIFT_BITS;
	switch (board_revision_id) {
	case Rev_A:
		board_info->board_revision_id = kasprintf(GFP_KERNEL, "board_revision: Rev_A");
		break;
	case Rev_B:
		board_info->board_revision_id = kasprintf(GFP_KERNEL, "board_revision: Rev_B");
		break;
	case Rev_C:
		board_info->board_revision_id = kasprintf(GFP_KERNEL, "board_revision: Rev_C");
		break;
	case Rev_D:
		board_info->board_revision_id = kasprintf(GFP_KERNEL, "board_revision: Rev_D");
		break;
	default:
		board_info->board_revision_id = kasprintf(GFP_KERNEL, "board_revision: Unknown");
		break;
	}

	return 0;
}

static int cix_opn_info_parse(const uint64_t opn, struct cix_opn_info *opn_info)
{
	unsigned int revision, roadmap, model, segment;
	unsigned char revision_code_number, roadmap_code_number, model_code_numer, segment_code_number;

	revision = (opn & REVISION_MASK) >> REVISION_SHIFT_BITS;
	switch (revision) {
	case REVISION_A0:
		revision_code_number = 0;
		break;
	case REVISION_A1:
		revision_code_number = 1;
		break;
	case REVISION_B0:
		revision_code_number = 2;
		break;
	case REVISION_B1:
		revision_code_number = 3;
		break;
	default:
		revision_code_number = 4;
		break;

	}

	roadmap = (opn & ROADMAP_MASK) >> ROADMAP_SHIFT_BITS;
	switch (roadmap) {
	case ROADMAP_A11:
		roadmap_code_number = 0;
		break;
	case ROADMAP_A12:
		roadmap_code_number = 1;
		break;
	case ROADMAP_B11:
		roadmap_code_number = 2;
		break;
	case ROADMAP_B12:
		roadmap_code_number = 3;
		break;
	case ROADMAP_C11:
		roadmap_code_number = 4;
		break;
	case ROADMAP_C12:
		roadmap_code_number = 5;
		break;
	default:
		roadmap_code_number = 6;
		break;
	}

	model = (opn & MODEL_MASK) >> MODEL_SHIFT_BITS;
	switch (model) {
	case MODEL8180:
		model_code_numer = 0;
		break;
	case MODEL2100:
		model_code_numer = 1;
		break;
	case MODEL1100:
		model_code_numer = 2;
		break;
	case MODEL0100:
		model_code_numer = 3;
		break;
	default:
		model_code_numer = 4;
		break;
	}

	segment = (opn & SEGMENT_MASK) >> SEGMENT_SHFIT_BITS;
	switch (segment) {
	case SEGMENT_A:
		segment_code_number = 0;
		break;
	case SEGMENT_P:
		segment_code_number = 1;
		break;
	case SEGMENT_E:
		segment_code_number = 2;
		break;
	case SEGMENT_F:
		segment_code_number = 3;
		break;
	case SEGMENT_H:
		segment_code_number = 4;
		break;
	case SEGMENT_G:
		segment_code_number = 5;
		break;
	case SEGMENT_D:
		segment_code_number = 6;
		break;
	case SEGMENT_S:
		segment_code_number = 7;
		break;
	case SEGMENT_X:
		segment_code_number = 8;
		break;
	case SEGMENT_W:
		segment_code_number = 9;
		break;
	case SEGMENT_C:
		segment_code_number = 10;
		break;
	default:
		segment_code_number = 11;
		break;
	}

	opn_info->opn = kasprintf(GFP_KERNEL, "%s%s%s%s", segment_code_types[segment_code_number], \
			model_code_types[model_code_numer], roadmap_code_types[roadmap_code_number], revision_code_types[revision_code_number]);

	return 0;
}

static ssize_t firmware_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "SE Version: %s\nPBL Version: %s\nATF Version: %s\nPM Version: %s\nTEE Version: %s\nUEFI Version: %s\nEC Version:%s\nPD Version:%s\n", \
			firmware_version->se_version, firmware_version->pbl_version, firmware_version->atf_version, firmware_version->pm_version, firmware_version->tee_version, \
			firmware_version->uefi_version, firmware_version->ec_version, firmware_version->pd_version);
}

static ssize_t board_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n%s\n%s\n%s\n%s\n", board_info->board_id, board_info->pcb_sku_id,\
			board_info->pmic_id, board_info->memory_type_id, board_info->board_revision_id);
}

static ssize_t cix_opn_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", opn_info->opn);
}

static DEVICE_ATTR_RO(firmware_version);
static DEVICE_ATTR_RO(board_info);
static DEVICE_ATTR_RO(cix_opn_info);

static struct attribute *cix_soc_board_attrs[] = {
	&dev_attr_firmware_version.attr,
	&dev_attr_board_info.attr,
	&dev_attr_cix_opn_info.attr,
	NULL,
};

ATTRIBUTE_GROUPS(cix_soc_board);

static const struct of_device_id cix_sky1_socinfo_match[] = {
	{ .compatible = "cix,sky1-top", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, cix_sky1_socinfo_match);

static int cix_sky1_socinfo_probe(struct platform_device *pdev)
{
	struct soc_device_attribute *soc_dev_attr;
	struct soc_device *soc_dev;
	struct device_node *np;
	void __iomem *board_info_base;
	uint64_t cix_sky1_opn;
	unsigned int ret;

	board_info_base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(board_info_base))
		return PTR_ERR(board_info_base);

	firmware_version = kzalloc(sizeof(*firmware_version), GFP_KERNEL);
	if (!firmware_version)
		return -ENOMEM;

	board_info = kzalloc(sizeof(*board_info), GFP_KERNEL);
	if (!board_info)
		return -ENOMEM;

	firmware_version->se_version = cix_sky1_firmware_parse(board_info_base + SE_FIRMWARE_OFFSET);
	if (!firmware_version->se_version)
		return -ENOMEM;

	firmware_version->pbl_version = cix_sky1_firmware_parse(board_info_base + PBL_FIRMWARE_OFFSET);
	if (!firmware_version->pbl_version)
		return -ENOMEM;

	firmware_version->atf_version = cix_sky1_firmware_parse(board_info_base + ATF_FIRMWARE_OFFSET);
	if (!firmware_version->atf_version)
		return -ENOMEM;

	firmware_version->pm_version = cix_sky1_firmware_parse(board_info_base + PM_FIRMWARE_OFFSET);
	if (!firmware_version->pm_version)
		return -ENOMEM;

	firmware_version->tee_version = cix_sky1_firmware_parse(board_info_base + TEE_FIRMWARE_OFFSET);
	if (!firmware_version->tee_version)
		return -ENOMEM;

	firmware_version->uefi_version = cix_sky1_firmware_parse(board_info_base + UEFI_FIRMWARE_OFFSET);
	if (!firmware_version->uefi_version)
		return -ENOMEM;

	firmware_version->ec_version = cix_sky1_firmware_parse(board_info_base + EC_FIRMWARE_OFFSET);
	if (!firmware_version->ec_version)
		return -ENOMEM;

	firmware_version->pd_version = cix_sky1_pd_firmware_parse(board_info_base +PD_FIRMWARE_OFFSET);
	if (!firmware_version->pd_version)
		return -ENOMEM;

	ret = cix_sky1_board_id_parse(board_info_base + BOARD_ID_OFFSET, board_info);
	if (ret)
		return -ENOMEM;

	ret = nvmem_cell_read_u64(&pdev->dev, "opn", &cix_sky1_opn);
	if (ret)
		return ret;

	opn_info = kzalloc(sizeof(*opn_info), GFP_KERNEL);
	if (!opn_info)
		return -ENOMEM;

	soc_dev_attr = kzalloc(sizeof(*soc_dev_attr), GFP_KERNEL);
	if (!soc_dev_attr)
		return -ENODEV;

	soc_dev_attr->family = "Cix SoC";
	np = of_find_node_by_path("/");
	of_property_read_string(np, "model", &soc_dev_attr->machine);
	of_node_put(np);

	ret = cix_opn_info_parse(cix_sky1_opn, opn_info);
	if (ret)
		return -ENOMEM;

	soc_dev_attr->custom_attr_group = cix_soc_board_groups[0];

	soc_dev = soc_device_register(soc_dev_attr);
	if (IS_ERR(soc_dev)) {
		kfree_const(firmware_version->se_version);
		kfree_const(firmware_version->pbl_version);
		kfree_const(firmware_version->atf_version);
		kfree_const(firmware_version->pm_version);
		kfree_const(firmware_version->tee_version);
		kfree_const(firmware_version->uefi_version);
		kfree_const(firmware_version->ec_version);
		kfree(firmware_version);
		kfree_const(board_info->board_id);
		kfree_const(board_info->pcb_sku_id);
		kfree_const(board_info->pmic_id);
		kfree_const(board_info->memory_type_id);
		kfree_const(board_info->board_revision_id);
		kfree_const(board_info);
		kfree_const(opn_info->opn);
		kfree_const(opn_info);
		kfree(soc_dev_attr);
		return PTR_ERR(soc_dev);
	}

	dev_info(soc_device_to_device(soc_dev), "CIX SKY1 %s %s %s detected\n",
		 soc_dev_attr->revision, soc_dev_attr->serial_number, soc_dev_attr->soc_id);

	return 0;
}

static struct platform_driver cix_sky1_socinfo_driver = {
	.probe = cix_sky1_socinfo_probe,
	.driver = {
		.name = "sky1-socinfo",
		.of_match_table = cix_sky1_socinfo_match,
	},
};

module_platform_driver(cix_sky1_socinfo_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jerry Zhu <jerry.zhu@cixtech.com>");
MODULE_DESCRIPTION("Cix efuse driver");
