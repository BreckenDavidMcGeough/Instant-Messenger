#pragma once

#include <errno.h>
#include <unistd.h>

void print_err(char *err) {
    perror(err);
    fprintf(stderr, "This error is likely not your fault, make a post on piazza or talk to a TA\n");
}
