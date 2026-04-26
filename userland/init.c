#include <aurora_uapi/string.h>
#include <aurora_uapi/syscall_lib.h>
#include <stdint.h>

static void write_text(const char *text) {
    (void)u_sys_write_serial(text, u_strlen(text));
}

static void write_key_event(char c) {
    char buffer[32];
    uint64_t length = 0U;

    buffer[length++] = '[';
    buffer[length++] = 'u';
    buffer[length++] = 'i';
    buffer[length++] = 'n';
    buffer[length++] = 'i';
    buffer[length++] = 't';
    buffer[length++] = ']';
    buffer[length++] = ' ';
    buffer[length++] = 'k';
    buffer[length++] = 'e';
    buffer[length++] = 'y';
    buffer[length++] = '=';

    if (c == '\n') {
        buffer[length++] = '\\';
        buffer[length++] = 'n';
    } else if (c == '\t') {
        buffer[length++] = '\\';
        buffer[length++] = 't';
    } else if (c == '\b') {
        buffer[length++] = '\\';
        buffer[length++] = 'b';
    } else {
        buffer[length++] = c;
    }

    buffer[length++] = '\n';
    (void)u_sys_write_serial(buffer, length);
}

__attribute__((noreturn)) void _start(void) {
    write_text("[uinit] userspace init online via int80\n");

    uint64_t last_tick = 0U;
    for (;;) {
        const uint64_t now = u_sys_get_ticks();
        if ((now - last_tick) >= 500U) {
            write_text("[uinit] heartbeat\n");
            last_tick = now;
        }

        const int64_t key = u_sys_getchar();
        if (key >= 0) {
            write_key_event((char)key);
        }

        u_sys_sleep(5U);
    }
}
