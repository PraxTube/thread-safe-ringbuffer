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

    if (context->read == NULL) {
        printf("Error: read pointer is null");
        return 1;
    }

    for (int i = 0; i < message_len; i++) {
        if (*context->write % *context->end == 0) {
            *context->write = 0;
        }
        context->begin[*context->write] = ((char *)message)[i];
        *context->write += 1;
    }
    return SUCCESS;
}

int ringbuffer_read(rbctx_t *context, void *buffer, size_t *buffer_len) {
    if (*buffer_len >= *context->end) {
        printf("Error: buffer length is bigger then the context size.\n");
        return OUTPUT_BUFFER_TOO_SMALL;
    }

    printf("READ: %d\n", *context->read);

    for (int i = 0; i < *buffer_len; i++) {
        int index = *context->read % *context->end;
        char c = ((char *)context->begin)[index];
        printf("%c\n", c);
        if (c == '\0') {
            *buffer_len = i + 1;
            break;
        }

        ((char *)buffer)[i] = c;
        *context->read += 1;
    }
    return SUCCESS;
}

void ringbuffer_destroy(rbctx_t *context) { /* your solution here */
}
