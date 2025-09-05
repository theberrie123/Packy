#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>


const char *headers[] = {
    "Device", "Start", "End", "Sectors", "Size", "Id", "Type"
};
const int n_headers = sizeof(headers) / sizeof(headers[0]);


struct Partition {
    char device[16];
    unsigned long start;
    unsigned long end;
    unsigned long sectors;
    char size[8];
    char id[4];
    char type[32];
};

struct Partition partitions[] = {
    {"/dev/nvme0n1", 2048, 1026047, 1024000, "500M", "83", "Linux"},
    {"/dev/nvme0n2", 1026048, 2097151, 1071104, "512M", "82", "Linux swap"},
    {"/dev/nvme0n3", 2097152, 8388607, 6291456, "3G",   "83", "Linux"},
};
const int n_partitions = sizeof(partitions) / sizeof(partitions[0]);

const char *menu_choices[] = {
    "Quit", "Resize", "Delete", "Write"
};
const int n_menu_choices = sizeof(menu_choices) / sizeof(menu_choices[0]);

struct termios orig_termios;

void enable_raw_mode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig_termios);
    raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

void clear_screen() {
    printf("\033[2J\033[H");
    fflush(stdout);
}

void hide_cursor() {
    printf("\033[?25l");
    fflush(stdout);
}

void show_cursor() {
    printf("\033[?25h");
    fflush(stdout);
}

int get_terminal_width() {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
        return 80;
    }
    return w.ws_col;
}

void print_centered(const char *text) {
    int term_width = get_terminal_width();
    int padding = (term_width - (int)strlen(text)) / 2;
    if (padding < 0) padding = 0;
    for (int i = 0; i < padding; i++) printf(" ");
    printf("%s\n", text);
}

void show_diskman(int part_highlight, int menu_highlight,
                 const char *disk, const char *size_info) {
    clear_screen();

    // Top info
    char disk_line[128];
    snprintf(disk_line, sizeof(disk_line), "Disk: %s", disk);
    print_centered(disk_line);
    print_centered(size_info);
    printf("\n");

    // Column widths
    const int widths[] = {11, 1, 10, 10, 10, 7, 3, 12};
    int table_width = 0;
    for (int i = 0; i < n_headers; i++) table_width += widths[i] + 1; // +1 for space

    // Print headers centered
    int term_width = get_terminal_width();
    int padding = (term_width - table_width) / 2;
    if (padding < 0) padding = 0;
    for (int i = 0; i < padding; i++) printf(" ");
    for (int i = 0; i < n_headers; i++) {
        printf("%-*s ", widths[i], headers[i]);
    }
    printf("\n");

    // Print partitions
    for (int i = 0; i < n_partitions; i++) {
        for (int j = 0; j < padding; j++) printf(" ");
        if (i == part_highlight) printf("\033[7m"); // reverse video

        printf("%-*s %*lu %*lu %*lu %*s %*s %-*s",
               widths[0], partitions[i].device,
               widths[2], partitions[i].start,
               widths[3], partitions[i].end,
               widths[4], partitions[i].sectors,
               widths[5], partitions[i].size,
               widths[6], partitions[i].id,
               widths[7], partitions[i].type);

        if (i == part_highlight) printf("\033[0m");
        printf("\n");
    }
    printf("\n");

    // Menu
    char menu_line[512] = "";
    for (int i = 0; i < n_menu_choices; i++) {
        char buf[64];
        if (i == menu_highlight) {
            snprintf(buf, sizeof(buf), "[\033[7m%s\033[0m] ", menu_choices[i]);
        } else {
            snprintf(buf, sizeof(buf), "[%s] ", menu_choices[i]);
        }
        strncat(menu_line, buf, sizeof(menu_line) - strlen(menu_line) - 1);
    }
    print_centered(menu_line);
}

int main() {
    const char *disk = "/dev/sda";
    const char *size_info =
        "Size: 40 GiB, 85438958 sectors, 5487787239487 bytes";

    enable_raw_mode();
    hide_cursor();

    int part_highlight = 0;
    int menu_highlight = 0;
    int running = 1;

    while (running) {
        show_diskman(part_highlight, menu_highlight, disk, size_info);

        char c = getchar();
        if (c == '\033') {
            getchar(); // skip '['
            char dir = getchar();
            if (dir == 'A') // Up
                part_highlight = (part_highlight - 1 + n_partitions) % n_partitions;
            else if (dir == 'B') // Down
                part_highlight = (part_highlight + 1) % n_partitions;
            else if (dir == 'C') // Right
                menu_highlight = (menu_highlight + 1) % n_menu_choices;
            else if (dir == 'D') // Left
                menu_highlight = (menu_highlight - 1 + n_menu_choices) % n_menu_choices;
        } else if (c == '\n') {
            clear_screen();
            printf("Partition: %s\n", partitions[part_highlight].device);
            printf("Action: %s\n\n", menu_choices[menu_highlight]);
            fflush(stdout);

            if (strcmp(menu_choices[menu_highlight], "Quit") == 0) {
                running = 0;
            } else {
                printf("Press Enter to return to menu...");
                fflush(stdout);
                while (getchar() != '\n');
            }
        }
    }

    show_cursor();
    disable_raw_mode();
    printf("Exiting...\n");
    return 0;
}
