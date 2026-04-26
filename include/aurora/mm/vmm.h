#ifndef AURORA_MM_VMM_H
#define AURORA_MM_VMM_H

#include <aurora/core/types.h>

void vmm_init(void);
void *vmm_alloc(size_t size, uint64_t page_flags);
void vmm_free(void *virtual_address, size_t size);
uint64_t vmm_bytes_reserved(void);

#endif
