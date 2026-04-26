#ifndef AURORA_CORE_IPC_INIT_H
#define AURORA_CORE_IPC_INIT_H

#include <aurora/core/types.h>

void core_ipc_init(void);
bool core_ipc_send_text(const char *text);
size_t core_ipc_receive(uint8_t *buffer, size_t max_length);

#endif
