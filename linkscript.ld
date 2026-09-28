ENTRY(_start)

__stack_size = 0x400;

FLASH_SIZE_TOTAL = 224K;
FLASH_SIZE_FOR_PARAM = 256;

MEMORY
{
    NON_WAIT_FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 64K
    FLASH (rx) : ORIGIN = ORIGIN(NON_WAIT_FLASH) + LENGTH(NON_WAIT_FLASH), LENGTH = FLASH_SIZE_TOTAL - LENGTH(NON_WAIT_FLASH)
    SRAM (rwx) : ORIGIN = 0x20000000, LENGTH = 10K
}

SECTIONS
{
    .startup : ALIGN(4)
    {
        *(.text.startup)
        *(.text.sysinit)
    } > NON_WAIT_FLASH

    .vectors : ALIGN(4)
    {
        KEEP(*(.vectors))
    } > NON_WAIT_FLASH

    .text : ALIGN(4)
    {
        *(.text)
        *(.text.*)
    } > NON_WAIT_FLASH

	.preinit_array  :
	{
	  PROVIDE_HIDDEN (__preinit_array_start = .);
	  KEEP (*(.preinit_array))
	  PROVIDE_HIDDEN (__preinit_array_end = .);
	} >NON_WAIT_FLASH
	
	.init_array     :
	{
	  PROVIDE_HIDDEN (__init_array_start = .);
	  KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
	  KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
	  PROVIDE_HIDDEN (__init_array_end = .);
	} >NON_WAIT_FLASH
	
	.fini_array     :
	{
	  PROVIDE_HIDDEN (__fini_array_start = .);
	  KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
	  KEEP (*(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
	  PROVIDE_HIDDEN (__fini_array_end = .);
	} >NON_WAIT_FLASH
	
	.ctors          :
	{
	  KEEP (*crtbegin.o(.ctors))
	  KEEP (*crtbegin?.o(.ctors))
	  KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
	  KEEP (*(SORT(.ctors.*)))
	  KEEP (*(.ctors))
	} >NON_WAIT_FLASH
	
	.dtors          :
	{
	  KEEP (*crtbegin.o(.dtors))
	  KEEP (*crtbegin?.o(.dtors))
	  KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
	  KEEP (*(SORT(.dtors.*)))
	  KEEP (*(.dtors))
	} >NON_WAIT_FLASH

    .rodata : ALIGN(4)
    {
        *(.rodata)
        *(.rodata.*)
    } > NON_WAIT_FLASH

    .data : ALIGN(4) SUBALIGN(4)
    {
        __data_lma__ = LOADADDR(.data);
        __data_start__ = .;
        *(.gnu.linkonce.r.*)
    	*(.data .data.*)
    	*(.gnu.linkonce.d.*)
		. = ALIGN(8);
    	*(.sdata .sdata.*)
		*(.sdata2.*)
    	*(.gnu.linkonce.s.*)
    	. = ALIGN(8);
    	*(.srodata.cst16)
    	*(.srodata.cst8)
    	*(.srodata.cst4)
    	*(.srodata.cst2)
    	*(.srodata .srodata.*)
        __data_end__ = .;
    } > SRAM AT> FLASH

    .bss (NOLOAD) : ALIGN(4) SUBALIGN(4)
    {
        __bss_start__ = .;
  	    *(.sbss*)
        *(.gnu.linkonce.sb.*)
		*(.bss*)
     	*(.gnu.linkonce.b.*)		
		*(COMMON*)
        __bss_end__ = .;
    } > SRAM

    .stack ORIGIN(SRAM) + LENGTH(SRAM) -  __stack_size (NOLOAD): ALIGN(4)
    {
        __stack_start__ = .;
        . = . + __stack_size;
        __stack_end__ = .;
    } > SRAM

	.param_store_sec ORIGIN(FLASH) + LENGTH(FLASH) - FLASH_SIZE_FOR_PARAM : ALIGN(256)
	{
		__param_stored_base = .;
		ASSERT(__param_stored_base < ORIGIN(FLASH) + LENGTH(FLASH), "Param store out of flash");
	} > FLASH

    __heap_start__ = ADDR(.bss) + SIZEOF(.bss);
    __heap_end__ = ADDR(.stack);
    __global_pointer$ = ADDR(.data) + 0x800;
}