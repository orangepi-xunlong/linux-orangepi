/* SPDX-License-Identifier: GPL-2.0*/

#include "rawnand_readretry.h"
#include "../../aw_nand_type.h"
#include "rawnand_chip.h"
#include "rawnand_ids.h"

struct rawnand_readretry rawnand_readretry[] = {
    {
	// index 0
	.nand_readretry_op_init = generic_special_init,
	.nand_readretry_op_exit = generic_special_exit,
    },
    {
	// index 1
	.nand_readretry_op_init = hynix16nm_special_init,
	.nand_readretry_op_exit = hynix16nm_special_exit,
    },
    {
	// index 2
	.nand_readretry_op_init = hynix20nm_special_init,
	.nand_readretry_op_exit = hynix20nm_special_exit,
    },
    {
	// index 3
	.nand_readretry_op_init = hynix26nm_special_init,
	.nand_readretry_op_exit = hynix26nm_special_exit,
    },
    {
	// index 4
	.nand_readretry_op_init = micron_special_init,
	.nand_readretry_op_exit = micron_special_exit,
    },
    {
	// index 5
	.nand_readretry_op_init = samsung_special_init,
	.nand_readretry_op_exit = samsung_special_exit,
    },
    {
	// index 6
	.nand_readretry_op_init = toshiba_special_init,
	.nand_readretry_op_exit = toshiba_special_exit,
    },
    {
	// index 7
	.nand_readretry_op_init = sandisk_special_init,
	.nand_readretry_op_exit = sandisk_special_exit,
    },
    {
	// index 8
	.nand_readretry_op_init = sandisk_A19_special_init,
	.nand_readretry_op_exit = sandisk_A19_special_exit,
    },
};
