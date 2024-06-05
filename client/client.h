#pragma once

#define PACKET_HISTORY_SIZE 256

struct client;

struct client *client_init(char *server_path, char *client_dir);
void client_destroy(struct client *client);

int client_send(struct client *client, void *buf);
int client_recv(struct client *client, void **buf, int *buflen, int *msgtype);
int client_refresh(struct client *client);
