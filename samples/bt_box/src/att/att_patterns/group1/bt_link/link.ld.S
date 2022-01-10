ENTRY(_crt0_start)

#include "att_export_apis.h"

#define CODE_LENGTH  0x1800
#define DATA_LENGTH  0x0800

MEMORY
{
	pattern_ibank : ORIGIN = att_speech_codec_end, LENGTH = CODE_LENGTH
	pattern_data  : ORIGIN = att_speech_codec_end + CODE_LENGTH, LENGTH = DATA_LENGTH
	flash	      : ORIGIN = 0x00100000, LENGTH = 0x100000
}

SECTIONS
{
	/DISCARD/ :
	{
	   *(.comment)
	   *(.eh_frame)
	   *(.eh_frame_hdr)
	   *(.eh_frame_entry)
	   *(.MIPS.abiflags)
	   *(.MIPS.options)
	   *(.options)
	   *(.pdr)
	   *(.reginfo)
	}

	.text : 
	{
		__core_c_code_start = .;
		KEEP(*crt0.o(.text* .rodata*))
		*(.text*)
		*(.rodata*)
		. = ALIGN(8);
		__core_c_code_end = .;

		__data_ram_start = .;
		*(.data*)
		. = ALIGN(8);
		__data_ram_end = .;
	} >pattern_ibank AT>flash

	.bss :
	{
		__bss_start = .;
		*(.bss*)
		*(COMMON*)
		*(.sbss)
		*(.common)
		*(common)
		. = ALIGN(4);
		__bss_end = .;
	} >pattern_data
}

