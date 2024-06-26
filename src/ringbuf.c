#include "../include/ringbuf.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

void print_buf(rbctx_t *context) {
    uintptr_t begin = (uintptr_t)context->begin;

    printf("\nContext: begin: %lu, read: %lu, write: %lu, end: %lu\nMessage: ",
           (unsigned long)(context->begin),
           (unsigned long)(context->read - begin),
           (unsigned long)(context->write - begin),
           (unsigned long)(context->end - begin));
    for (int i = 0; i < *context->end; i++) {
        char c = context->begin[i];
        if (c == '\0') {
            printf("$ ");
        } else {
            printf("%c", c);
        }
    }
    printf("---FIN\n");
}

void increment_writer(rbctx_t *context) {
    context->write += 1;
    if (context->write >= context->end) {
        context->write = context->begin;
    }
}

void increment_reader(rbctx_t *context) {
    context->read += 1;
    if (context->read >= context->end) {
        context->read = context->begin;
    }
}

void ringbuffer_init(rbctx_t *context, void *buffer_location,
                     size_t buffer_size) {
    context->begin = buffer_location;

    context->read = malloc(sizeof(uint8_t));
    if (context->read != NULL) {
        context->read = buffer_location;
    }
    context->write = malloc(sizeof(uint8_t));
    if (context->write != NULL) {
        context->write = buffer_location;
    }
    context->end = malloc(sizeof(uint8_t));
    if (context->end != NULL) {
        context->end = buffer_location + buffer_size;
    }
}

int ringbuffer_write(rbctx_t *context, void *message, size_t message_len) {
    // Take into consideration the bytes needed to store the message_len
    if (context->begin + message_len + sizeof(size_t) >= context->end) {
        return OUTPUT_BUFFER_TOO_SMALL;
    }

    // Write the size of the message into buffer before the actual content
    for (size_t i = 0; i < sizeof(message_len); i++) {
        *context->write = (message_len << (8 * i)) & 0xFF;
        increment_writer(context);
    }

    // Write content of message into rinbuffer
    for (int i = 0; i < message_len; i++) {
        *context->write = ((char *)message)[i];
        increment_writer(context);
    }
    // *context->write -= 1;
    return SUCCESS;
}

int ringbuffer_read(rbctx_t *context, void *buffer, size_t *buffer_len) {
    print_buf(context);

    size_t message_len = 0;
    for (size_t i = 0; i < 8; i++) {
        uint8_t byte = *context->read;
        message_len |= byte << (8 * i);
        increment_reader(context);
    }

    if (context->begin + message_len > context->end) {
        printf(
            "Error: read message length is bigger than ringbuffer length, %lu "
            "> %u\n",
            message_len, *context->end);
        return 3;
    }

    for (size_t i = 0; i < message_len; i++) {
        char char_to_read = *context->read;
        ((char *)buffer)[i] = char_to_read;

        printf("Char: %c\n", char_to_read);
        increment_reader(context);
    }
    print_buf(context);

    return SUCCESS;
}

void ringbuffer_destroy(rbctx_t *context) { /* your solution here */
}
