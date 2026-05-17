#ifndef CORE_IPC_CHANNEL_H
#define CORE_IPC_CHANNEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t* buffer;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t used;
} ipc_channel_t;

void ipc_channel_init(ipc_channel_t* channel, uint8_t* buffer, size_t capacity);
bool ipc_channel_send(ipc_channel_t* channel, const void* data, size_t size);
size_t ipc_channel_recv(ipc_channel_t* channel, void* out, size_t max_size);
size_t ipc_channel_available(const ipc_channel_t* channel);
size_t ipc_channel_free(const ipc_channel_t* channel);

#endif
