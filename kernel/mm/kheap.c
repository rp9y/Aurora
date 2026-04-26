#include <aurora/drivers/serial.h>
#include <aurora/lib/string.h>
#include <aurora/mm/kheap.h>
#include <aurora/mm/paging.h>
#include <aurora/mm/pmm.h>

enum {
    KHEAP_PAGE_SIZE = 4096ULL,
    KHEAP_ALIGNMENT = 16ULL,
    KHEAP_BASE = 0xFFFFC10000000000ULL,
    KHEAP_INITIAL_PAGES = 64ULL,
    KHEAP_MAX_PAGES = 16384ULL,
    KHEAP_BLOCK_MAGIC = 0xA8524F5241484B31ULL
};

typedef struct heap_block {
    uint64_t magic;
    uint64_t size;
    bool free;
    uint8_t reserved[7];
    struct heap_block *prev;
    struct heap_block *next;
} heap_block_t;

static heap_block_t *g_head = (heap_block_t *)0;
static heap_block_t *g_tail = (heap_block_t *)0;

static uint64_t g_heap_capacity = 0ULL;
static uint64_t g_heap_used = 0ULL;
static bool g_heap_ready = false;

static uint64_t align_up_u64(uint64_t value, uint64_t alignment) {
    const uint64_t mask = alignment - 1ULL;
    return (value + mask) & ~mask;
}

static bool kheap_is_contiguous(heap_block_t *left, heap_block_t *right) {
    if (left == (heap_block_t *)0 || right == (heap_block_t *)0) {
        return false;
    }

    const uintptr_t left_end = (uintptr_t)left + sizeof(heap_block_t) + (uintptr_t)left->size;
    return left_end == (uintptr_t)right;
}

static void kheap_split_block(heap_block_t *block, uint64_t requested_size) {
    const uint64_t aligned_size = align_up_u64(requested_size, KHEAP_ALIGNMENT);

    if (block->size <= aligned_size + sizeof(heap_block_t) + KHEAP_ALIGNMENT) {
        return;
    }

    uint8_t *new_ptr = (uint8_t *)(uintptr_t)block + sizeof(heap_block_t) + aligned_size;
    heap_block_t *new_block = (heap_block_t *)(uintptr_t)new_ptr;

    new_block->magic = KHEAP_BLOCK_MAGIC;
    new_block->size = block->size - aligned_size - sizeof(heap_block_t);
    new_block->free = true;
    aurora_memset(new_block->reserved, 0, sizeof(new_block->reserved));

    new_block->prev = block;
    new_block->next = block->next;
    if (new_block->next != (heap_block_t *)0) {
        new_block->next->prev = new_block;
    } else {
        g_tail = new_block;
    }

    block->next = new_block;
    block->size = aligned_size;
}

static heap_block_t *kheap_coalesce_forward(heap_block_t *block) {
    while (block->next != (heap_block_t *)0 && block->next->free && kheap_is_contiguous(block, block->next)) {
        heap_block_t *next = block->next;
        block->size += sizeof(heap_block_t) + next->size;
        block->next = next->next;
        if (block->next != (heap_block_t *)0) {
            block->next->prev = block;
        } else {
            g_tail = block;
        }
    }
    return block;
}

static heap_block_t *kheap_coalesce(heap_block_t *block) {
    block = kheap_coalesce_forward(block);
    if (block->prev != (heap_block_t *)0 && block->prev->free && kheap_is_contiguous(block->prev, block)) {
        block = block->prev;
        block = kheap_coalesce_forward(block);
    }
    return block;
}

static heap_block_t *kheap_find_free_block(uint64_t aligned_size) {
    heap_block_t *cursor = g_head;
    while (cursor != (heap_block_t *)0) {
        if (cursor->magic == KHEAP_BLOCK_MAGIC && cursor->free && cursor->size >= aligned_size) {
            return cursor;
        }
        cursor = cursor->next;
    }
    return (heap_block_t *)0;
}

static bool kheap_map_additional_pages(uint64_t pages) {
    if (pages == 0ULL) {
        return true;
    }

    const uint64_t current_pages = g_heap_capacity / KHEAP_PAGE_SIZE;
    if (current_pages + pages > KHEAP_MAX_PAGES) {
        return false;
    }

    const uint64_t old_capacity = g_heap_capacity;
    const uint64_t map_start = KHEAP_BASE + old_capacity;
    uint64_t mapped = 0ULL;

    for (; mapped < pages; ++mapped) {
        const uint64_t phys = pmm_alloc_page();
        if (phys == 0ULL) {
            break;
        }

        const uint64_t virt = map_start + (mapped * KHEAP_PAGE_SIZE);
        if (!paging_map_page(virt, phys, PAGE_WRITABLE | PAGE_NO_EXECUTE)) {
            pmm_free_page(phys);
            break;
        }
    }

    if (mapped != pages) {
        for (uint64_t i = 0ULL; i < mapped; ++i) {
            const uint64_t virt = map_start + (i * KHEAP_PAGE_SIZE);
            const uint64_t phys = paging_lookup_physical(virt);
            paging_unmap_page(virt);
            if (phys != 0ULL) {
                pmm_free_page(phys);
            }
        }
        return false;
    }

    g_heap_capacity += pages * KHEAP_PAGE_SIZE;
    const uint64_t added_bytes = g_heap_capacity - old_capacity;

    if (added_bytes <= sizeof(heap_block_t)) {
        return false;
    }

    if (g_tail != (heap_block_t *)0 && g_tail->free) {
        g_tail->size += added_bytes;
        kheap_coalesce_forward(g_tail);
        return true;
    }

    heap_block_t *block = (heap_block_t *)(uintptr_t)(KHEAP_BASE + old_capacity);
    block->magic = KHEAP_BLOCK_MAGIC;
    block->size = added_bytes - sizeof(heap_block_t);
    block->free = true;
    aurora_memset(block->reserved, 0, sizeof(block->reserved));
    block->prev = g_tail;
    block->next = (heap_block_t *)0;

    if (g_tail != (heap_block_t *)0) {
        g_tail->next = block;
    } else {
        g_head = block;
    }
    g_tail = block;
    return true;
}

void kheap_init(void) {
    g_head = (heap_block_t *)0;
    g_tail = (heap_block_t *)0;
    g_heap_capacity = 0ULL;
    g_heap_used = 0ULL;
    g_heap_ready = false;

    const bool mapped = kheap_map_additional_pages(KHEAP_INITIAL_PAGES);
    if (!mapped) {
        serial_write("[aurora] kheap init failed\n");
        return;
    }

    g_heap_ready = true;
    serial_write("[aurora] kheap online bytes=");
    serial_write_dec_u64(g_heap_capacity);
    serial_write_char('\n');
}

void *kmalloc(size_t size) {
    if (!g_heap_ready || size == 0U) {
        return (void *)0;
    }

    const uint64_t aligned_size = align_up_u64((uint64_t)size, KHEAP_ALIGNMENT);
    heap_block_t *block = kheap_find_free_block(aligned_size);

    if (block == (heap_block_t *)0) {
        const uint64_t needed = aligned_size + sizeof(heap_block_t);
        uint64_t pages = (needed + KHEAP_PAGE_SIZE - 1ULL) / KHEAP_PAGE_SIZE;
        if (pages < 4ULL) {
            pages = 4ULL;
        }

        if (!kheap_map_additional_pages(pages)) {
            return (void *)0;
        }

        block = kheap_find_free_block(aligned_size);
        if (block == (heap_block_t *)0) {
            return (void *)0;
        }
    }

    kheap_split_block(block, aligned_size);
    block->free = false;
    g_heap_used += block->size;

    return (void *)((uint8_t *)(uintptr_t)block + sizeof(heap_block_t));
}

void *kcalloc(size_t count, size_t size) {
    if (count == 0U || size == 0U) {
        return (void *)0;
    }

    if (count > (SIZE_MAX / size)) {
        return (void *)0;
    }

    const size_t total = count * size;
    void *ptr = kmalloc(total);
    if (ptr != (void *)0) {
        aurora_memset(ptr, 0, total);
    }
    return ptr;
}

void *krealloc(void *ptr, size_t new_size) {
    if (ptr == (void *)0) {
        return kmalloc(new_size);
    }
    if (new_size == 0U) {
        kfree(ptr);
        return (void *)0;
    }

    heap_block_t *block = (heap_block_t *)((uint8_t *)ptr - sizeof(heap_block_t));
    if (block->magic != KHEAP_BLOCK_MAGIC || block->free) {
        return (void *)0;
    }

    const uint64_t aligned_size = align_up_u64((uint64_t)new_size, KHEAP_ALIGNMENT);
    if (aligned_size <= block->size) {
        const uint64_t old_size = block->size;
        kheap_split_block(block, aligned_size);
        if (old_size > block->size) {
            g_heap_used -= old_size - block->size;
        }
        return ptr;
    }

    if (block->next != (heap_block_t *)0 && block->next->free && kheap_is_contiguous(block, block->next)) {
        const uint64_t old_size = block->size;
        const uint64_t merged_size = block->size + sizeof(heap_block_t) + block->next->size;
        if (merged_size >= aligned_size) {
            heap_block_t *next = block->next;
            block->size = merged_size;
            block->next = next->next;
            if (block->next != (heap_block_t *)0) {
                block->next->prev = block;
            } else {
                g_tail = block;
            }

            kheap_split_block(block, aligned_size);
            g_heap_used += block->size - old_size;
            return ptr;
        }
    }

    void *new_ptr = kmalloc(new_size);
    if (new_ptr == (void *)0) {
        return (void *)0;
    }

    aurora_memcpy(new_ptr, ptr, (size_t)block->size);
    kfree(ptr);
    return new_ptr;
}

void kfree(void *ptr) {
    if (ptr == (void *)0 || !g_heap_ready) {
        return;
    }

    heap_block_t *block = (heap_block_t *)((uint8_t *)ptr - sizeof(heap_block_t));
    if (block->magic != KHEAP_BLOCK_MAGIC || block->free) {
        return;
    }

    block->free = true;
    if (g_heap_used >= block->size) {
        g_heap_used -= block->size;
    } else {
        g_heap_used = 0ULL;
    }

    (void)kheap_coalesce(block);
}

uint64_t kheap_bytes_total(void) {
    return g_heap_capacity;
}

uint64_t kheap_bytes_used(void) {
    return g_heap_used;
}
