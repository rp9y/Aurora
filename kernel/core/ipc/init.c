#include <aurora/core/ipc/init.h>
#include <aurora/ipc/channel.h>

enum {
    CORE_IPC_BUFFER_SIZE = 1024U
};

static uint8_t g_core_ipc_storage[CORE_IPC_BUFFER_SIZE];
static ipc_channel_t g_core_ipc_channel;
static bool g_core_ipc_ready = false;

void core_ipc_init(void) {
    g_core_ipc_ready = ipc_channel_init(&g_core_ipc_channel, g_core_ipc_storage, CORE_IPC_BUFFER_SIZE);
}

bool core_ipc_send_text(const char *text) {
    if (!g_core_ipc_ready || text == (const char *)0) {
        return false;
    }

    size_t length = 0U;
    while (text[length] != '\0') {
        ++length;
    }

    return ipc_channel_send(&g_core_ipc_channel, (const uint8_t *)text, length);
}

size_t core_ipc_receive(uint8_t *buffer, size_t max_length) {
    if (!g_core_ipc_ready) {
        return 0U;
    }
    return ipc_channel_receive(&g_core_ipc_channel, buffer, max_length);
}
