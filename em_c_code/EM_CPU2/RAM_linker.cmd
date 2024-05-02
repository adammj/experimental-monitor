
MEMORY
{
PAGE 0 :
//page 0 is program code
   /* BEGIN is used for the "boot to SARAM" bootloader mode   */

	BEGIN           	: origin = 0x000000, length = 0x000002
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

PAGE 1 :
//page 1 is for data

   	BOOT_RSVD       : origin = 0x000002, length = 0x000120     /* Part of M0, BOOT rom will use this for stack */
   	RAMM1_STACK     : origin = 0x000400, length = 0x000400     /* on-chip RAM block M1 */
   	RAMD1           : origin = 0x00B800, length = 0x000800

	//combined LS2 and LS3 for cla_data
	//runs from 0x009000 (length 0x001000)
	RAMLS_CLA_DATA	: origin = 0x009000, length = 0x001000

	//cla and cpu message ram (uselessly small, so unused)
	CLA1_MSG_CLA_TO_CPU  : origin = 0x001480, length = 0x000080	//CLAtoCPU
	CLA1_MSG_CPU_TO_CLA  : origin = 0x001500, length = 0x000080	//CPUtoCLA

	//use GS 8-15 for page1
	//runs from 0x014000 to 0x01BFFF (length 0x008000)
	//setting for the smallest size possible (grow as needed)
   	RAMGS_CPU0_PAGE1		: origin = 0x014000, length = 0x002000
}


SECTIONS
{
	// Allocate program areas:
	codestart	: > BEGIN,		PAGE = 0
   	.cinit		: > RAMGS_CPU0_PAGE0,	PAGE = 0
   	.pinit      : > RAMM0,		PAGE = 0
   	.text       : >>RAMM0 | RAMD0 | RAMLS4 | RAMLS5 | RAMGS_CPU0_PAGE0,	PAGE = 0

   	// Allocate uninitalized data sections:
   	.stack		: > RAMM1_STACK,		PAGE = 1
   	.ebss		: >> RAMD1 | RAMGS_CPU0_PAGE1,	PAGE = 1
   	.esysmem		: >> RAMD1 | RAMGS_CPU0_PAGE1,	PAGE = 1
   	.cio			: >> RAMD1 | RAMGS_CPU0_PAGE1,	PAGE = 1

	// Initalized sections
   	.switch		: > RAMM0,     	PAGE = 0
   	.econst		: >> RAMD1 | RAMGS_CPU0_PAGE1,	PAGE = 1

	.reset           : > RESET,     PAGE = 0, TYPE = DSECT /* not used, */


#ifdef __TI_COMPILER_VERSION__
   #if __TI_COMPILER_VERSION__ >= 15009000
    .TI.ramfunc : {} > RAMM0,      PAGE = 0
   #else
   ramfuncs         : > RAMM0      PAGE = 0   
   #endif
#endif


#ifdef CLA_C
// CLA specific sections

	Cla1Prog		: > RAMLS_CLA_PROG,	PAGE = 0		//must keep the "Cla1Prog" name

	cla_to_cpu_msg_ram  : > CLA1_MSG_CLA_TO_CPU,	PAGE = 1
	cpu_to_cla_msg_ram  : > CLA1_MSG_CPU_TO_CLA,	PAGE = 1

	cla_data		: > RAMLS_CLA_DATA	,	PAGE = 1
	.scratchpad	: > RAMLS_CLA_DATA,	PAGE = 1
	.bss_cla		: > RAMLS_CLA_DATA,	PAGE = 1
	.const_cla	: > RAMLS_CLA_DATA,	PAGE = 1
#endif //CLA_C

}

/*
//===========================================================================
// End of file.
//===========================================================================
*/
