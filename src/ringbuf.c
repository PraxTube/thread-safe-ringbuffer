#include "../include/ringbuf.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

void print_buf(rbctx_t *context) {
    printf("\nContext: begin: %u, read: %u, write: %u, end: %u\nMessage: ",
           *context->begin, *context->read, *context->write, *context->end);
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

void ringbuffer_init(rbctx_t *context, void *buffer_location,
                     size_t buffer_size) {
    context->begin = buffer_location;

    context->read = malloc(sizeof(uint8_t));
    if (context->read != NULL) {
        *context->read = 0;
    }
    context->write = malloc(sizeof(uint8_t));
    if (context->write != NULL) {
        *context->write = 0;
    }
    context->end = malloc(sizeof(uint8_t));
    if (context->end != NULL) {
        *context->end = buffer_size;
    }
}

int ringbuffer_write(rbctx_t *context, void *message, size_t message_len) {
    // Take into consideration the bytes needed to store the message_len
    if (message_len + sizeof(size_t) >= *context->end) {
        return OUTPUT_BUFFER_TOO_SMALL;
    }

    for (size_t i = 0; i < sizeof(size_t); i++) {
        if (*context->write % *context->end == 0) {
            *context->write = 0;
        }
        context->begin[*context->write] = (message_len << (8 * i)) & 0xFF;
        *context->write = (*context->write + 1) % *context->end;
    }

    for (int i = 0; i < message_len; i++) {
        if (*context->write % *context->end == 0) {
            *context->write = 0;
        }
        context->begin[*context->write] = ((char *)message)[i];
        *context->write = (*context->write + 1) % *context->end;
    }
    *context->write -= 1;
    return SUCCESS;
}

int ringbuffer_read(rbctx_t *context, void *buffer, size_t *buffer_len) {
    // Prevent division by 0
    if (*context->end == 0) {
        return OUTPUT_BUFFER_TOO_SMALL;
    }

    print_buf(context);

    size_t message_len = 0;
    for (size_t i = 0; i < 8; i++) {
        uint8_t byte = ((uint8_t *)context->begin)[*context->read];
        message_len |= byte << (8 * i);
        *context->read = (*context->read + 1) % *context->end;
    }

    if (message_len > *context->end) {
        printf(
            "Error: read message length is bigger than ringbuffer length, %lu "
            "> %u\n",
            message_len, *context->end);
        return 3;
    }

    for (size_t i = 0; i < message_len; i++) {
        char char_to_read = ((char *)context->begin)[*context->read];
        ((char *)buffer)[i] = char_to_read;

        printf("Char: %c\n", char_to_read);
        *context->read = (*context->read + 1) % *context->end;
    }
    print_buf(context);

    return SUCCESS;
}

void ringbuffer_destroy(rbctx_t *context) { /* your solution here */
}
