/***************************************************************************
// switch2normal
// This is called when the Secure world wishes to
// move to the Normal world.
// the procedure as follow: secure -> monitor_mode -> non-secure.
// 
***************************************************************************/

#include "./../../../pm_assembler_i.h"
	.text
	.arm
	.globl switch2normal
	
	//PRESERVE8
	
	//AREA  Monitor_Code, CODE, ALIGN=5, READONLY

// Defines used in the code
#ifdef CONFIG_ARCH_SUN8IW6P1
#define CCI_SECURE_ACCESS_REG (0x01790008)
#elif defined(CONFIG_ARCH_SUN9IW1P1)
#define CCI_SECURE_ACCESS_REG (0x01c90008)
#endif

#define	Mode_MON	(0x16)
#define	Mode_SVC        (0x13)
#define	NS_BIT          (0x1)

// SCR Bit Masks
// Bit masks for SCR bit settings (CP15-C1 SCR register)
// For details see ARM ARM Security Extensions Supplement (DDI 0309B)
#define SCR_BIT_0_NS                	(0x01)
#define SCR_BIT_1_IRQ_INTO_MON      	(0x02)
#define SCR_BIT_2_FIQ_INTO_MON      	(0x04)
#define SCR_BIT_3_EA_INTO_MON       	(0x08)
#define SCR_BIT_4_FW_MODIFY_ENABLE  	(0x10)
#define SCR_BIT_5_AW_MODIFY_ENABLE  	(0x20)
#define SCR_BIT_4_FW_MODIFY_DISABLE 	(0x00)
#define SCR_BIT_5_AW_MODIFY_DISABLE 	(0x00)

// Bit masks for NSACR bit setting
#define NSACR_ACTLR_ENABLE     	(0x1<<18)
#define NSACR_L2ECTLR_ENABLE   	(0x1<<17)
#define NS_ACCESS_CP10_ENABLE	(0x1<<10)
#define NS_ACCESS_CP11_ENABLE	(0x1<<11)

// config SCR: 		secure configuration register
// affect: 		CPSR, mode entry, secure or non-secure state.
// config: 		bit4,bit5: FW, AW; F bit Writable, A bit Writable -> 1; 
//			can be modified in any security state.
// 			in normal world, irq is take care by normal os
// 			fiq is take care by FIQ mode.?
#define SCR_NS                   (SCR_BIT_0_NS                | \
                                  SCR_BIT_4_FW_MODIFY_ENABLE  | \
                                  SCR_BIT_5_AW_MODIFY_ENABLE)

// config nsacr: 	non-secure Access Control Register
// affect: 		access to ACTLR.SMP, auxiliary  Ctrol Register.
//			access to L2ECTLR.AXI, L2 Extended Control Register.
//			access to CPACR.ASEDIS, Coprocessor Access Control Register    
// config:		bit17, bit18: NS_L2ERR, NS_SMP -> 1; to allow non-secure write.
// config: 		bit10, bit11: NS_ACCESS_CP10_ENABLE, NS_ACCESS_CP11_ENABLE ->1; to allow non-secure write. 
    
#define NSACR			(NSACR_ACTLR_ENABLE          | \
				 NSACR_L2ECTLR_ENABLE        | \
				 NS_ACCESS_CP10_ENABLE       | \
				 NS_ACCESS_CP11_ENABLE)

// ------------------------------------------------------------
// switch2normal
//
// This is called when the Secure world wishes to
// move to the Normal world.
// On entry:
//   Must be in secure state.
//   r0 = Normal World "Reset" entry.
// ------------------------------------------------------------

switch2normal:
    //config banked gic
    //ldr r4, =0x01c41080
    //ldr r5, =0xffffffff
    //str r5,[r4]

    ////config gic priority.
    //ldr r4, =0x01c42004
    //ldr r5, =0xf8
    //str r5,[r4]
    //
    ////config gic running priority.
    //ldr r4, =0x01c42014
    //ldr r5, =0xd0
    //str r5,[r4]
    
    //config TZPC, make sure rtc&alarm region is non-secure.
    //ldr r4, =0x01c23408
    //ldr r5, =0x2
    //str r5,[r4]
    
    // config cci
    ldr     r4, =CCI_SECURE_ACCESS_REG
    ldr     r5, [r4]
    mov	    r5, #1
    str     r5, [r4]

    // --------------------
    // config monitor vector
    //=====================       
     MCR  p15, 0, r1, c12, c0, 1
      
    // --------------------
    // config NSACR.  
    // --------------------
      MRC     p15, 0, r4, c1, c1, 2        	// Read 
      ldr 	r2, =NSACR			//
      ORR	r4, r2				//r2 can be a 32-bit -value
      MCR     p15, 0, r4, c1, c1, 2        	// Write
      
    // Switch to Monitor mode
    // -----------------------
    CPS     #Mode_MON                   // Move to Monitor mode after saving Secure state
    
    // Set up execption return information
    // ------------------------------------
    MOV		lr, r0			// backup normal world entry.
    MSR     spsr_cxsf, #Mode_SVC         // Set SPSR to be SVC mode
    
    // -----------------------
    // 	Switch to Normal world
    // -----------------------
    MRC     p15, 0, r4, c1, c1, 0        // Read Secure Configuration Register data
    ORR     r4, #SCR_NS                  // Set NS bit
    MCR     p15, 0, r4, c1, c1, 0        // Write Secure Configuration Register data
   

    //-----------------------
    CPS     #Mode_SVC
    LDR     R0, =0x0FFFFFF1
    .arch_extension sec
    smc	#0

    // Clear general purpose registers
    // --------------------------------
    MOV     r0,  #0
    MOV     r1,  #0
    MOV     r2,  #0
    MOV     r3,  #0
    MOV     r4,  #0
    MOV     r5,  #0
    MOV     r6,  #0
    MOV     r7,  #0
    MOV     r8,  #0
    MOV     r9,  #0
    MOV     r10, #0
    MOV     r11, #0
    MOV     r12, #0
    // Clear local monitor
    // --------------------
    CLREX                               // Not strictly required in here, as not using LDREX/STREX
                                        // However, architecturally should execute CLREX on a context switch
                                        
    //return from exception: pc=lr, cpsr=spsr
    MOVS    pc, lr
    
    .end
