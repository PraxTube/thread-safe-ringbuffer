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
            printf("$");
        } else {
            printf("%c", c);
        }
    }
    printf("\n");
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
    *context->write -= 1;
    return SUCCESS;
}

int ringbuffer_read(rbctx_t *context, void *buffer, size_t *buffer_len) {
    if (*buffer_len >= *context->end) {
        printf("Warn: buffer length is bigger than the context size.\n");
    }
    // Prevent division by 0
    if (*context->end == 0) {
        return OUTPUT_BUFFER_TOO_SMALL;
    }

    print_buf(context);

    for (int i = 0; i < *buffer_len; i++) {
        int index = *context->read % *context->end;
        char char_to_read = ((char *)context->begin)[index];
        ((char *)buffer)[i] = char_to_read;

        printf("Char: %c\n", char_to_read);
        if (char_to_read == '\0') {
            *buffer_len = i + 1;
            break;
        }
        *context->read = (*context->read + 1) % *context->end;
    }
    print_buf(context);

    return SUCCESS;
}

void ringbuffer_destroy(rbctx_t *context) { /* your solution here */
}
