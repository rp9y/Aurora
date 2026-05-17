#include "core/ipc/channel.h"

#include "core/libk/string.h"

void ipc_channel_init(ipc_channel_t* channel, uint8_t* buffer, size_t capacity) {
    channel->buffer = buffer;
    channel->capacity = capacity;
    channel->head = 0;
    channel->tail = 0;
    channel->used = 0;
}

size_t ipc_channel_available(const ipc_channel_t* channel) {
    return channel->used;
}

size_t ipc_channel_free(const ipc_channel_t* channel) {
    return channel->capacity - channel->used;
}

bool ipc_channel_send(ipc_channel_t* channel, const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    if (size > ipc_channel_free(channel)) {
        return false;
    }
    for (size_t i = 0; i < size; i++) {
        channel->buffer[channel->tail] = bytes[i];
        channel->tail = (channel->tail + 1) % channel->capacity;
    }
    channel->used += size;
    return true;
}

size_t ipc_channel_recv(ipc_channel_t* channel, void* out, size_t max_size) {
    const size_t pull = (channel->used < max_size) ? channel->used : max_size;
    uint8_t* bytes = (uint8_t*)out;
    for (size_t i = 0; i < pull; i++) {
        bytes[i] = channel->buffer[channel->head];
        channel->head = (channel->head + 1) % channel->capacity;
    }
    channel->used -= pull;
    return pull;
}
