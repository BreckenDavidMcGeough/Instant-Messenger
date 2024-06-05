#pragma once

#include "../src/serialize.h"

// Maximum number of characters in input box
#define MAX_INPUT_SIZE 256

struct ui;

struct ui *ui_init(void);
void ui_destroy(struct ui *ui);

void ui_display(struct ui *ui, char *message);
void ui_display_italics(struct ui *ui, char *message);
void ui_display_statistics(struct ui *ui, struct statistics *statistics);
char *ui_readline(struct ui *ui, volatile int *running);
