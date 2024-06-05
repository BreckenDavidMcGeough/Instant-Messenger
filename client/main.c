#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <limits.h>

#include "client.h"
#include "ui.h"
#include "../src/serialize.h"

static volatile int running;
static volatile int resized;

void usage(char *argv[]) {
    fprintf(stderr, "Usage: %s <server path> <client directory>\n", argv[0]);
    fprintf(stderr, "Server Path must be less than 100 characters long when fully expanded\n");
    fprintf(stderr, "Client Directory must be lass than 90 characters long when fully expanded\n");
    fprintf(stderr, "Client Directory is not required, it will default to `.`\n");
}

void sigint_handler(int signum) {
    running = 0;
}

void sigwinch_handler(int signum) {
    running = 0;
    resized = 1;
}

void *refresh_worker(void *data) {
    struct client *client = ((void **)data)[0];
    struct ui *ui = ((void **)data)[1];

    void *buf[PACKET_HISTORY_SIZE];
    char msg[PACKET_SIZE];
    struct statistics stats;
    int nmessages = -1;
    int msgtype = -1;

    while (running) {
        if (client_refresh(client) == -1) {
            continue;
        }

        do {
            if (client_recv(client, buf, &nmessages, &msgtype) == -1) {
                break;
            }

            for (int i = 0; i < nmessages; ++i) {
                if (msgtype == STATISTICS) {
                    if (unpack_statistics(&stats, buf[i]) < 0) {
                        ui_display(ui, "Failed to unpack statistics");
                        continue;
                    }
                    ui_display_statistics(ui, &stats);
                } else {
                    if (unpack(msg, buf[i]) < 0) {
                        ui_display(ui, "Failed to unpack packet");
                        continue;
                    }

                    if (msgtype == STATUS) {
                        ui_display_italics(ui, msg);
                    } else {
                        ui_display(ui, msg);
                    }
                }
            }                   // for loop

        } while (running && nmessages != 0);

        sleep(1);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    struct ui *ui;
    struct client *client;
    char *server_path = NULL;
    char *client_dir = NULL;
    struct sigaction sa;

    if (argc < 2) {
        usage(argv);
        return -1;
    }

    if ((server_path = realpath(argv[1], NULL)) == NULL) {
	perror("Server Path");
	return -1;
    }

    if (strlen(server_path) >= 100) {
        fprintf(stderr, "The given server socket path is too long, must be fewer than 100 characters:\n");
        fprintf(stderr, "%s\n", server_path);
	free(server_path);
        return -1;
    }

    if (argc < 3) {
	client_dir = ".";
    } else {
	client_dir = argv[2];
    }

    if ((client_dir = realpath(client_dir, NULL)) == NULL) {
	perror("Client Directory");
	free(server_path);
	return -1;
    }

    char packed[PACKET_SIZE];
    pthread_t refresh_thread;
    void *udata[2];

    client = client_init(server_path, client_dir);
    if ((ui = ui_init()) == NULL) {
	fprintf(stderr, "Failed to initialize user interface");
	client_destroy(client);
	free(server_path);
	free(client_dir);
	return -1;
    }

    running = 1;
    resized = 0;
    memset(&sa, 0, sizeof(struct sigaction));
    /* Allow C-c to interrupt getch */
    sa.sa_handler = sigint_handler;
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sa.sa_handler = sigwinch_handler;
    sigaction(SIGWINCH, &sa, NULL);

    udata[0] = client;
    udata[1] = ui;

    pthread_create(&refresh_thread, NULL, refresh_worker, udata);

    while (running) {
        char *inbuf = ui_readline(ui, &running);
        int msgtype;

        if ((msgtype = pack(packed, inbuf)) < 0) {
            continue;
        }
        client_send(client, packed);
    }

    pthread_join(refresh_thread, NULL);

    ui_destroy(ui);
    client_destroy(client);

    free(server_path);
    free(client_dir);

    if (resized) {
        fprintf(stderr, "Do not resize the window while the chat is running\n");
    }

    return 0;
}
