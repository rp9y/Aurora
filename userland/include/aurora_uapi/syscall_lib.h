#ifndef AURORA_UAPI_SYSCALL_LIB_H
#define AURORA_UAPI_SYSCALL_LIB_H

#include <stdint.h>

uint64_t u_sys_write_serial(const char *text, uint64_t length);
uint64_t u_sys_get_ticks(void);
int64_t u_sys_getchar(void);
void u_sys_sleep(uint64_t ticks);

#endif
