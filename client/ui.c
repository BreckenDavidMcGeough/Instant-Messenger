#include <stdio.h>
#include <stdlib.h>
#include <curses.h>

#include "ui.h"

struct ui {
    WINDOW *window, *input_box, *input_win, *view_win;
    int lines, cols;
    char inbuf[MAX_INPUT_SIZE];
};

struct ui *ui_init(void) {
    struct ui *ui = calloc(1, sizeof(struct ui));
    if ((ui->window = initscr()) == NULL) {
        ui_destroy(ui);
        return NULL;
    }
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    refresh();

    if (LINES < 20 || COLS < 80) {
        fprintf(stderr, "Terminal too small (%d, %d)\n", COLS, LINES);
        ui_destroy(ui);
        return NULL;
    }

    ui->view_win = newwin(LINES - 3, COLS, 0, 0);
    scrollok(ui->view_win, 1);

    ui->input_box = newwin(3, COLS, LINES - 3, 0);
    box(ui->input_box, 0, 0);

    ui->input_win = newwin(1, COLS, LINES - 2, 1);

    wrefresh(ui->view_win);
    wrefresh(ui->input_win);
    wrefresh(ui->input_box);

    return ui;
}

void ui_destroy(struct ui *ui) {
    endwin();
    free(ui);
}

void ui_display(struct ui *ui, char *message) {
    wprintw(ui->view_win, "> %s\n", message);
    wrefresh(ui->view_win);
}

void ui_display_italics(struct ui *ui, char *message) {
    wattr_on(ui->view_win, A_ITALIC, NULL);
    wprintw(ui->view_win, "> %s\n", message);
    wattr_off(ui->view_win, A_ITALIC, NULL);
    wrefresh(ui->view_win);
}

void ui_display_statistics(struct ui *ui, struct statistics *statistics) {
    wattr_on(ui->view_win, A_BOLD, NULL);
    wprintw(ui->view_win, "> %s - Statistics:\n", statistics->sender);
    wattr_off(ui->view_win, A_BOLD, NULL);
    wprintw(ui->view_win, "\t Number of valid messages sent to server: %d\n", statistics->messages_count);
    wprintw(ui->view_win, "\t Number of invalid messages sent to server: %ld\n", statistics->invalid_count);
    wprintw(ui->view_win, "\t Number of refresh messages sent to server: %ld\n", statistics->refresh_count);
    wprintw(ui->view_win, "\t Most active user: %s (%d messages)\n", statistics->most_active,
            statistics->most_active_count);
    wrefresh(ui->view_win);
}

// Read in input, and fill input box
// Blocks until enter is pressed
char *ui_readline(struct ui *ui, volatile int *running) {
    char in;
    int i;
    move(LINES - 2, 1);
    ui->inbuf[0] = 0;

    i = 0;
    while (*running && (in = getch()) != '\n') {
        if (in == KEY_BACKSPACE || in == 127 || in == 7) {
	    if (i > 0) {
		--i;
	    }
        } else if (i < (COLS-2)) {
            ui->inbuf[i] = in;
            ++i;
        }
        ui->inbuf[i] = '\0';
        mvwprintw(ui->input_win, 0, 0, "%s  ", ui->inbuf);
        wmove(ui->input_win, 0, getcurx(ui->input_win) - 2);
        wrefresh(ui->input_win);
    }
    wclear(ui->input_win);
    wrefresh(ui->input_win);
    if (*running == 0)
    {
	ui->inbuf[0] = 0;
    }
    return ui->inbuf;
}
