#ifndef AURORA_IPC_CHANNEL_H
#define AURORA_IPC_CHANNEL_H

#include <aurora/core/types.h>

typedef struct ipc_channel {
    uint8_t *buffer;
    size_t capacity;
    size_t read_index;
    size_t write_index;
    size_t count;
} ipc_channel_t;

bool ipc_channel_init(ipc_channel_t *channel, uint8_t *storage, size_t capacity);
bool ipc_channel_send(ipc_channel_t *channel, const uint8_t *data, size_t length);
size_t ipc_channel_receive(ipc_channel_t *channel, uint8_t *data_out, size_t max_length);
size_t ipc_channel_pending(const ipc_channel_t *channel);

#endif
