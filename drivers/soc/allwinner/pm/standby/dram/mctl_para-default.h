#ifndef _MCTL_PARA_H
#define _MCTL_PARA_H

typedef struct __DRAM_PARA {
	/*normal configuration */
	unsigned int dram_clk;
	unsigned int dram_type;	/*dram_type                     DDR2: 2                         DDR3: 3         LPDDR2: 6       LPDDR3: 7       DDR3L: 31 */
	/*unsigned int        lpddr2_type;      //LPDDR2 type           S4:0    S2:1    NVM:2 */
	unsigned int dram_zq;	/*do not need */
	unsigned int dram_odt_en;

	/*control configuration */
	unsigned int dram_para1;
	unsigned int dram_para2;

	/*timing configuration */
	unsigned int dram_mr0;
	unsigned int dram_mr1;
	unsigned int dram_mr2;
	unsigned int dram_mr3;
	unsigned int dram_tpr0;	/*DRAMTMG0 */
	unsigned int dram_tpr1;	/*DRAMTMG1 */
	unsigned int dram_tpr2;	/*DRAMTMG2 */
	unsigned int dram_tpr3;	/*DRAMTMG3 */
	unsigned int dram_tpr4;	/*DRAMTMG4 */
	unsigned int dram_tpr5;	/*DRAMTMG5 */
	unsigned int dram_tpr6;	/*DRAMTMG8 */

	/*reserved for future use */
	unsigned int dram_tpr7;
	unsigned int dram_tpr8;
	unsigned int dram_tpr9;
	unsigned int dram_tpr10;
	unsigned int dram_tpr11;
	unsigned int dram_tpr12;
	unsigned int dram_tpr13;
} __dram_para_t;

#endif	/*_MCTL_HAL_H*/
