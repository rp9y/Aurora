.section .multiboot2,"a",@progbits
.align 8
mb2_header_start:
    .long 0xE85250D6
    .long 0
    .long mb2_header_end - mb2_header_start
    .long -(0xE85250D6 + 0 + (mb2_header_end - mb2_header_start))
    .short 0
    .short 0
    .long 8
mb2_header_end:

.section .text,"ax",@progbits
.code32
.global start
.type start, @function
.extern kmain
start:
    cli
    movl %eax, boot_magic
    movl %ebx, boot_info
    movl $stack_top32, %esp

    xorl %eax, %eax
    movl $pml4_table, %edi
    movl $3072, %ecx
    rep stosl

    movl $pdpt_table, %eax
    orl $0x3, %eax
    movl %eax, pml4_table
    movl $0, pml4_table + 4

    movl $pd_table, %eax
    orl $0x3, %eax
    movl %eax, pdpt_table
    movl $0, pdpt_table + 4

    xorl %ebx, %ebx
    movl $pd_table, %edi
    movl $512, %ecx
1:
    movl %ebx, %eax
    orl $0x83, %eax
    movl %eax, (%edi)
    movl $0, 4(%edi)
    addl $0x200000, %ebx
    addl $8, %edi
    loop 1b

    lgdt gdt64_descriptor

    movl %cr4, %eax
    orl $0x20, %eax
    movl %eax, %cr4

    movl $pml4_table, %eax
    movl %eax, %cr3

    movl $0xC0000080, %ecx
    rdmsr
    orl $0x00000100, %eax
    wrmsr

    movl %cr0, %eax
    orl $0x80000001, %eax
    movl %eax, %cr0

    ljmp $0x08, $long_mode_start

.code64
long_mode_start:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw %ax, %fs
    movw %ax, %gs

    movq $stack_top64, %rsp
    xorq %rbp, %rbp

    movl boot_magic, %edi
    movl boot_info, %esi
    call kmain

2:
    hlt
    jmp 2b

.align 16
gdt64:
    .quad 0x0000000000000000
    .quad 0x00209A0000000000
    .quad 0x0000920000000000
gdt64_end:

gdt64_descriptor:
    .word gdt64_end - gdt64 - 1
    .long gdt64

.section .bss,"aw",@nobits
.align 4096
pml4_table:
    .skip 4096
pdpt_table:
    .skip 4096
pd_table:
    .skip 4096

.align 16
stack_bottom32:
    .skip 4096
stack_top32:

.align 16
stack_bottom64:
    .skip 16384
stack_top64:

.align 4
boot_magic:
    .long 0
boot_info:
    .long 0

.section .note.GNU-stack,"",@progbits
