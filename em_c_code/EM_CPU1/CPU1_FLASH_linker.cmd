
MEMORY
{
PAGE 0 :
//page 0 is program code

	// BEGIN is used for the "boot to Flash" bootloader mode
	BEGIN           	: origin = 0x080000, length = 0x000002

	//Cla1Prog - cannot be shared with CPU
	RAMLS0_CLA_PROG	: origin = 0x008000, length = 0x000800

	//combined to fit ramfunc
	RAMLS2345D01		: origin = 0x009000, length = 0x003000

   	// Flash sectors
   	// A,B,C,D,... K,L,M,N are 8k long
   	// E,F,G,H,I,J are 32k long
   	FLASHABCDE      : origin = 0x080002, length = 0x00FFFE	//chunk taken out for BEGIN

	//reset
   	RESET           	: origin = 0x3FFFC0, length = 0x000002

PAGE 1 :
//page 1 is for data

   	BOOT_RSVD       : origin = 0x000002, length = 0x000120     // Part of M0, BOOT rom will use this for stack

	//section taken out for BOOT_RSVD
	RAMM01_STACK     : origin = 0x000130, length = 0x0006D0	//make sure to keep stack_end in misc updated

	//cla_data - can be shared with CPU
	RAMLS1_CLA_DATA		: origin = 0x008800, length = 0x000800

	//misc data
   	RAMGS_CPU1_PAGE1		: origin = 0x00C000, length = 0x000400

   	//heap
   	RAMGS_HEAP			: origin = 0x00C400, length = 0x00FC00
}

SECTIONS
{
	// Allocate program areas:
	codestart           : > BEGIN,       PAGE = 0, ALIGN(4)
   	.cinit              : > FLASHABCDE,		PAGE = 0, ALIGN(4)
   	.pinit              : > FLASHABCDE,		PAGE = 0, ALIGN(4)
   	.text               : >> FLASHABCDE,    PAGE = 0, ALIGN(4)

   	// Allocate uninitalized data sections:
   	.stack              : > RAMM01_STACK,		PAGE = 1
   	.esysmem            : > RAMGS_HEAP,		PAGE = 1
   	.ebss               : >> RAMGS_CPU1_PAGE1 | RAMLS1_CLA_DATA,	PAGE = 1
   	//.cio                : >> RAMGS_CPU1_PAGE1 | RAMLS1_CLA_DATA,	PAGE = 1	    //commented out to find bad calls

	// Initalized sections go in Flash
   	.econst             : >> FLASHABCDE,    PAGE = 0, ALIGN(4)
   	.switch             : > FLASHABCDE,      PAGE = 0, ALIGN(4)	//jump table

   	.reset              : > RESET,     PAGE = 0, TYPE = DSECT /* not used, */

#ifdef __TI_COMPILER_VERSION__
   #if __TI_COMPILER_VERSION__ >= 15009000
    .TI.ramfunc : {} LOAD = FLASHABCDE,
                         RUN = RAMLS2345D01,
                         LOAD_START(_RamfuncsLoadStart),
                         LOAD_SIZE(_RamfuncsLoadSize),
                         LOAD_END(_RamfuncsLoadEnd),
                         RUN_START(_RamfuncsRunStart),
                         RUN_SIZE(_RamfuncsRunSize),
                         RUN_END(_RamfuncsRunEnd),
                         PAGE = 0, ALIGN(4)
	{
    	 //put additional functions in here
     //rts memory functions ~1068 bytes
     //these cause errors if added to ramfunc
     //rts2800_fpu32.lib<array_del.c.obj, array_new.c.obj, delete.c.obj, memcpy.c.obj, memory.c.obj, memset.c.obj, memzero.c.obj, new.c.obj, newhandler.c.obj, vec_newdel.c.obj>(.text)

     //rts 64bit integer functions ~337 bytes
     rts2800_fpu32.lib<ll_aox28.asm.obj, ll_cmp28.asm.obj, ll_div28.asm.obj, ll_mpy28.asm.obj>(.text)

     //rts 32 and 64bit floating function ~1224
     rts2800_fpu32.lib<s_scalbnf.c.obj, s_fabs.c.obj, s_llroundl.c.obj, s_lroundl.c.obj, s_copysignf.c.obj, fs_div28.asm.obj, fs_toullfpu32.asm.obj, ull_tofsfpu32.asm.obj, fd_add28.asm.obj, fd_div28.asm.obj, fd_mpy28.asm.obj, fd_cmp28.asm.obj, ll_tofd28.asm.obj, fd_tofsfpu32.asm.obj, ull_tofd28.asm.obj, fd_tol28.asm.obj, fd_toll28.asm.obj, i_tofd28.asm.obj, fd_toull28.asm.obj, fs_tofdfpu32.asm.obj, l_tofd28.asm.obj, fd_toul28.asm.obj, fd_tou28.asm.obj, u_tofd28.asm.obj, ul_tofd28.asm.obj, fd_sub28.asm.obj, fd_neg28.asm.obj>(.text)
    }
   #else
   ramfuncs            : LOAD = FLASHABCDE,
                         RUN = RAMLS2345D01,
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

	//must keep the "Cla1Prog" name
	Cla1Prog		: LOAD = FLASHABCDE,
                  RUN = RAMLS0_CLA_PROG,
                  LOAD_START(_Cla1funcsLoadStart),
                  LOAD_END(_Cla1funcsLoadEnd),
                  RUN_START(_Cla1funcsRunStart),
                  LOAD_SIZE(_Cla1funcsLoadSize),
                  PAGE = 0, ALIGN(4)

	cla_data		: > RAMLS1_CLA_DATA,	PAGE = 1
	.scratchpad	: > RAMLS1_CLA_DATA,	PAGE = 1
	.bss_cla		: > RAMLS1_CLA_DATA,	PAGE = 1
	.const_cla	:  LOAD = FLASHABCDE,
                   RUN = RAMLS1_CLA_DATA,
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
