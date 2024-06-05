#pragma once


/* The size of each packet.  Note that every message is exactly
 * this long, although many of the bytes may be meaningless. */
#define PACKET_SIZE 1024

/*
 * The maximum size of a valid message, the sum of all data lengths
 * must not exceed this.
 */
#define MAX_MESSAGE_SIZE 256

/* Number of bytes to be used for your UBIT name */
#define NAME_SIZE 16

/* Packet type definitions */
#define REFRESH    0
#define STATUS     1
#define MESSAGE    2
#define LABELED    3
#define STATISTICS 4

struct statistics {
    char sender[NAME_SIZE+1];   /* Name of sender */
    int messages_count;         /* Number of messages sent to the server */

    char most_active[NAME_SIZE+1]; /* User who has sent the most messages */
    int most_active_count;      /* Number of messages sent by that user */

    long invalid_count;         /* Number of invalid packets sent to the server */
    long refresh_count;         /* Number of refresh packets sent to the server */
};

int pack(void *packed, char *input);
int pack_refresh(void *packed, int message_id);

int unpack(char *message, void *packed);
int unpack_statistics(struct statistics *statistics, void *packed);
