#include <aurora/ipc/channel.h>

bool ipc_channel_init(ipc_channel_t *channel, uint8_t *storage, size_t capacity) {
    if (channel == (ipc_channel_t *)0 || storage == (uint8_t *)0 || capacity == 0U) {
        return false;
    }

    channel->buffer = storage;
    channel->capacity = capacity;
    channel->read_index = 0U;
    channel->write_index = 0U;
    channel->count = 0U;
    return true;
}

bool ipc_channel_send(ipc_channel_t *channel, const uint8_t *data, size_t length) {
    if (channel == (ipc_channel_t *)0 || data == (const uint8_t *)0) {
        return false;
    }

    if (length > (channel->capacity - channel->count)) {
        return false;
    }

    for (size_t i = 0U; i < length; ++i) {
        channel->buffer[channel->write_index] = data[i];
        channel->write_index = (channel->write_index + 1U) % channel->capacity;
    }
    channel->count += length;
    return true;
}

size_t ipc_channel_receive(ipc_channel_t *channel, uint8_t *data_out, size_t max_length) {
    if (channel == (ipc_channel_t *)0 || data_out == (uint8_t *)0 || max_length == 0U) {
        return 0U;
    }

    size_t read = 0U;
    while (read < max_length && channel->count > 0U) {
        data_out[read++] = channel->buffer[channel->read_index];
        channel->read_index = (channel->read_index + 1U) % channel->capacity;
        --channel->count;
    }

    return read;
}

size_t ipc_channel_pending(const ipc_channel_t *channel) {
    if (channel == (const ipc_channel_t *)0) {
        return 0U;
    }
    return channel->count;
}
