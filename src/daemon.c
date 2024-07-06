#include "../include/daemon.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/ringbuf.h"

/* IN THE FOLLOWING IS THE CODE PROVIDED FOR YOU
 * changing the code will result in points deduction */

/********************************************************************
 * NETWORK TRAFFIC SIMULATION:
 * This section simulates incoming messages from various ports using
 * files. Think of these input files as data sent by clients over the
 * network to our computer. The data isn't transmitted in a single
 * large file but arrives in multiple small packets. This concept
 * is discussed in more detail in the advanced module:
 * Rechnernetze und Verteilte Systeme
 *
 * To simulate this parallel packet-based data transmission, we use multiple
 * threads. Each thread reads small segments of the files and writes these
 * smaller packets into the ring buffer. Between each packet, the
 * thread sleeps for a random time between 1 and 100 us. This sleep
 * simulates that data packets take varying amounts of time to arrive.
 *********************************************************************/
typedef struct {
    rbctx_t *ctx;
    connection_t *connection;
} w_thread_args_t;

void *write_packets(void *arg) {
    /* extract arguments */
    rbctx_t *ctx = ((w_thread_args_t *)arg)->ctx;
    size_t from = (size_t)((w_thread_args_t *)arg)->connection->from;
    size_t to = (size_t)((w_thread_args_t *)arg)->connection->to;
    char *filename = ((w_thread_args_t *)arg)->connection->filename;

    /* open file */
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Cannot open file with name %s\n", filename);
        exit(1);
    }

    /* read file in chunks and write to ringbuffer with random delay */
    unsigned char buf[MESSAGE_SIZE];
    size_t packet_id = 0;
    size_t read = 1;
    while (read > 0) {
        size_t msg_size = MESSAGE_SIZE - 3 * sizeof(size_t);
        read = fread(buf + 3 * sizeof(size_t), 1, msg_size, fp);
        if (read > 0) {
            memcpy(buf, &from, sizeof(size_t));
            memcpy(buf + sizeof(size_t), &to, sizeof(size_t));
            memcpy(buf + 2 * sizeof(size_t), &packet_id, sizeof(size_t));
            while (ringbuffer_write(ctx, buf, read + 3 * sizeof(size_t)) !=
                   SUCCESS) {
                usleep(((rand() % 50) +
                        25));  // sleep for a random time between 25 and 75 us
            }
        }
        packet_id++;
        usleep(((rand() % (100 - 1)) +
                1));  // sleep for a random time between 1 and 100 us
    }
    fclose(fp);
    return NULL;
}

/* END OF PROVIDED CODE */

/********************************************************************/

/* YOUR CODE STARTS HERE */

#define NUMBER_OF_STRINGS 1000
#define BUF_SIZE 50     // bytes
#define RBUF_SIZE 500   // bytes
#define WAIT_TIME 1000  // usec

typedef struct {
    rbctx_t *ctx;
} r_thread_args_t;

// 1. read functionality
// 2. filtering functionality
// 3. (thread-safe) write to file functionality

void *read_packets(void *arg) {
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

    rbctx_t *ctx = ((r_thread_args_t *)arg)->ctx;

    unsigned char buffer[MESSAGE_SIZE];
    size_t buffer_len = MESSAGE_SIZE;
    while (1) {
        buffer_len = MESSAGE_SIZE;
        while (ringbuffer_read(ctx, buffer, &buffer_len) == RINGBUFFER_EMPTY) {
            buffer_len = MESSAGE_SIZE;
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
            usleep(WAIT_TIME);
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        }

        size_t n = sizeof(size_t);
        if (buffer_len < 3 * n) {
            printf("error: The read message is too small, %zu\n", buffer_len);
            continue;
        }

        printf("ok: length of buffer is %zu\n", buffer_len);

        size_t source_port, target_port, packet_id;
        memcpy(&source_port, buffer, n);
        memcpy(&target_port, buffer + n, n);
        memcpy(&packet_id, buffer + 2 * n, n);

        if (source_port > MAXIMUM_PORT || target_port > MAXIMUM_PORT) {
            printf("error: Port bigger than maximum port allowed, from: %zu, to: %zu", source_port, target_port);
            exit(1);
        }

        size_t message_len = buffer_len - 3 * n;
        unsigned char message[message_len];
        memcpy(message, buffer + 3 * n, message_len);

        printf("Message: %s\n", message);

        if (source_port == target_port) {
            continue;
        } else if (source_port == 42 || target_port == 42) {
            continue;
        } else if (source_port + target_port == 42) {
            continue;
        }

        int contains_malicious = 0;
        unsigned char malicious_message[10] = {0};
        for (int i = 0; i < message_len; i++) {
            if (!strchr("malicious", message[i])) {
                continue;
            }

            for (size_t i = 0; i < 8; i++) {
                malicious_message[i] = malicious_message[i + 1];
            }
            malicious_message[8] = message[i];

            if (strcmp((char *)malicious_message, "malicious") == 0) {
                contains_malicious = 1;
                break;
            }
        }

        if (contains_malicious) {
            continue;
        }

        char file_name[20];
        sprintf(file_name, "%zu.txt", target_port);
        FILE *fout = fopen(file_name, "w");

        if (fout == NULL) {
            return NULL;
        }

        fprintf(fout, (char *)message);

        fclose(fout);
    };
    return NULL;
}

/* YOUR CODE ENDS HERE */

/********************************************************************/

int simpledaemon(connection_t *connections, int nr_of_connections) {
    /* initialize ringbuffer */
    rbctx_t rb_ctx;
    size_t rbuf_size = 1024;
    void *rbuf = malloc(rbuf_size);
    if (rbuf == NULL) {
        fprintf(stderr, "Error allocation ringbuffer\n");
    }

    ringbuffer_init(&rb_ctx, rbuf, rbuf_size);

    /****************************************************************
     * WRITER THREADS
     * ***************************************************************/

    /* prepare writer thread arguments */
    w_thread_args_t w_thread_args[nr_of_connections];
    for (int i = 0; i < nr_of_connections; i++) {
        w_thread_args[i].ctx = &rb_ctx;
        w_thread_args[i].connection = &connections[i];
        /* guarantee that port numbers range from MINIMUM_PORT (0) - MAXIMUMPORT
         */
        if (connections[i].from > MAXIMUM_PORT ||
            connections[i].to > MAXIMUM_PORT ||
            connections[i].from < MINIMUM_PORT ||
            connections[i].to < MINIMUM_PORT) {
            fprintf(stderr, "Port numbers %d and/or %d are too large\n",
                    connections[i].from, connections[i].to);
            exit(1);
        }
    }

    /* start writer threads */
    pthread_t w_threads[nr_of_connections];
    for (int i = 0; i < nr_of_connections; i++) {
        pthread_create(&w_threads[i], NULL, write_packets, &w_thread_args[i]);
    }

    /****************************************************************
     * READER THREADS
     * ***************************************************************/

    pthread_t r_threads[NUMBER_OF_PROCESSING_THREADS];

    /* END OF PROVIDED CODE */

    /********************************************************************/

    /* YOUR CODE STARTS HERE */

    printf("creating reader threads\n");

    for (size_t i = 0; i < NUMBER_OF_PROCESSING_THREADS; i++) {
        r_thread_args_t r_args = {&rb_ctx};
        pthread_create(&r_threads[i], NULL, read_packets, &r_args);
    }

    /* YOUR CODE ENDS HERE */

    /********************************************************************/

    /* IN THE FOLLOWING IS THE CODE PROVIDED FOR YOU
     * changing the code will result in points deduction */

    /****************************************************************
     * CLEANUP
     * ***************************************************************/

    /* after 5 seconds JOIN all threads (we should definitely have received all
     * messages by then) */
    printf(
        "daemon: waiting for 5 seconds before canceling reading threads\nYou "
        "may want to increase this sleep time if the tests keep failing\n");
    sleep(5);
    for (int i = 0; i < NUMBER_OF_PROCESSING_THREADS; i++) {
        pthread_cancel(r_threads[i]);
    }

    /* wait for all threads to finish */
    for (int i = 0; i < nr_of_connections; i++) {
        pthread_join(w_threads[i], NULL);
    }

    /* join all threads */
    for (int i = 0; i < NUMBER_OF_PROCESSING_THREADS; i++) {
        pthread_join(r_threads[i], NULL);
    }

    /* END OF PROVIDED CODE */

    /********************************************************************/

    /* YOUR CODE STARTS HERE */

    // use this section to free any memory, destory mutexe etc.

    /* YOUR CODE ENDS HERE */

    /********************************************************************/

    /* IN THE FOLLOWING IS THE CODE PROVIDED FOR YOU
     * changing the code will result in points deduction */

    free(rbuf);
    ringbuffer_destroy(&rb_ctx);

    return 0;

    /* END OF PROVIDED CODE */
}
