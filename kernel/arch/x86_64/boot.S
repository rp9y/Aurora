.intel_syntax noprefix
.extern kmain

.set MULTIBOOT2_MAGIC, 0xE85250D6
.set MULTIBOOT2_ARCH,  0x00000000
.set MULTIBOOT2_LEN,   multiboot2_header_end - multiboot2_header
.set MULTIBOOT2_CSUM,  -(MULTIBOOT2_MAGIC + MULTIBOOT2_ARCH + MULTIBOOT2_LEN)

.section .multiboot2, "a"
.align 8
multiboot2_header:
    .long MULTIBOOT2_MAGIC
    .long MULTIBOOT2_ARCH
    .long MULTIBOOT2_LEN
    .long MULTIBOOT2_CSUM

    .short 0
    .short 0
    .long 8
multiboot2_header_end:

.section .text, "ax"
.code32
.global start32
.type start32, @function
start32:
    mov [mb_magic], eax
    mov [mb_info], ebx

    cli
    mov esp, offset boot_stack_top
    mov ebp, esp

    call check_cpuid
    call check_long_mode
    call setup_page_tables

    lgdt [gdt64_descriptor]

    mov eax, cr4
    or eax, (1 << 5)
    mov cr4, eax

    mov eax, offset pml4_table
    mov cr3, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, (1 << 8)
    wrmsr

    mov eax, cr0
    or eax, 0x80000001
    mov cr0, eax

    push 0x08
    push offset long_mode_entry
    retf

check_cpuid:
    pushfd
    pop eax
    mov ecx, eax
    xor eax, (1 << 21)
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    xor eax, ecx
    test eax, (1 << 21)
    jz boot_hang
    ret

check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb boot_hang

    mov eax, 0x80000001
    cpuid
    test edx, (1 << 29)
    jz boot_hang
    ret

setup_page_tables:
    mov edi, offset pml4_table
    mov ecx, (4096 * 6) / 4
    xor eax, eax
    rep stosd

    mov eax, offset pdpt_table
    or eax, 0x3
    mov [pml4_table], eax
    mov dword ptr [pml4_table + 4], 0

    mov ecx, 4
    mov edi, offset pdpt_table
    mov ebx, offset pd_tables
.fill_pdpt:
    mov eax, ebx
    or eax, 0x3
    mov [edi], eax
    mov dword ptr [edi + 4], 0
    add edi, 8
    add ebx, 4096
    loop .fill_pdpt

    mov ecx, 2048
    mov edi, offset pd_tables
    xor ebx, ebx
.fill_pd:
    mov eax, ebx
    or eax, 0x83
    mov [edi], eax
    mov dword ptr [edi + 4], 0
    add ebx, 0x200000
    add edi, 8
    loop .fill_pd
    ret

boot_hang:
    cli
.halt:
    hlt
    jmp .halt

.code64
.type long_mode_entry, @function
long_mode_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov rsp, offset boot_stack_top
    mov rbp, rsp

    mov edi, dword ptr [mb_magic]
    mov esi, dword ptr [mb_info]
    call kmain

.hang64:
    cli
    hlt
    jmp .hang64

.align 8
gdt64:
    .quad 0x0000000000000000
    .quad 0x00AF9A000000FFFF
    .quad 0x00AF92000000FFFF
gdt64_end:

gdt64_descriptor:
    .word gdt64_end - gdt64 - 1
    .long gdt64

.section .bss, "aw", @nobits
.align 16
boot_stack:
    .space 16384
boot_stack_top:

.align 4096
pml4_table:
    .space 4096

.align 4096
pdpt_table:
    .space 4096

.align 4096
pd_tables:
    .space 16384

.align 4
mb_magic:
    .long 0
mb_info:
    .long 0
