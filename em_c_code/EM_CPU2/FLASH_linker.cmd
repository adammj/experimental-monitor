
MEMORY
{
PAGE 0 :  /* Program Memory */
          /* Memory (RAM/FLASH) blocks can be moved to PAGE1 for data allocation */
          /* BEGIN is used for the "boot to Flash" bootloader mode   */

	BEGIN           	: origin = 0x080000, length = 0x000002	//actually loaded into FLASHA
   	RAMM0           	: origin = 0x000122, length = 0x0002DE
   	RAMD0           	: origin = 0x00B000, length = 0x000800

	//combined LS0 and LS1 for Cla1Prog
	//runs from 0x008000 (length 0x001000)
	RAMLS_CLA_PROG	: origin = 0x008000, length = 0x001000

   	RAMLS4			: origin = 0x00A000, length = 0x000800
   	RAMLS5			: origin = 0x00A800, length = 0x000800

	//use GS 0-7 for page0
	//runs from 0x00C000 to 0x013FFF (length 0x008000)
	//setting for the smallest size possible (grow as needed)
   	RAMGS_CPU0_PAGE0			: origin = 0x00C000, length = 0x002000

   	RESET           	: origin = 0x3FFFC0, length = 0x000002

   	// Flash sectors
   	// In total, A-N
   	// A,B,C,D,K,L,M,N are 8k long
   	// E,F,G,H,I,J are 32k long
   	// setting for the smallest size possible, using A-C for CPU0
   	FLASHA_CPU0      : origin = 0x080002, length = 0x001FFD	//chunk taken out for BEGIN
   	FLASHB_CPU0      : origin = 0x082000, length = 0x002000
   	FLASHC_CPU0      : origin = 0x084000, length = 0x002000

	//using M and N for CPU0 CLA PROG and CONST
   	FLASHM_CPU0_CLA_PROG  : origin = 0x0BC000, length = 0x002000


PAGE 1 : /* Data Memory */
         /* Memory (RAM/FLASH) blocks can be moved to PAGE0 for program allocation */

   	BOOT_RSVD       : origin = 0x000002, length = 0x000120     /* Part of M0, BOOT rom will use this for stack */
   	RAMM1_STACK     : origin = 0x000400, length = 0x000400     /* on-chip RAM block M1 */
   	RAMD1           : origin = 0x00B800, length = 0x000800

	//combined LS2 and LS3 for cla_data
	//runs from 0x009000 (length 0x001000)
	RAMLS_CLA_DATA	: origin = 0x009000, length = 0x001000

	//cla and cpu message ram
	CLA1_MSG_CLA_TO_CPU  : origin = 0x001480, length = 0x000080	//CLAtoCPU
	CLA1_MSG_CPU_TO_CLA  : origin = 0x001500, length = 0x000080	//CPUtoCLA

	//use GS 8-15 for page1
	//runs from 0x014000 to 0x01BFFF (length 0x008000)
	//setting for the smallest size possible (grow as needed)
   	RAMGS_CPU0_PAGE1		: origin = 0x014000, length = 0x002000

   	//for cla constants
   	FLASHN_CPU0_CLA_CONST : origin = 0x0BE000, length = 0x002000
}

SECTIONS
{
	// Allocate program areas:
   	codestart           : > BEGIN,       PAGE = 0, ALIGN(4)
   	.cinit              : > FLASHA_CPU0,		PAGE = 0, ALIGN(4)
   	.pinit              : > FLASHA_CPU0,		PAGE = 0, ALIGN(4)
   	.text               : >> FLASHA_CPU0 | FLASHB_CPU0 | FLASHC_CPU0       PAGE = 0, ALIGN(4)

   	// Allocate uninitalized data sections:
   	.stack              : > RAMM1_STACK,		PAGE = 1
   	.ebss               : > RAMGS_CPU0_PAGE1,	PAGE = 1
   	.esysmem            : > RAMGS_CPU0_PAGE1,	PAGE = 1
   	.cio                : > RAMGS_CPU0_PAGE1,	PAGE = 1

   	// Initalized sections go in Flash
   	.econst             : >> FLASHA_CPU0 | FLASHB_CPU0 | FLASHC_CPU0       PAGE = 0, ALIGN(4)
   	.switch             : > FLASHA_CPU0      PAGE = 0, ALIGN(4)

   	.reset              : > RESET,     PAGE = 0, TYPE = DSECT /* not used, */

#ifdef __TI_COMPILER_VERSION__
   #if __TI_COMPILER_VERSION__ >= 15009000
    .TI.ramfunc : {} LOAD = FLASHA_CPU0,
                         RUN = RAMM0,
                         LOAD_START(_RamfuncsLoadStart),
                         LOAD_SIZE(_RamfuncsLoadSize),
                         LOAD_END(_RamfuncsLoadEnd),
                         RUN_START(_RamfuncsRunStart),
                         RUN_SIZE(_RamfuncsRunSize),
                         RUN_END(_RamfuncsRunEnd),
                         PAGE = 0, ALIGN(4)
   #else
   ramfuncs            : LOAD = FLASHA_CPU0,
                         RUN = RAMM0.
                         LOAD_START(_RamfuncsLoadStart),
                         LOAD_SIZE(_RamfuncsLoadSize),
                         LOAD_END(_RamfuncsLoadEnd),
                         RUN_START(_RamfuncsRunStart),
                         RUN_SIZE(_RamfuncsRunSize),
                         RUN_END(_RamfuncsRunEnd),
                         PAGE = 0, ALIGN(4)
   #endif
#endif


#ifdef CLA_C
// CLA specific sections

	Cla1Prog		: LOAD = FLASHM_CPU0_CLA_PROG,
                  RUN = RAMLS_CLA_PROG,
                  LOAD_START(_Cla1funcsLoadStart),
                  LOAD_END(_Cla1funcsLoadEnd),
                  RUN_START(_Cla1funcsRunStart),
                  LOAD_SIZE(_Cla1funcsLoadSize),
                  PAGE = 0, ALIGN(4)

	cla_to_cpu_msg_ram  : > CLA1_MSG_CLA_TO_CPU,	PAGE = 1
	cpu_to_cla_msg_ram  : > CLA1_MSG_CPU_TO_CLA,	PAGE = 1

	cla_data		: > RAMLS_CLA_DATA	,	PAGE = 1
	.scratchpad	: > RAMLS_CLA_DATA,	PAGE = 1
	.bss_cla		: > RAMLS_CLA_DATA,	PAGE = 1
	.const_cla	:  LOAD = FLASHN_CPU0_CLA_CONST,
                   RUN = RAMLS_CLA_DATA,
                   RUN_START(_Cla1ConstRunStart),
                   LOAD_START(_Cla1ConstLoadStart),
                   LOAD_SIZE(_Cla1ConstLoadSize),
                   PAGE = 1
#endif //CLA_C

}

/*
//===========================================================================
// End of file.
//===========================================================================
*/
