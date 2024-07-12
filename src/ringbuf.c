#include "../include/ringbuf.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

uint8_t *incremented_writer(rbctx_t *context, uint8_t *writer_ptr) {
    writer_ptr += 1;
    if (writer_ptr >= context->end) {
        writer_ptr = context->begin;
    }
    return writer_ptr;
}

uint8_t *incremented_reader(rbctx_t *context, uint8_t *reader_ptr) {
    reader_ptr += 1;
    if (reader_ptr >= context->end) {
        reader_ptr = context->begin;
    }
    return reader_ptr;
}

size_t writable_space(rbctx_t *context) {
    if (context->write < context->read) {
        return context->read - context->write - 1;
    }
    return context->end - context->write + context->read - context->begin - 1;
}

size_t readable_space(rbctx_t *context) {
    if (context->write >= context->read) {
        return context->write - context->read;
    }
    return context->end - context->read + context->write - context->begin;
}

struct timespec get_abstime() {
    struct timespec wait_until;
    clock_gettime(CLOCK_REALTIME, &wait_until);
    wait_until.tv_sec += 1;
    return wait_until;
}

void ringbuffer_init(rbctx_t *context, void *buffer_location,
                     size_t buffer_size) {
    context->begin = buffer_location;
    context->read = buffer_location;
    context->write = buffer_location;
    context->end = buffer_location + buffer_size;

    pthread_mutex_init(&context->mtx, NULL);
    pthread_cond_init(&context->sig, NULL);
}

int ringbuffer_write(rbctx_t *context, void *message, size_t message_len) {
    // Take into consideration the bytes needed to store the message_len
    pthread_mutex_lock(&context->mtx);
    while (writable_space(context) < message_len + sizeof(size_t)) {
        struct timespec abstime = get_abstime();
        if (pthread_cond_timedwait(&context->sig,
                                   &context->mtx, &abstime) != 0) {
            pthread_mutex_unlock(&context->mtx);
            return RINGBUFFER_FULL;
        }
    }

    if (writable_space(context) < message_len + sizeof(size_t)) {
        pthread_mutex_unlock(&context->mtx);
        return RINGBUFFER_FULL;
    }

    uint8_t *tmp_writer = context->write;
    // Write the size of the message into buffer before the actual content
    for (size_t i = 0; i < sizeof(message_len); i++) {
        *tmp_writer = (uint8_t)((message_len >> (8 * i)) & 0xFF);
        tmp_writer = incremented_writer(context, tmp_writer);
    }

    // Write content of message into rinbuffer
    for (size_t i = 0; i < message_len; i++) {
        *tmp_writer = ((char *)message)[i];
        tmp_writer = incremented_writer(context, tmp_writer);
    }
    context->write = tmp_writer;

    pthread_cond_signal(&context->sig);
    pthread_mutex_unlock(&context->mtx);
    return SUCCESS;
}

int ringbuffer_read(rbctx_t *context, void *buffer, size_t *buffer_len) {
    pthread_mutex_lock(&context->mtx);
    if (readable_space(context) < sizeof(size_t)) {
        pthread_mutex_unlock(&context->mtx);
        return RINGBUFFER_EMPTY;
    }

    uint8_t *tmp_reader = context->read;
    // Read the size of the message before reading the actual content
    size_t message_len = 0;
    for (size_t i = 0; i < sizeof(size_t); i++) {
        message_len |= (uint8_t)*tmp_reader << (8 * i);
        tmp_reader = incremented_reader(context, tmp_reader);
    }

    if (message_len > *buffer_len) {
        context->read = tmp_reader;
        pthread_mutex_unlock(&context->mtx);
        return OUTPUT_BUFFER_TOO_SMALL;
    }
    *buffer_len = message_len;

    while (readable_space(context) < message_len) {
        struct timespec abstime = get_abstime();
        if (pthread_cond_timedwait(&context->sig, &context->mtx,
                                   &abstime) != 0) {
            context->read = tmp_reader;
            pthread_mutex_unlock(&context->mtx);
            return RINGBUFFER_EMPTY;
        }
    }

    if (readable_space(context) < message_len) {
        context->read = tmp_reader;
        pthread_mutex_unlock(&context->mtx);
        return RINGBUFFER_EMPTY;
    }

    // Read the actual content of the ringbuffer into the given buffer
    for (size_t i = 0; i < message_len; i++) {
        ((char *)buffer)[i] = *tmp_reader;
        tmp_reader = incremented_reader(context, tmp_reader);
    }
    context->read = tmp_reader;

    pthread_cond_signal(&context->sig);
    pthread_mutex_unlock(&context->mtx);
    return SUCCESS;
}

void ringbuffer_destroy(rbctx_t *context) {
    if (context == NULL) {
        return;
    }

    pthread_mutex_destroy(&context->mtx);
    pthread_cond_destroy(&context->sig);
}
