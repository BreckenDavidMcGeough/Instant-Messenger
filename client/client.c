#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <time.h>
#include <poll.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

#include "err.h"
#include "client.h"

#include "../src/serialize.h"

struct client {
    int sockfd;
    char *server_path;
    char *socket_path;

    struct msghdr *send_msghdr;
    struct sockaddr_un *send_addr;
    char *sendbuf;
    pthread_mutex_t *sendlock;

    struct msghdr *recv_msghdr;
    struct sockaddr_un *recv_addr;
    char recvbuf[PACKET_HISTORY_SIZE][PACKET_SIZE];
    char refresh[PACKET_SIZE];
    int message_id;
    int message_type;
};

char *gen_path(char *dir);
int create_socket(char *filename);

/* server_path must be canonicalized (e.g., using realpath()) before it is passed in. */
struct client *client_init(char *server_path, char *client_dir) {
    struct client *cl = malloc(sizeof(struct client));

    cl->socket_path = gen_path(client_dir);
    cl->sockfd = create_socket(cl->socket_path);
    cl->server_path = strdup(server_path);

    // Send structures
    cl->send_msghdr = calloc(1, sizeof(struct msghdr));
    cl->sendbuf = calloc(PACKET_SIZE, sizeof(char));
    cl->send_addr = malloc(sizeof(struct sockaddr_un));
    cl->sendlock = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(cl->sendlock, NULL);

    cl->send_addr->sun_family = AF_UNIX;
    strncpy(cl->send_addr->sun_path, server_path, 100);

    cl->send_msghdr->msg_name = cl->send_addr;
    cl->send_msghdr->msg_namelen = sizeof(struct sockaddr_un);
    cl->send_msghdr->msg_iov = malloc(sizeof(struct iovec));
    cl->send_msghdr->msg_iov->iov_base = cl->sendbuf;
    cl->send_msghdr->msg_iov->iov_len = PACKET_SIZE;
    cl->send_msghdr->msg_iovlen = 1;

    int controllen = CMSG_SPACE(sizeof(struct ucred));
    cl->send_msghdr->msg_controllen = controllen;
    cl->send_msghdr->msg_control = malloc(controllen);

    struct cmsghdr *send_cmsg;
    send_cmsg = CMSG_FIRSTHDR(cl->send_msghdr);
    send_cmsg->cmsg_level = SOL_SOCKET;
    send_cmsg->cmsg_type = SCM_CREDENTIALS;
    send_cmsg->cmsg_len = CMSG_LEN(sizeof(struct ucred));

    // Receive structures
    cl->recv_msghdr = calloc(1, sizeof(struct msghdr));
    cl->recv_addr = malloc(sizeof(struct sockaddr_un));

    cl->recv_msghdr->msg_name = cl->recv_addr;
    cl->recv_msghdr->msg_namelen = sizeof(struct sockaddr_un);
    cl->recv_msghdr->msg_iov = malloc(sizeof(struct iovec) * (PACKET_HISTORY_SIZE + 2));
    cl->recv_msghdr->msg_iovlen = PACKET_HISTORY_SIZE + 2;
    cl->recv_msghdr->msg_iov[0].iov_base = &cl->message_type;
    cl->recv_msghdr->msg_iov[0].iov_len = sizeof(int);
    cl->recv_msghdr->msg_iov[1].iov_base = &cl->message_id;
    cl->recv_msghdr->msg_iov[1].iov_len = sizeof(int);
    for (int i = 0; i < PACKET_HISTORY_SIZE; ++i) {
        cl->recv_msghdr->msg_iov[i + 2].iov_base = cl->recvbuf[i];
        cl->recv_msghdr->msg_iov[i + 2].iov_len = PACKET_SIZE;
    }

    cl->message_id = -1;

    struct ucred *uc = (struct ucred *)CMSG_DATA(send_cmsg);
    uc->uid = getuid();
    uc->gid = getgid();
    uc->pid = getpid();

    return cl;
}

void client_destroy(struct client *client) {
    unlink(client->socket_path);

    free(client->sendbuf);
    free(client->send_addr);
    free(client->socket_path);
    free(client->server_path);
    free(client->send_msghdr->msg_iov);
    free(client->send_msghdr->msg_control);
    free(client->send_msghdr);
    free(client->recv_msghdr->msg_iov);
    free(client->recv_msghdr);
    free(client->recv_addr);
    free(client);
}

int client_send(struct client *client, void *buf) {
    pthread_mutex_lock(client->sendlock);
    memcpy(client->sendbuf, buf, PACKET_SIZE);
    if (sendmsg(client->sockfd, client->send_msghdr, 0) < 0) {
        print_err("sendmsg");
        return -1;
    }
    pthread_mutex_unlock(client->sendlock);
    return 0;
}

int client_refresh(struct client *client) {
    if (pack_refresh(client->refresh, client->message_id) != 0) {
        return -1;
    }
    if (client_send(client, client->refresh) != 0) {
        return -1;
    }
    return 0;
}

int client_recv(struct client *client, void **buf, int *buflen, int *msgtype) {
    int old_message_id = client->message_id;
    struct pollfd pfd;
    pfd.fd = client->sockfd;
    pfd.events = POLLIN;

    // Timeout if the client doesn't receive anything
    if (poll(&pfd, 1, 100) == 0) {
        *msgtype = -1;
        *buflen = 0;
        return 0;
    }

    if (recvmsg(client->sockfd, client->recv_msghdr, 0) < 0) {
        print_err("recvmsg");
	*msgtype = -1;
        *buflen = 0;
        return -1;
    }

    if (strcmp(client->recv_addr->sun_path, client->server_path) != 0) {
	printf("Paths do not match: %s, %s\n",
	       client->recv_addr->sun_path, client->server_path);
	*msgtype = -1;
        *buflen = 0;
        return -1;
    }

    *msgtype = client->message_type;

    if (*msgtype == REFRESH) {
        *buflen = client->message_id - old_message_id;
        if (old_message_id == -1 || *buflen >= PACKET_HISTORY_SIZE || *buflen < 0) {
            *buflen = 0;
            return 0;
        }
    } else {
        *buflen = client->message_id;
        client->message_id = old_message_id;
    }

    for (int i = 0; i < *buflen; ++i) {
        buf[i] = client->recvbuf[i];
    }


    return 0;
}

int create_socket(char *filename) {
    // Initialize socket
    struct sockaddr_un name;
    int sockfd;
    mode_t mode;

    sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        print_err("Socket");
        return -1;
    }

    memset(&name, 0, sizeof(struct sockaddr_un));
    name.sun_family = AF_UNIX;
    strncpy(name.sun_path, filename, 100);

    mode = umask(0);
    if (bind(sockfd, (struct sockaddr *)&name, sizeof(name)) < 0) {
        print_err("Socket Bind");
        return -1;
    }

    umask(mode);

    return sockfd;
}

char *gen_path(char *dir) {
    char *path = calloc(100, sizeof(char));
    struct passwd *pwd = getpwuid(getuid());
    snprintf(path, 100, "%s/%s-%ld", dir, pwd->pw_name, time(NULL));
    return path;
}
