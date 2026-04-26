#include <aurora/boot/boot_info.h>
#include <aurora/core/memory/init.h>
#include <aurora/core/printk.h>
#include <aurora/mm/kheap.h>
#include <aurora/mm/paging.h>
#include <aurora/mm/pmm.h>
#include <aurora/mm/vmm.h>

void core_memory_init(uintptr_t multiboot_info_addr, uintptr_t kernel_start, uintptr_t kernel_end) {
    boot_info_init(multiboot_info_addr);
    if (multiboot_info_addr != 0U) {
        pmm_init(multiboot_info_addr, kernel_start, kernel_end);
    } else {
        pmm_init_fallback(0x00200000ULL, 0x20000000ULL, kernel_start, kernel_end);
    }
    paging_init(kernel_start, kernel_end);
    vmm_init();
    kheap_init();
}

void core_memory_log_stats(void) {
    printk(
        "[aurora] memory pages free=%u total=%u heap=%u/%u vmm=%u boot_size=%u regions=%u\n",
        pmm_free_pages(),
        pmm_total_pages(),
        kheap_bytes_used(),
        kheap_bytes_total(),
        vmm_bytes_reserved(),
        (uint64_t)boot_info_total_size(),
        (uint64_t)boot_info_memory_region_count()
    );
}
