#include "../include/ringbuf.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

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
    if (message_len >= *context->end) {
        return OUTPUT_BUFFER_TOO_SMALL;
    }

    for (int i = 0; i < message_len; i++) {
        if (*context->write % *context->end == 0) {
            context->write = context->begin;
        }
        context->begin[*context->write] = ((char *)message)[i];
        *context->write += 1;
    }
    return 0;
}

int ringbuffer_read(rbctx_t *context, void *buffer, size_t *buffer_len) {
    /* your solution here */
}

void ringbuffer_destroy(rbctx_t *context) { /* your solution here */
}
