#include <aurora_uapi/syscall.h>
#include <aurora_uapi/syscall_lib.h>

uint64_t u_sys_write_serial(const char *text, uint64_t length) {
    return sys_write_serial(text, length);
}

uint64_t u_sys_get_ticks(void) {
    return sys_get_ticks();
}

int64_t u_sys_getchar(void) {
    return sys_getchar();
}

void u_sys_sleep(uint64_t ticks) {
    sys_sleep(ticks);
}
