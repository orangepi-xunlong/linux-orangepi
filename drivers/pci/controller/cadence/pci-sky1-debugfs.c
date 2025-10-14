// SPDX-License-Identifier: GPL-2.0
/*
 * pci-sky1 - PCIe debug for CIX's sky1 SoCs
 *
 * Author: Hans Zhang <Hans.Zhang@cixtech.com>
 *
 * 20240124：add link status, msi_msix, retraining, set speed,
 * compliance, set speed debugfs node.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/reset.h>
#include <linux/irq.h>
#include <linux/msi.h>
#include <linux/debugfs.h>

#include "../../pci.h"
#include "pcie-cadence.h"
#include "pci-sky1.h"
#include "pci-sky1-debugfs.h"

#define SKY1_ASPM_STATE_L0S_UP		(1)
#define SKY1_ASPM_STATE_L0S_DW		(2)
#define SKY1_ASPM_STATE_L1		(4)
#define SKY1_ASPM_STATE_L1_1		(8)
#define SKY1_ASPM_STATE_L1_2		(0x10)
#define SKY1_ASPM_STATE_L1_1_PCIPM	(0x20)
#define SKY1_ASPM_STATE_L1_2_PCIPM	(0x40)
#define SKY1_ASPM_STATE_L1_SS_ASPM	(SKY1_ASPM_STATE_L1_1_PCIPM | SKY1_ASPM_STATE_L1_2_PCIPM)
#define SKY1_ASPM_STATE_L1_SS_PCIPM	(SKY1_ASPM_STATE_L1_1_PCIPM | SKY1_ASPM_STATE_L1_2_PCIPM)
#define SKY1_ASPM_STATE_L1_2_MASK	(SKY1_ASPM_STATE_L1_2 | SKY1_ASPM_STATE_L1_2_PCIPM)
#define SKY1_ASPM_STATE_L1SS		(SKY1_ASPM_STATE_L1_1 | SKY1_ASPM_STATE_L1_1_PCIPM |\
					SKY1_ASPM_STATE_L1_2_MASK)
#define SKY1_ASPM_STATE_L0S		(SKY1_ASPM_STATE_L0S_UP | SKY1_ASPM_STATE_L0S_DW)
#define SKY1_ASPM_STATE_ALL		(SKY1_ASPM_STATE_L0S | SKY1_ASPM_STATE_L1 |	\
					SKY1_ASPM_STATE_L1SS)

const char *sky1_ltssm_sts_name(u32 code)
{
	static const char * const sky1_ltssm_sts_strings[] = {
		"DETECT_QUIET",
		"DETECT_ACTIVE",
		"POLLING_ACTIVE",
		"POLLING_COMPLIANCE",
		"POLLING_CONFIGURATION",
		"CONFIGURATION_LINKWIDTH_START",
		"CONFIGURATION_LINKWIDTH_ACCEPT",
		"CONFIGURATION_LANENUM_ACCEPT",
		"CONFIGURATION_LANENUM_WAIT",
		"CONFIGURATION_COMPLETE",
		"CONFIGURATION_IDLE",
		"RECOVERY_RCVRLOCK",
		"RECOVERY_SPEED",
		"RECOVERY_RCVRCFG",
		"RECOVERY_IDLE",
		"NULL_STA1",
		"L0",
		"RX_L0S_ENTRY",
		"RX_L0S_IDLE",
		"RX_L0S_FTS",
		"TX_L0S_ENTRY",
		"TX_L0S_IDLE",
		"TX_L0S_FTS",
		"L1_ENTRY",
		"L1_IDLE",
		"L2_IDLE",
		"L2_TRANSMITWAKE",
		"NULL_STA2",
		"NULL_STA3",
		"NULL_STA4",
		"NULL_STA5",
		"NULL_STA6",
		"DISABLED",
		"LOOPBACK_ENTRY_MASTER",
		"LOOPBACK_ACTIVE_MASTER",
		"LOOPBACK_EXIT_MASTER",
		"LOOPBACK_ENTRY_SLAVE",
		"LOOPBACK_ACTIVE_SLAVE",
		"LOOPBACK_EXIT_SLAVE",
		"HOT_RESET",
		"RECOVERY_EQ_PHASE_0",
		"RECOVERY_EQ_PHASE_1",
		"RECOVERY_EQ_PHASE_2",
		"RECOVERY_EQ_PHASE_3",
	};

	if (code < ARRAY_SIZE(sky1_ltssm_sts_strings))
		return sky1_ltssm_sts_strings[code];
	return "Unknown";
}


const char *sky1_local_err_msg(enum sky1_local_err err, u32 code)
{
#define ERR_MSG_LEN 32
	static const char *const local_err0_int_msg[ERR_MSG_LEN] = {
		"PRXFPE", // [0]
		"NPRXFPE", // [1]
		"SCFPE", // [2]
		"RBRPE", // [3]
		"HLSRPE", // [4]
		"AXIRPE", // [5]
		"null", // [6]
		"null", // [7]
		"PFOVFL", // [8]
		"NPFOVFL", // [9]
		"CPFOVFL", // [10]
		"RETTO", // [11]
		"RETRL", // [12]
		"PHYER", // [13]
		"MLTLRX", // [14]
		"UNEXPCPL", // [15]
		"FCERR", // [16]
		"CPLTO", // [17]
		"USERER", // [18]
		"MSMMSGE", // [19]
		"HWAUWCDT", // [20]
		"null", // [21]
		"ISWMCRX", // [22]
		"IPHMCRX", // [23]
		"WRAWTOE", // [24]
		"UNPHRER", // [25]
		"NFTTO", // [26]
		"PTMCXAI", // [27]
		"null", // [28]
		"LNEQREI", // [29]
		"ERRLGOVF", // [30]
		"LTSSTTRAN", // [31]
	};

	static const char *const local_err1_int_msg[ERR_MSG_LEN] = {
		"PMETORX", // [0]
		"null", // [1]
		"null", // [2]
		"L0PENTGL", // [3]
		"MSWTOE", // [4]
		"PMPMEI", // [5]
		"null", // [6]
		"null", // [7]
		"null", // [8]
		"null", // [9]
		"null", // [10]
		"null", // [11]
		"null", // [12]
		"null", // [13]
		"null", // [14]
		"null", // [15]
		"null", // [16]
		"null", // [17]
		"null", // [18]
		"null", // [19]
		"null", // [20]
		"null", // [21]
		"null", // [22]
		"null", // [23]
		"null", // [24]
		"null", // [25]
		"null", // [26]
		"null", // [27]
		"null", // [28]
		"null", // [29]
		"null", // [30]
		"null", // [31]
	};

	if (code < ERR_MSG_LEN) {
		if (err == PCIE_LOCAL_ERR0)
			return local_err0_int_msg[code];
		else if (err == PCIE_LOCAL_ERR1)
			return local_err1_int_msg[code];
	}
	return "Unknown";
}

const char *sky1_ltssttran_msg(u32 code)
{
	static const char *const local_ltssttran_msg[LTSSTTRAN_MSG_LEN] = {
		"HOT_RESET_1 -> TX_ELEC_IDLE_ST", // [0]
		"DISABLE_LINK_2 -> DISABLE_LINK_3", // [1]
		"TX_ELEC_IDLE_2 -> DETECT_QUIET_ENTRY", // [2]
		"DISABLE_LINK_6 -> DETECT_QUIET_ENTRY", // [3]
		"l2_IDLE -> DETECT_QUIET_ENTRY", // [4]
		"TX_ELEC_IDLE_3 -> DETECT_QUIET_ENTRY", // [5]
		"HOT_RESET -> HOT_RESET_1", // [6]
		"null", // [7]
	};

	if (code < LTSSTTRAN_MSG_LEN)
		return local_ltssttran_msg[code];
	return "Unknown";
}

#ifdef CONFIG_DEBUG_FS
static void sky1_pcie_msi_msix_test(struct sky1_pcie *pcie, int irq)
{
	struct device *dev = pcie->dev;
	struct msi_desc *msi_desc;

	msi_desc = irq_get_msi_desc(irq);
	if (!msi_desc) {
		dev_err(dev, "Failed to get msi_desc\n");
		return;
	}

	dev_info(dev,
		 "address_hi = 0x%08x, address_lo = 0x%08x, data = 0x%08x\n",
		 msi_desc->msg.address_hi, msi_desc->msg.address_lo,
		 msi_desc->msg.data);
}

static int sky1_pcie_get_link_pci_dev(struct sky1_pcie *pcie)
{
	struct pci_host_bridge *bridge;
	struct pci_bus *child, *root_bus = NULL;
	struct pci_dev *pdev;
	struct device *dev = pcie->dev;

	bridge = pci_host_bridge_from_priv(pcie->cdns_pcie_rc);
	if (!bridge)
		return -EINVAL;

	list_for_each_entry(child, &bridge->bus->children, node) {
		if (child->parent == bridge->bus) {
			root_bus = child;
			break;
		}
	}

	list_for_each_entry(pdev, &root_bus->devices, bus_list)
		if (PCI_SLOT(pdev->devfn) == 0)
			pcie->ep_pdev = pdev;

	pcie->rc_pdev = pcie_find_root_port(pcie->ep_pdev);
	if (!pcie->rc_pdev) {
		dev_err(dev, "find pcie->rc_pdev error");
		return -EINVAL;
	}

	return 0;
}

static u32 sky1_calc_l0s_latency(u32 lnkcap)
{
	u32 encoding = (lnkcap & PCI_EXP_LNKCAP_L0SEL) >> 12;

	if (encoding == 0x7)
		return (5 * 1000);
	return (64 << encoding);
}

static u32 sky1_calc_l0s_acceptable(u32 encoding)
{
	if (encoding == 0x7)
		return -1U;
	return (64 << encoding);
}

static u32 sky1_calc_l1_latency(u32 lnkcap)
{
	u32 encoding = (lnkcap & PCI_EXP_LNKCAP_L1EL) >> 15;

	if (encoding == 0x7)
		return (65 * 1000);
	return (1000 << encoding);
}

static u32 sky1_calc_l1_acceptable(u32 encoding)
{
	if (encoding == 0x7)
		return -1U;
	return (1000 << encoding);
}

static u32 sky1_calc_l1ss_pwron(struct pci_dev *pdev, u32 scale, u32 val)
{
	switch (scale) {
	case 0:
		return val * 2;
	case 1:
		return val * 10;
	case 2:
		return val * 100;
	}
	pci_err(pdev, "%s: Invalid T_PwrOn scale: %u\n", __func__, scale);
	return 0;
}

static void sky1_encode_l12_threshold(u32 threshold_us, u32 *scale, u32 *value)
{
	u64 threshold_ns = (u64) threshold_us * 1000;

	if (threshold_ns <= 0x3ff * 1) {
		*scale = 0;
		*value = threshold_ns;
	} else if (threshold_ns <= 0x3ff * 32) {
		*scale = 1;
		*value = roundup(threshold_ns, 32) / 32;
	} else if (threshold_ns <= 0x3ff * 1024) {
		*scale = 2;
		*value = roundup(threshold_ns, 1024) / 1024;
	} else if (threshold_ns <= 0x3ff * 32768) {
		*scale = 3;
		*value = roundup(threshold_ns, 32768) / 32768;
	} else if (threshold_ns <= 0x3ff * 1048576) {
		*scale = 4;
		*value = roundup(threshold_ns, 1048576) / 1048576;
	} else if (threshold_ns <= 0x3ff * (u64) 33554432) {
		*scale = 5;
		*value = roundup(threshold_ns, 33554432) / 33554432;
	} else {
		*scale = 5;
		*value = 0x3ff;
	}
}

static void sky1_pcie_aspm_check_latency(struct sky1_pcie *pcie, struct pci_dev *endpoint)
{
	u32 latency, encoding, lnkcap_up, lnkcap_dw;
	u32 l1_switch_latency = 0, latency_up_l0s;
	u32 latency_up_l1, latency_dw_l0s, latency_dw_l1;
	u32 acceptable_l0s, acceptable_l1;

	encoding = (endpoint->devcap & PCI_EXP_DEVCAP_L0S) >> 6;
	acceptable_l0s = sky1_calc_l0s_acceptable(encoding);

	encoding = (endpoint->devcap & PCI_EXP_DEVCAP_L1) >> 9;
	acceptable_l1 = sky1_calc_l1_acceptable(encoding);

	pcie_capability_read_dword(pcie->ep_pdev, PCI_EXP_LNKCAP,
				   &lnkcap_up);
	pcie_capability_read_dword(pcie->rc_pdev, PCI_EXP_LNKCAP,
				   &lnkcap_dw);
	latency_up_l0s = sky1_calc_l0s_latency(lnkcap_up);
	latency_up_l1 = sky1_calc_l1_latency(lnkcap_up);
	latency_dw_l0s = sky1_calc_l0s_latency(lnkcap_dw);
	latency_dw_l1 = sky1_calc_l1_latency(lnkcap_dw);

	if ((pcie->aspm_capable & SKY1_ASPM_STATE_L0S_UP) &&
		(latency_up_l0s > acceptable_l0s)) {
		pcie->aspm_capable &= ~SKY1_ASPM_STATE_L0S_UP;
		pcie->L0s_support = false;
		dev_err(pcie->dev, "EP latency not suport L0s\n");
	}

	if ((pcie->aspm_capable & SKY1_ASPM_STATE_L0S_DW) &&
		(latency_dw_l0s > acceptable_l0s)) {
		pcie->aspm_capable &= ~SKY1_ASPM_STATE_L0S_DW;
		pcie->L0s_support = false;
		dev_err(pcie->dev, "RC latency not suport L0s\n");
	}

	if ((pcie->aspm_capable & SKY1_ASPM_STATE_L0S_UP) &&
		(pcie->aspm_capable & SKY1_ASPM_STATE_L0S_DW))
		pcie->L0s_support = true;

	latency = max_t(u32, latency_up_l1, latency_dw_l1);
	if ((pcie->aspm_capable & SKY1_ASPM_STATE_L1) &&
		(latency + l1_switch_latency > acceptable_l1)) {
		pcie->aspm_capable &= ~SKY1_ASPM_STATE_L1;
		pcie->L1ss_support = false;
		dev_err(pcie->dev, "not suport L1ss\n");
	} else
		pcie->L1ss_support = true;
}

static void sky1_pci_clear_and_set_dword(struct pci_dev *pdev, int pos,
				    u32 clear, u32 set)
{
	u32 val;

	pci_read_config_dword(pdev, pos, &val);
	val &= ~clear;
	val |= set;
	pci_write_config_dword(pdev, pos, val);
}

static void sky1_aspm_calc_l1ss_info(struct sky1_pcie *pcie,
				u32 parent_l1ss_cap, u32 child_l1ss_cap)
{
	struct pci_dev *child = pcie->ep_pdev, *parent = pcie->rc_pdev;
	u32 val1, val2, scale1, scale2;
	u32 t_common_mode, t_power_on, l1_2_threshold, scale, value;
	u32 ctl1 = 0, ctl2 = 0;
	u32 pctl1, pctl2, cctl1, cctl2;
	u32 pl1_2_enables, cl1_2_enables;

	if (!(pcie->aspm_support & SKY1_ASPM_STATE_L1_2_MASK))
		return;

	val1 = (parent_l1ss_cap & PCI_L1SS_CAP_CM_RESTORE_TIME) >> 8;
	val2 = (child_l1ss_cap & PCI_L1SS_CAP_CM_RESTORE_TIME) >> 8;
	t_common_mode = max(val1, val2);

	val1   = (parent_l1ss_cap & PCI_L1SS_CAP_P_PWR_ON_VALUE) >> 19;
	scale1 = (parent_l1ss_cap & PCI_L1SS_CAP_P_PWR_ON_SCALE) >> 16;
	val2   = (child_l1ss_cap & PCI_L1SS_CAP_P_PWR_ON_VALUE) >> 19;
	scale2 = (child_l1ss_cap & PCI_L1SS_CAP_P_PWR_ON_SCALE) >> 16;

	if (sky1_calc_l1ss_pwron(parent, scale1, val1) >
	    sky1_calc_l1ss_pwron(child, scale2, val2)) {
		ctl2 |= scale1 | (val1 << 3);
		t_power_on = sky1_calc_l1ss_pwron(parent, scale1, val1);
	} else {
		ctl2 |= scale2 | (val2 << 3);
		t_power_on = sky1_calc_l1ss_pwron(child, scale2, val2);
	}

	l1_2_threshold = 2 + 4 + t_common_mode + t_power_on;
	sky1_encode_l12_threshold(l1_2_threshold, &scale, &value);
	ctl1 |= t_common_mode << 8 | scale << 29 | value << 16;

	pci_read_config_dword(parent, parent->l1ss + PCI_L1SS_CTL1, &pctl1);
	pci_read_config_dword(parent, parent->l1ss + PCI_L1SS_CTL2, &pctl2);
	pci_read_config_dword(child, child->l1ss + PCI_L1SS_CTL1, &cctl1);
	pci_read_config_dword(child, child->l1ss + PCI_L1SS_CTL2, &cctl2);

	if (ctl1 == pctl1 && ctl1 == cctl1 &&
	    ctl2 == pctl2 && ctl2 == cctl2)
		return;

	pl1_2_enables = pctl1 & PCI_L1SS_CTL1_L1_2_MASK;
	cl1_2_enables = cctl1 & PCI_L1SS_CTL1_L1_2_MASK;

	if (pl1_2_enables || cl1_2_enables) {
		sky1_pci_clear_and_set_dword(child, child->l1ss + PCI_L1SS_CTL1,
					PCI_L1SS_CTL1_L1_2_MASK, 0);
		sky1_pci_clear_and_set_dword(parent, parent->l1ss + PCI_L1SS_CTL1,
					PCI_L1SS_CTL1_L1_2_MASK, 0);
	}

	pci_write_config_dword(parent, parent->l1ss + PCI_L1SS_CTL2, ctl2);
	pci_write_config_dword(child, child->l1ss + PCI_L1SS_CTL2, ctl2);

	sky1_pci_clear_and_set_dword(parent, parent->l1ss + PCI_L1SS_CTL1,
				PCI_L1SS_CTL1_CM_RESTORE_TIME, ctl1);

	sky1_pci_clear_and_set_dword(parent,	parent->l1ss + PCI_L1SS_CTL1,
				PCI_L1SS_CTL1_LTR_L12_TH_VALUE |
				PCI_L1SS_CTL1_LTR_L12_TH_SCALE, ctl1);
	sky1_pci_clear_and_set_dword(child, child->l1ss + PCI_L1SS_CTL1,
				PCI_L1SS_CTL1_LTR_L12_TH_VALUE |
				PCI_L1SS_CTL1_LTR_L12_TH_SCALE, ctl1);

	if (pl1_2_enables || cl1_2_enables) {
		sky1_pci_clear_and_set_dword(parent, parent->l1ss + PCI_L1SS_CTL1, 0,
					pl1_2_enables);
		sky1_pci_clear_and_set_dword(child, child->l1ss + PCI_L1SS_CTL1, 0,
					cl1_2_enables);
	}
}

static void sky1_aspm_l1ss_init(struct sky1_pcie *pcie)
{
	struct pci_dev *child = pcie->ep_pdev, *parent = pcie->rc_pdev;
	u32 parent_l1ss_cap, child_l1ss_cap;
	u32 parent_l1ss_ctl1 = 0, child_l1ss_ctl1 = 0;

	if (!parent->l1ss || !child->l1ss)
		return;

	pci_read_config_dword(parent, parent->l1ss + PCI_L1SS_CAP,
			      &parent_l1ss_cap);
	pci_read_config_dword(child, child->l1ss + PCI_L1SS_CAP,
			      &child_l1ss_cap);

	if (!(parent_l1ss_cap & PCI_L1SS_CAP_L1_PM_SS))
		parent_l1ss_cap = 0;
	if (!(child_l1ss_cap & PCI_L1SS_CAP_L1_PM_SS))
		child_l1ss_cap = 0;

	if (!child->ltr_path)
		child_l1ss_cap &= ~PCI_L1SS_CAP_ASPM_L1_2;

	if (!pcie->aspm_not_support) {
		if (parent_l1ss_cap & child_l1ss_cap & PCI_L1SS_CAP_ASPM_L1_1)
			pcie->aspm_support |= SKY1_ASPM_STATE_L1_1;
		if (parent_l1ss_cap & child_l1ss_cap & PCI_L1SS_CAP_ASPM_L1_2)
			pcie->aspm_support |= SKY1_ASPM_STATE_L1_2;
	}
	if (parent_l1ss_cap & child_l1ss_cap & PCI_L1SS_CAP_PCIPM_L1_1)
		pcie->aspm_support |= SKY1_ASPM_STATE_L1_1_PCIPM;
	if (parent_l1ss_cap & child_l1ss_cap & PCI_L1SS_CAP_PCIPM_L1_2)
		pcie->aspm_support |= SKY1_ASPM_STATE_L1_2_PCIPM;

	if (parent_l1ss_cap)
		pci_read_config_dword(parent, parent->l1ss + PCI_L1SS_CTL1,
				      &parent_l1ss_ctl1);
	if (child_l1ss_cap)
		pci_read_config_dword(child, child->l1ss + PCI_L1SS_CTL1,
				      &child_l1ss_ctl1);

	if (pcie->aspm_support & SKY1_ASPM_STATE_L1SS)
		sky1_aspm_calc_l1ss_info(pcie, parent_l1ss_cap, child_l1ss_cap);
}

static void sky1_pcie_pcipm_l1ss_cap_init(struct sky1_pcie *pcie)
{
	struct pci_dev *child = pcie->ep_pdev;

	sky1_aspm_l1ss_init(pcie);
	pcie->aspm_capable = pcie->aspm_support;
	sky1_pcie_aspm_check_latency(pcie, child);
}

static void sky1_pcie_aspm_cap_init(struct sky1_pcie *pcie)
{
	struct pci_dev *child = pcie->ep_pdev, *parent = pcie->rc_pdev;
	u32 parent_lnkcap, child_lnkcap;

	pcie_capability_read_dword(parent, PCI_EXP_LNKCAP, &parent_lnkcap);
	pcie_capability_read_dword(child, PCI_EXP_LNKCAP, &child_lnkcap);
	if (!(parent_lnkcap & child_lnkcap & PCI_EXP_LNKCAP_ASPMS)) {
		dev_err(pcie->dev, "not suport aspm)\n");
		pcie->aspm_not_support = true;
		sky1_pcie_pcipm_l1ss_cap_init(pcie);
		return;
	}

	if (parent_lnkcap & child_lnkcap & PCI_EXP_LNKCAP_ASPM_L0S)
		pcie->aspm_support |= SKY1_ASPM_STATE_L0S;
	else
		dev_err(pcie->dev, "link cap not suport L0s\n");

	if (parent_lnkcap & child_lnkcap & PCI_EXP_LNKCAP_ASPM_L1)
		pcie->aspm_support |= SKY1_ASPM_STATE_L1;
	else
		dev_err(pcie->dev, "link cap not suport L1ss\n");

	sky1_aspm_l1ss_init(pcie);
	pcie->aspm_capable = pcie->aspm_support;
	sky1_pcie_aspm_check_latency(pcie, child);
}

static void sky1_link_l1sub_sta(struct sky1_pcie *pcie, int val)
{
	char *sta = NULL;
	int l1sub = (val >> 1) & 0x7;

	switch (l1sub)
	{
	case 0:
		sta = "L1-substate machine not active";
		break;
	case 1:
		sta = "L1.0 substate";
		break;
	case 2:
		sta = "L1.1 substate";
		break;
	case 4:
		sta = "L1.2 entry";
		break;
	case 5:
		sta = "L1.2 idle";
		break;
	case 6:
		sta = "L1.2 exit";
		break;
	default:
		sta = "not correct";
		break;
	}
	dev_info(pcie->dev, "sky1 l1sub val = 0x%08x l1sub sta = %s\n", l1sub, sta);
}

static void sky1_link_status_lpwr(struct sky1_pcie *pcie, int cnt, bool l1)
{
	void __iomem *status_addr;
	u32 val, ltssm;
	u32 i = 0;

	status_addr = pcie->status_base + STATUS_REG(0);
	do {
		msleep(100);
		val = sky1_pcie_get_bits32(status_addr, ~0);
		ltssm = (val >> PCIE_LTSSM_STATUS_SHIFT) & 0x3f;
		dev_info(pcie->dev, "sky1 status = 0x%08x ltssm = %d\n", val, ltssm);

		if (l1)
			sky1_link_l1sub_sta(pcie, val);
		dev_info(pcie->dev, "sky1 link status is 6'd%d %s\n", ltssm,
			 sky1_ltssm_sts_name(ltssm));
		i++;
	} while (i < cnt);
}

static void sky1_l0s_test(struct sky1_pcie *pcie)
{
	u32 rc_linkctrl_default, ep_linkctrl_default;
	struct pci_dev *rc_pdev = pcie->rc_pdev;
	struct pci_dev *ep_pdev = pcie->ep_pdev;
	u32 reg;

	if ((!pcie->L0s_support) ||
	    ((pcie->aspm_support & SKY1_ASPM_STATE_L0S) != SKY1_ASPM_STATE_L0S))
		return;

	pcie_capability_read_dword(rc_pdev, PCI_EXP_LNKCTL, &rc_linkctrl_default);
	pcie_capability_read_dword(ep_pdev, PCI_EXP_LNKCTL, &ep_linkctrl_default);

	reg = rc_linkctrl_default | PCI_EXP_LNKCTL_ASPM_L0S;
	pcie_capability_write_dword(rc_pdev, PCI_EXP_LNKCTL, reg);

	reg = ep_linkctrl_default | PCI_EXP_LNKCTL_ASPM_L0S;
	pcie_capability_write_dword(ep_pdev, PCI_EXP_LNKCTL, reg);

	sky1_link_status_lpwr(pcie, 30, false);

	pcie_capability_write_dword(rc_pdev, PCI_EXP_LNKCTL, rc_linkctrl_default);
	pcie_capability_write_dword(ep_pdev, PCI_EXP_LNKCTL, ep_linkctrl_default);

	sky1_link_status_lpwr(pcie, 2, false);
}

void sky1_l10_aspm_test(struct sky1_pcie *pcie)
{
	u32 rc_linkctrl_default, ep_linkctrl_default;
	struct pci_dev *rc_pdev = pcie->rc_pdev;
	struct pci_dev *ep_pdev = pcie->ep_pdev;
	u32 reg;

	if ((pcie->aspm_not_support) ||
	    (!(pcie->aspm_support & SKY1_ASPM_STATE_L1)) ||
	    (!pcie->L1ss_support))
		return;

	pcie_capability_read_dword(rc_pdev, PCI_EXP_LNKCTL, &rc_linkctrl_default);
	pcie_capability_read_dword(ep_pdev, PCI_EXP_LNKCTL, &ep_linkctrl_default);

	reg = rc_linkctrl_default | PCI_EXP_LNKCTL_ASPM_L1;
	pcie_capability_write_dword(rc_pdev, PCI_EXP_LNKCTL, reg);
	reg = ep_linkctrl_default | PCI_EXP_LNKCTL_ASPM_L1;
	pcie_capability_write_dword(ep_pdev, PCI_EXP_LNKCTL, reg);

	sky1_link_status_lpwr(pcie, 4, true);

	pcie_capability_write_dword(rc_pdev, PCI_EXP_LNKCTL, rc_linkctrl_default);
	pcie_capability_write_dword(ep_pdev, PCI_EXP_LNKCTL, ep_linkctrl_default);

	sky1_link_status_lpwr(pcie, 2, false);
}

void sky1_l11_aspm_test(struct sky1_pcie *pcie)
{
	u32 rc_linkctrl_default, ep_linkctrl_default;
	u32 rc_l1ssctrl_default, ep_l1ssctrl_default;
	u16 rc_l1ss_offset = 0, ep_l1ss_offset = 0;
	struct pci_dev *rc_pdev = pcie->rc_pdev;
	struct pci_dev *ep_pdev = pcie->ep_pdev;
	u32 reg;

	if ((pcie->aspm_not_support) ||
	    (!(pcie->aspm_support & SKY1_ASPM_STATE_L1_1)) ||
	    (!pcie->L1ss_support))
		return;

	rc_l1ss_offset = pci_find_ext_capability(rc_pdev, PCI_EXT_CAP_ID_L1SS);
	ep_l1ss_offset = pci_find_ext_capability(ep_pdev, PCI_EXT_CAP_ID_L1SS);
	pci_read_config_dword(rc_pdev, rc_l1ss_offset + PCI_L1SS_CTL1, &rc_l1ssctrl_default);
	pci_read_config_dword(ep_pdev, ep_l1ss_offset + PCI_L1SS_CTL1, &ep_l1ssctrl_default);

	reg = rc_l1ssctrl_default | PCI_L1SS_CTL1_ASPM_L1_1;
	pci_write_config_dword(rc_pdev, rc_l1ss_offset + PCI_L1SS_CTL1, reg);
	reg = ep_l1ssctrl_default | PCI_L1SS_CTL1_ASPM_L1_1;
	pci_write_config_dword(ep_pdev, ep_l1ss_offset + PCI_L1SS_CTL1, reg);

	pcie_capability_read_dword(rc_pdev, PCI_EXP_LNKCTL, &rc_linkctrl_default);
	pcie_capability_read_dword(ep_pdev, PCI_EXP_LNKCTL, &ep_linkctrl_default);

	reg = rc_linkctrl_default | PCI_EXP_LNKCTL_ASPM_L1;
	pcie_capability_write_dword(rc_pdev, PCI_EXP_LNKCTL, reg);
	reg = ep_linkctrl_default | PCI_EXP_LNKCTL_ASPM_L1;
	pcie_capability_write_dword(ep_pdev, PCI_EXP_LNKCTL, reg);

	sky1_link_status_lpwr(pcie, 4, true);

	pci_write_config_dword(rc_pdev, rc_l1ss_offset + PCI_L1SS_CTL1, rc_l1ssctrl_default);
	pci_write_config_dword(ep_pdev, ep_l1ss_offset + PCI_L1SS_CTL1, ep_l1ssctrl_default);
	pcie_capability_write_dword(rc_pdev, PCI_EXP_LNKCTL, rc_linkctrl_default);
	pcie_capability_write_dword(ep_pdev, PCI_EXP_LNKCTL, ep_linkctrl_default);

	sky1_link_status_lpwr(pcie, 2, false);
}

void sky1_l12_aspm_test(struct sky1_pcie *pcie)
{
	u32 rc_linkctrl_default, ep_linkctrl_default;
	u32 rc_l1ssctrl_default, ep_l1ssctrl_default;
	u16 rc_l1ss_offset = 0, ep_l1ss_offset = 0;
	struct pci_dev *rc_pdev = pcie->rc_pdev;
	struct pci_dev *ep_pdev = pcie->ep_pdev;
	u32 reg;

	if ((pcie->aspm_not_support) ||
	    (!(pcie->aspm_support & SKY1_ASPM_STATE_L1_2)) ||
	    (!pcie->L1ss_support))
		return;

	rc_l1ss_offset = pci_find_ext_capability(rc_pdev, PCI_EXT_CAP_ID_L1SS);
	ep_l1ss_offset = pci_find_ext_capability(ep_pdev, PCI_EXT_CAP_ID_L1SS);
	pci_read_config_dword(rc_pdev, rc_l1ss_offset + PCI_L1SS_CTL1, &rc_l1ssctrl_default);
	pci_read_config_dword(ep_pdev, ep_l1ss_offset + PCI_L1SS_CTL1, &ep_l1ssctrl_default);

	reg = rc_l1ssctrl_default | PCI_L1SS_CTL1_ASPM_L1_2;
	pci_write_config_dword(rc_pdev, rc_l1ss_offset + PCI_L1SS_CTL1, reg);
	reg = ep_l1ssctrl_default | PCI_L1SS_CTL1_ASPM_L1_2;
	pci_write_config_dword(ep_pdev, ep_l1ss_offset + PCI_L1SS_CTL1, reg);

	pcie_capability_read_dword(rc_pdev, PCI_EXP_LNKCTL, &rc_linkctrl_default);
	pcie_capability_read_dword(ep_pdev, PCI_EXP_LNKCTL, &ep_linkctrl_default);

	reg = rc_linkctrl_default | PCI_EXP_LNKCTL_ASPM_L1;
	pcie_capability_write_dword(rc_pdev, PCI_EXP_LNKCTL, reg);
	reg = ep_linkctrl_default | PCI_EXP_LNKCTL_ASPM_L1;
	pcie_capability_write_dword(ep_pdev, PCI_EXP_LNKCTL, reg);

	sky1_link_status_lpwr(pcie, 4, true);

	pci_write_config_dword(rc_pdev, rc_l1ss_offset + PCI_L1SS_CTL1, rc_l1ssctrl_default);
	pci_write_config_dword(ep_pdev, ep_l1ss_offset + PCI_L1SS_CTL1, ep_l1ssctrl_default);
	pcie_capability_write_dword(rc_pdev, PCI_EXP_LNKCTL, rc_linkctrl_default);
	pcie_capability_write_dword(ep_pdev, PCI_EXP_LNKCTL, ep_linkctrl_default);

	sky1_link_status_lpwr(pcie, 2, false);
}

void sky1_l10_pcipm_test(struct sky1_pcie *pcie)
{
	u32 rc_pmctrl_default, ep_pmctrl_default;
	u8 rc_pm_offset, ep_pm_offset;
	struct pci_dev *rc_pdev = pcie->rc_pdev;
	struct pci_dev *ep_pdev = pcie->ep_pdev;
	u32 reg;

	rc_pm_offset = pci_find_capability(rc_pdev, PCI_CAP_ID_PM);
	ep_pm_offset = pci_find_capability(ep_pdev, PCI_CAP_ID_PM);
	pci_read_config_dword(rc_pdev, rc_pm_offset + PCI_PM_CTRL, &rc_pmctrl_default);
	pci_read_config_dword(ep_pdev, ep_pm_offset + PCI_PM_CTRL, &ep_pmctrl_default);

	reg = rc_pmctrl_default | 3;
	pci_write_config_dword(rc_pdev, rc_pm_offset + PCI_PM_CTRL, reg);
	reg = ep_pmctrl_default | 3;
	pci_write_config_dword(ep_pdev, ep_pm_offset + PCI_PM_CTRL, reg);

	sky1_link_status_lpwr(pcie, 4, true);

	pci_write_config_dword(rc_pdev, rc_pm_offset + PCI_PM_CTRL, rc_pmctrl_default);
	pci_write_config_dword(ep_pdev, ep_pm_offset + PCI_PM_CTRL, ep_pmctrl_default);

	sky1_link_status_lpwr(pcie, 2, false);
}

void sky1_l11_pcipm_test(struct sky1_pcie *pcie)
{
	u32 rc_l1ssctrl_default, ep_l1ssctrl_default;
	u32 rc_pmctrl_default, ep_pmctrl_default;
	struct pci_dev *rc_pdev = pcie->rc_pdev;
	struct pci_dev *ep_pdev = pcie->ep_pdev;
	u8 rc_pm_offset, ep_pm_offset;
	u16 rc_l1ss_offset = 0, ep_l1ss_offset = 0;
	u32 reg;

	if ((!pcie->L1ss_support) ||
	    (!(pcie->aspm_support & SKY1_ASPM_STATE_L1_1_PCIPM)))
		return;

	rc_l1ss_offset = pci_find_ext_capability(rc_pdev, PCI_EXT_CAP_ID_L1SS);
	ep_l1ss_offset = pci_find_ext_capability(ep_pdev, PCI_EXT_CAP_ID_L1SS);
	pci_read_config_dword(rc_pdev, rc_l1ss_offset + PCI_L1SS_CTL1, &rc_l1ssctrl_default);
	pci_read_config_dword(ep_pdev, ep_l1ss_offset + PCI_L1SS_CTL1, &ep_l1ssctrl_default);

	reg = rc_l1ssctrl_default | PCI_L1SS_CTL1_PCIPM_L1_1;
	pci_write_config_dword(rc_pdev, rc_l1ss_offset + PCI_L1SS_CTL1, reg);
	reg = ep_l1ssctrl_default | PCI_L1SS_CTL1_PCIPM_L1_1;
	pci_write_config_dword(ep_pdev, ep_l1ss_offset + PCI_L1SS_CTL1, reg);

	rc_pm_offset = pci_find_capability(rc_pdev, PCI_CAP_ID_PM);
	ep_pm_offset = pci_find_capability(ep_pdev, PCI_CAP_ID_PM);
	pci_read_config_dword(rc_pdev, rc_pm_offset + PCI_PM_CTRL, &rc_pmctrl_default);
	pci_read_config_dword(ep_pdev, ep_pm_offset + PCI_PM_CTRL, &ep_pmctrl_default);

	reg = rc_pmctrl_default | 3;
	pci_write_config_dword(rc_pdev, rc_pm_offset + PCI_PM_CTRL, reg);

	reg = ep_pmctrl_default | 3;
	pci_write_config_dword(ep_pdev, ep_pm_offset + PCI_PM_CTRL, reg);

	sky1_link_status_lpwr(pcie, 4, true);

	pci_write_config_dword(rc_pdev, rc_l1ss_offset + PCI_L1SS_CTL1, rc_l1ssctrl_default);
	pci_write_config_dword(ep_pdev, ep_l1ss_offset + PCI_L1SS_CTL1, ep_l1ssctrl_default);
	pci_write_config_dword(rc_pdev, rc_pm_offset + PCI_PM_CTRL, rc_pmctrl_default);
	pci_write_config_dword(ep_pdev, ep_pm_offset + PCI_PM_CTRL, ep_pmctrl_default);

	sky1_link_status_lpwr(pcie, 2, false);
}

void sky1_l12_pcipm_test(struct sky1_pcie *pcie)
{
	u32 rc_l1ssctrl_default, ep_l1ssctrl_default;
	u32 rc_pmctrl_default, ep_pmctrl_default;
	struct pci_dev *rc_pdev = pcie->rc_pdev;
	struct pci_dev *ep_pdev = pcie->ep_pdev;
	u8 rc_pm_offset, ep_pm_offset;
	u16 rc_l1ss_offset = 0, ep_l1ss_offset = 0;
	u32 reg;

	if ((!pcie->L1ss_support) ||
	    (!(pcie->aspm_support & SKY1_ASPM_STATE_L1_2_PCIPM)))
		return;

	rc_l1ss_offset = pci_find_ext_capability(rc_pdev, PCI_EXT_CAP_ID_L1SS);
	ep_l1ss_offset = pci_find_ext_capability(ep_pdev, PCI_EXT_CAP_ID_L1SS);
	pci_read_config_dword(rc_pdev, rc_l1ss_offset + PCI_L1SS_CTL1, &rc_l1ssctrl_default);
	pci_read_config_dword(ep_pdev, ep_l1ss_offset + PCI_L1SS_CTL1, &ep_l1ssctrl_default);

	reg = rc_l1ssctrl_default | PCI_L1SS_CTL1_PCIPM_L1_2;
	pci_write_config_dword(rc_pdev, rc_l1ss_offset + PCI_L1SS_CTL1, reg);
	reg = ep_l1ssctrl_default | PCI_L1SS_CTL1_PCIPM_L1_2;
	pci_write_config_dword(ep_pdev, ep_l1ss_offset + PCI_L1SS_CTL1, reg);

	rc_pm_offset = pci_find_capability(rc_pdev, PCI_CAP_ID_PM);
	ep_pm_offset = pci_find_capability(ep_pdev, PCI_CAP_ID_PM);
	pci_read_config_dword(rc_pdev, rc_pm_offset + PCI_PM_CTRL, &rc_pmctrl_default);
	pci_read_config_dword(ep_pdev, ep_pm_offset + PCI_PM_CTRL, &ep_pmctrl_default);

	reg = rc_pmctrl_default | 3;
	pci_write_config_dword(rc_pdev, rc_pm_offset + PCI_PM_CTRL, reg);

	reg = ep_pmctrl_default | 3;
	pci_write_config_dword(ep_pdev, ep_pm_offset + PCI_PM_CTRL, reg);

	sky1_link_status_lpwr(pcie, 4, true);

	pci_write_config_dword(rc_pdev, rc_l1ss_offset + PCI_L1SS_CTL1, rc_l1ssctrl_default);
	pci_write_config_dword(ep_pdev, ep_l1ss_offset + PCI_L1SS_CTL1, ep_l1ssctrl_default);
	pci_write_config_dword(rc_pdev, rc_pm_offset + PCI_PM_CTRL, rc_pmctrl_default);
	pci_write_config_dword(ep_pdev, ep_pm_offset + PCI_PM_CTRL, ep_pmctrl_default);

	sky1_link_status_lpwr(pcie, 2, false);
}

static void sky1_pcie_lpwr_test(struct sky1_pcie *pcie, enum esky1_lpwr type)
{
	struct device *dev = pcie->dev;

	if (sky1_pcie_get_link_pci_dev(pcie)) {
		dev_err(dev, "PCIe link enum have a problem");
		return;
	}
	sky1_pcie_aspm_cap_init(pcie);

	switch (type) {
	case eLPWR_L0S:
		sky1_l0s_test(pcie);
		break;
	case eLPWR_ASPM_L10:
		sky1_l10_aspm_test(pcie);
		break;
	case eLPWR_ASPM_L11:
		sky1_l11_aspm_test(pcie);
		break;
	case eLPWR_ASPM_L12:
		sky1_l12_aspm_test(pcie);
		break;
	case eLPWR_PCIPM_L10:
		sky1_l10_pcipm_test(pcie);
		break;
	case eLPWR_PCIPM_L11:
		sky1_l11_pcipm_test(pcie);
		break;
	case eLPWR_PCIPM_L12:
		sky1_l12_pcipm_test(pcie);
		break;
	default:
		dev_info(dev, "Unknown low power status.\n");
		dev_info(dev, "Available commands:\n");
		dev_info(dev, "0 (L0S)\n");
		dev_info(dev, "1 (ASPM L1.0)\n");
		dev_info(dev, "2 (ASPM L1.1)\n");
		dev_info(dev, "3 (ASPM L1.2)\n");
		dev_info(dev, "4 (PCIPM L1.0)\n");
		dev_info(dev, "5 (PCIPM L1.1)\n");
		dev_info(dev, "6 (PCIPM L1.2)\n");
		break;
	}
}

static void sky1_pcie_link_set_speed(struct sky1_pcie *pcie, u32 link_gen)
{
	u32 cap, ctrl2, link_speed;
	void __iomem *reg_base;
	u8 offset;

	reg_base = pcie->reg_base + CDNS_PCIE_RP_BASE;
	offset = cdns_pcie_find_capability(reg_base, PCI_CAP_ID_EXP);
	dev_info(pcie->dev, "PCI_CAP_ID_EXP: 0x%x\n", offset);

	cap = readl(reg_base + offset + PCI_EXP_LNKCAP);
	ctrl2 = readl(reg_base + offset + PCI_EXP_LNKCTL2);
	ctrl2 &= ~PCI_EXP_LNKCTL2_TLS;

	switch (pcie_link_speed[link_gen]) {
	case PCIE_SPEED_2_5GT:
		link_speed = PCI_EXP_LNKCTL2_TLS_2_5GT;
		break;
	case PCIE_SPEED_5_0GT:
		link_speed = PCI_EXP_LNKCTL2_TLS_5_0GT;
		break;
	case PCIE_SPEED_8_0GT:
		link_speed = PCI_EXP_LNKCTL2_TLS_8_0GT;
		break;
	case PCIE_SPEED_16_0GT:
		link_speed = PCI_EXP_LNKCTL2_TLS_16_0GT;
		break;
	default:
		/* Use hardware capability */
		link_speed = FIELD_GET(PCI_EXP_LNKCAP_SLS, cap);
		ctrl2 &= ~PCI_EXP_LNKCTL2_HASD;
		break;
	}

	writel(ctrl2 | link_speed, reg_base + offset + PCI_EXP_LNKCTL2);

	cap &= ~((u32)PCI_EXP_LNKCAP_SLS);
	writel(cap | link_speed, reg_base + offset + PCI_EXP_LNKCAP);
}

static void sky1_pcie_set_speed_retrain(struct sky1_pcie *pcie)
{
	void __iomem *reg_base;
	u32 retries;
	u32 val = 0;
	u8 offset;
	char *speed;

	reg_base = pcie->reg_base + CDNS_PCIE_RP_BASE;
	offset = cdns_pcie_find_capability(reg_base, PCI_CAP_ID_EXP);
	val = readl(reg_base + offset + PCI_EXP_LNKCTL);
	val |= PCI_EXP_LNKCTL_RL;
	writel(val, reg_base + offset + PCI_EXP_LNKCTL);

	for (retries = 0; retries < LINK_WAIT_MAX_RETRIES; retries++) {
		if (sky1_pcie_link_up(pcie->cdns_pcie)) {
			msleep(20);
			break;
		}
		msleep(100);
	}

	val = readl(reg_base + offset + PCI_EXP_LNKCTL);
	dev_info(pcie->dev, "link sta ctrl is 0x%08x\n", val);
	val = val >> 16;
	if ((val & PCI_EXP_LNKSTA_DLLLA) > 0) {
		dev_info(pcie->dev, "data link layer actived\n");
		switch (val & 0x7) {
		case 1:
			speed = "2.5 GT/sec";
			break;
		case 2:
			speed = "5 GT/sec";
			break;
		case 3:
			speed = "8 GT/sec";
			break;
		case 4:
			speed = "16 GT/sec";
			break;
		default:
			break;
		}
		dev_info(pcie->dev, "current link speed is %s\n", speed);
	}
}

static int sky1_link_status_show(struct seq_file *s, void *v)
{
	struct sky1_pcie *pcie = s->private;
	void __iomem *status_addr;
	u32 val, ltssm;

	status_addr = pcie->status_base + STATUS_REG(0);
	val = sky1_pcie_get_bits32(status_addr, ~0);
	ltssm = (val >> PCIE_LTSSM_STATUS_SHIFT) & 0x3f;
	dev_info(pcie->dev, "sky1 status = 0x%08x ltssm = %d\n", val, ltssm);

	sky1_link_l1sub_sta(pcie, val);
	seq_printf(s, "sky1 link status is 6'd%d %s\n", ltssm,
		   sky1_ltssm_sts_name(ltssm));
	return 0;
}

static int sky1_pcie_link_status_open(struct inode *inode, struct file *file)
{
	return single_open(file, sky1_link_status_show, inode->i_private);
}

static const struct file_operations sky1_pcie_link_status_ops = {
	.owner = THIS_MODULE,
	.open = sky1_pcie_link_status_open,
	.read = seq_read,
};

static int sky1_set_speed_show(struct seq_file *s, void *v)
{
	seq_puts(s, "set speed\n");
	return 0;
}

static int sky1_pcie_set_speed_open(struct inode *inode, struct file *file)
{
	return single_open(file, sky1_set_speed_show, inode->i_private);
}

static ssize_t sky1_pcie_set_speed_write(struct file *file,
					 const char __user *user_buf,
					 size_t count, loff_t *ppos)
{
	struct seq_file *s = file->private_data;
	struct sky1_pcie *pcie = s->private;
	int ret;
	int speed;

	ret = kstrtoint_from_user(user_buf, count, 0, &speed);
	if (!ret) {
		dev_info(pcie->dev, "speed = %d\n", speed);
		if ((speed <= 0) || (speed > 4)) {
			dev_info(pcie->dev, "please set speed 1,2,3,4\n");
			return -EINVAL;
		}

		sky1_pcie_link_set_speed(pcie, speed);
		sky1_pcie_set_speed_retrain(pcie);
	} else {
		dev_err(pcie->dev, "Unknown set speed.\n");
		dev_err(pcie->dev, "Available commands:\n");
		dev_err(pcie->dev, "<speed>\n");
		dev_err(pcie->dev, "speed 1,2,3,4: (set speed test)\n");
		return ret;
	}
	return count;
}

static const struct file_operations sky1_pcie_set_speed_ops = {
	.owner = THIS_MODULE,
	.open = sky1_pcie_set_speed_open,
	.read = seq_read,
	.write = sky1_pcie_set_speed_write,
};

static int sky1_retraining_show(struct seq_file *s, void *v)
{
	seq_puts(s, "retraining\n");
	return 0;
}

static int sky1_pcie_retraining_open(struct inode *inode, struct file *file)
{
	return single_open(file, sky1_retraining_show, inode->i_private);
}

static ssize_t sky1_pcie_retraining_write(struct file *file,
					  const char __user *user_buf,
					  size_t count, loff_t *ppos)
{
	struct seq_file *s = file->private_data;
	struct sky1_pcie *pcie = s->private;
	int ret;
	int val;

	ret = kstrtoint_from_user(user_buf, count, 0, &val);
	if (!ret) {
		sky1_pcie_set_speed_retrain(pcie);
	} else {
		dev_err(pcie->dev, "Unknown retraining.\n");
		dev_err(pcie->dev, "Available commands:\n");
		dev_err(pcie->dev, "<retraining> > 1\n");
		return ret;
	}
	return count;
}

static const struct file_operations sky1_pcie_retraining_ops = {
	.owner = THIS_MODULE,
	.open = sky1_pcie_retraining_open,
	.read = seq_read,
	.write = sky1_pcie_retraining_write,
};

static int sky1_compliance_show(struct seq_file *s, void *v)
{
	seq_puts(s, "compliance\n");
	return 0;
}

static int sky1_pcie_compliance_open(struct inode *inode, struct file *file)
{
	return single_open(file, sky1_compliance_show, inode->i_private);
}

static ssize_t sky1_pcie_compliance_write(struct file *file,
					  const char __user *user_buf,
					  size_t count, loff_t *ppos)
{
	struct seq_file *s = file->private_data;
	struct sky1_pcie *pcie = s->private;
	void __iomem *reg_base;
	u8 offset;
	int ret;
	int type;
	u32 val = 0;

	ret = kstrtoint_from_user(user_buf, count, 0, &type);
	if (!ret) {
		dev_info(pcie->dev, "type: %d\n", type);
		reg_base = pcie->reg_base + CDNS_PCIE_RP_BASE;
		offset = cdns_pcie_find_capability(reg_base, PCI_CAP_ID_EXP);
		dev_info(pcie->dev, "PCI_CAP_ID_EXP: 0x%x\n", offset);
		val = readl(reg_base + offset + PCI_EXP_LNKCTL2);
		if (!!type)
			val |= PCI_EXP_LNKCTL2_ENTER_COMP;
		else
			val &= ~PCI_EXP_LNKCTL2_ENTER_COMP;
		writel(val, reg_base + offset + PCI_EXP_LNKCTL2);
		val = readl(reg_base + offset + PCI_EXP_LNKCTL2);

		if (!!type) {
			// retraining
			val = readl(reg_base + offset + PCI_EXP_LNKCTL);
			val |= PCI_EXP_LNKCTL_RL;
			writel(val, reg_base + offset + PCI_EXP_LNKCTL);
			usleep_range(1000, 1001);
			// retry ltssm
			/* sky1_pcie_set_link_training_en(pcie, false);
			 * usleep_range(1000, 1001);
			 * sky1_pcie_set_link_training_en(pcie, true);
			 */
		}

		// hot reset
		val = readl(reg_base + PCI_INTERRUPT_LINE);
		val |= (PCI_BRIDGE_CTL_BUS_RESET << 16);
		writel(val, reg_base + PCI_INTERRUPT_LINE);
		usleep_range(2000, 2001);
		val &= ~(PCI_BRIDGE_CTL_BUS_RESET << 16);
		writel(val, reg_base + PCI_INTERRUPT_LINE);
	} else {
		dev_err(pcie->dev, "Unknown compliance type.\n");
		dev_err(pcie->dev, "Available commands:\n");
		dev_err(pcie->dev, "<type>\n");
		dev_err(pcie->dev, "type 0/1: (exit/enter compliance test)\n");
		return ret;
	}
	return count;
}

static const struct file_operations sky1_pcie_compliance_ops = {
	.owner = THIS_MODULE,
	.open = sky1_pcie_compliance_open,
	.read = seq_read,
	.write = sky1_pcie_compliance_write,
};

static int sky1_msi_msix_show(struct seq_file *s, void *v)
{
	seq_puts(s, "msi_msix\n");
	return 0;
}

static int sky1_pcie_msi_msix_open(struct inode *inode, struct file *file)
{
	return single_open(file, sky1_msi_msix_show, inode->i_private);
}

static ssize_t sky1_pcie_msi_msix_write(struct file *file,
					const char __user *user_buf,
					size_t count, loff_t *ppos)
{
	struct seq_file *s = file->private_data;
	struct sky1_pcie *pcie = s->private;
	int ret;
	int irq;

	ret = kstrtoint_from_user(user_buf, count, 0, &irq);
	if (!ret) {
		dev_info(pcie->dev, "irq: %d\n", irq);
		sky1_pcie_msi_msix_test(pcie, irq);
	} else {
		dev_err(pcie->dev, "Unknown msi_msix irq.\n");
		dev_err(pcie->dev, "Available commands:\n");
		dev_err(pcie->dev, "<irq>\n");
		dev_err(pcie->dev, "irq: (msi or msix irq number)\n");
		return ret;
	}
	return count;
}

static const struct file_operations sky1_pcie_msi_msix_ops = {
	.owner = THIS_MODULE,
	.open = sky1_pcie_msi_msix_open,
	.read = seq_read,
	.write = sky1_pcie_msi_msix_write,
};

static int sky1_aer_show(struct seq_file *s, void *v)
{
	struct sky1_pcie *pcie = s->private;
	struct sky1_aer_stats *aer_stats = &pcie->aer_stats;

	seq_printf(s, "rp_total_cor_errs: %lld\n",
		   aer_stats->rp_total_cor_errs);
	seq_printf(s, "rp_total_fatal_errs: %lld\n",
		   aer_stats->rp_total_fatal_errs);
	seq_printf(s, "rp_total_nonfatal_errs: %lld\n",
		   aer_stats->rp_total_nonfatal_errs);
	seq_printf(s, "ep_total_cor_errs: %lld\n",
		   aer_stats->ep_total_cor_errs);
	seq_printf(s, "ep_total_fatal_errs: %lld\n",
		   aer_stats->ep_total_fatal_errs);
	seq_printf(s, "ep_total_nonfatal_errs: %lld\n",
		   aer_stats->ep_total_nonfatal_errs);

	return 0;
}

static int sky1_pcie_aer_open(struct inode *inode, struct file *file)
{
	return single_open(file, sky1_aer_show, inode->i_private);
}

static ssize_t sky1_pcie_aer_write(struct file *file,
				   const char __user *user_buf,
				   size_t count, loff_t *ppos)
{
	return count;
}

static const struct file_operations sky1_pcie_aer_ops = {
	.owner = THIS_MODULE,
	.open = sky1_pcie_aer_open,
	.read = seq_read,
	.write = sky1_pcie_aer_write,
};

static int sky1_lpwr_test_show(struct seq_file *s, void *v)
{
	seq_puts(s, "lpwr_test\n");
	return 0;
}

static int sky1_pcie_lpwr_test_open(struct inode *inode, struct file *file)
{
	return single_open(file, sky1_lpwr_test_show, inode->i_private);
}

static ssize_t sky1_pcie_lpwr_test_write(struct file *file,
					 const char __user *user_buf,
					 size_t count, loff_t *ppos)
{
	struct seq_file *s = file->private_data;
	struct sky1_pcie *pcie = s->private;
	int ret;
	int type;

	ret = kstrtoint_from_user(user_buf, count, 0, &type);
	if (!ret) {
		dev_info(pcie->dev, "lpwr type: %d\n", type);
		sky1_pcie_lpwr_test(pcie, type);
	} else {
		dev_err(pcie->dev, "Unknown lpwr_test.\n");
		dev_err(pcie->dev, "Available commands:\n");
		dev_err(pcie->dev, "<lpwr_test> > 1\n");
		return ret;
	}
	return count;
}

static const struct file_operations sky1_pcie_lpwr_test_ops = {
	.owner = THIS_MODULE,
	.open = sky1_pcie_lpwr_test_open,
	.read = seq_read,
	.write = sky1_pcie_lpwr_test_write,
};

struct dentry *pcie_debug_root = NULL;
EXPORT_SYMBOL_GPL(pcie_debug_root);
static DEFINE_MUTEX(debugfs_mutex);
static int debugfs_count;

void sky1_pcie_debugfs_init(struct sky1_pcie *pcie)
{
	mutex_lock(&debugfs_mutex);
	if (!pcie_debug_root)
		pcie_debug_root = debugfs_create_dir("pcie", NULL);
	++debugfs_count;
	mutex_unlock(&debugfs_mutex);
	pcie->debugfs =
		debugfs_create_dir(dev_name(pcie->dev), pcie_debug_root);
	if (!pcie->debugfs)
		return;

	debugfs_create_file("link_status", 0644, pcie->debugfs, pcie,
			    &sky1_pcie_link_status_ops);
	debugfs_create_file("set_speed", 0644, pcie->debugfs, pcie,
			    &sky1_pcie_set_speed_ops);
	debugfs_create_file("retraining", 0644, pcie->debugfs, pcie,
			    &sky1_pcie_retraining_ops);
	debugfs_create_file("compliance", 0644, pcie->debugfs, pcie,
			    &sky1_pcie_compliance_ops);
	debugfs_create_file("msi_msix", 0644, pcie->debugfs, pcie,
			    &sky1_pcie_msi_msix_ops);
	debugfs_create_file("aer", 0644, pcie->debugfs, pcie,
			&sky1_pcie_aer_ops);
	debugfs_create_file("lpwr_test", 0644, pcie->debugfs, pcie,
			    &sky1_pcie_lpwr_test_ops);
}

void sky1_pcie_debugfs_exit(struct sky1_pcie *pcie)
{
	debugfs_remove_recursive(pcie->debugfs);
	pcie->debugfs = NULL;
	mutex_lock(&debugfs_mutex);
	if (debugfs_count == 1) {
		debugfs_remove(pcie_debug_root);
		pcie_debug_root = NULL;
	}
	--debugfs_count;
	mutex_unlock(&debugfs_mutex);
}
#else
void sky1_pcie_debugfs_init(struct sky1_pcie *pcie)
{
}
void sky1_pcie_debugfs_exit(struct sky1_pcie *pcie)
{
}
#endif
