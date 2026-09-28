.section .text.startup, "ax", @progbits
.option push
.option norelax
.global _start
_start:
    j   sys_init
.option pop

.global __init_data_section
.global __init_bss_section
.global sys_init

.section .text.sysinit, "ax", @progbits
sys_init:
.option push
.option norelax
la gp, __global_pointer$
.option pop
    la  sp, __stack_end__
    jal __init_data_section
    jal __init_bss_section
    jal SystemInit
    jal SystemCoreClockUpdate

    li t0, 0x1f
    csrw 0xbc0, t0
	li t0, 0x3
	csrw 0x804, t0
   	li t0, 0x88           
   	csrw mstatus, t0
 	la t0, _vector_base
    ori t0, t0, 3           
	csrw mtvec, t0

    la  t0, main
    csrw    mepc, t0
    mret