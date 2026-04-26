#include <aurora_uapi/string.h>

uint64_t u_strlen(const char *text) {
    if (text == (const char *)0) {
        return 0U;
    }

    uint64_t length = 0U;
    while (text[length] != '\0') {
        ++length;
    }
    return length;
}
