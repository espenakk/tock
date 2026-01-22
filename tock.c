#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <sys/ioctl.h>

/* --- ASCII Art Digits (Height 7) --- */
const char *digits[10][7] = {
    { // 0
        "  #####  ",
        " ##   ## ",
        " ##   ## ",
        " ##   ## ",
        " ##   ## ",
        " ##   ## ",
        "  #####  "
    },
    { // 1
        "    #    ",
        "   ##    ",
        "    #    ",
        "    #    ",
        "    #    ",
        "    #    ",
        "  #####  "
    },
    { // 2
        "  #####  ",
        " ##   ## ",
        "      ## ",
        "  #####  ",
        " ##      ",
        " ##      ",
        " ####### "
    },
    { // 3
        "  #####  ",
        " #     # ",
        "       # ",
        "  #####  ",
        "       # ",
        " #     # ",
        "  #####  "
    },
    { // 4
        " #    #  ",
        " #    #  ",
        " #    #  ",
        " ####### ",
        "      #  ",
        "      #  ",
        "      #  "
    },
    { // 5
        " ####### ",
        " #       ",
        " #       ",
        " ######  ",
        "       # ",
        " #     # ",
        "  #####  "
    },
    { // 6
        "  #####  ",
        " #     # ",
        " #       ",
        " ######  ",
        " #     # ",
        " #     # ",
        "  #####  "
    },
    { // 7
        " ####### ",
        " #     # ",
        "      #  ",
        "     #   ",
        "    #    ",
        "    #    ",
        "    #    "
    },
    { // 8
        "  #####  ",
        " #     # ",
        " #     # ",
        "  #####  ",
        " #     # ",
        " #     # ",
        "  #####  "
    },
    { // 9
        "  #####  ",
        " #     # ",
        " #     # ",
        "  ###### ",
        "       # ",
        " #     # ",
        "  #####  "
    }
};

const char *colon[7] = {
    "   ",
    " # ",
    " # ",
    "   ",
    " # ",
    " # ",
    "   "
};

/* --- Terminal Handling --- */
struct termios orig_termios;

void reset_terminal_mode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    printf("\e[?25h"); // Show cursor
}

void handle_signal(int sig) {
    (void)sig; // unused
    reset_terminal_mode();
    exit(0);
}

void set_conio_terminal_mode() {
    struct termios new_termios;

    // take two copies - one for now, one for later
    tcgetattr(STDIN_FILENO, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    // register cleanup handler, and set the new terminal mode
    atexit(reset_terminal_mode);
    signal(SIGINT, handle_signal); // Handle Ctrl+C
    
    cfmakeraw(&new_termios);
    new_termios.c_lflag |= ISIG; // Allow Ctrl+C to interrupt
    new_termios.c_oflag |= OPOST; // Post-process output (newlines work)
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
    
    printf("\e[?25l"); // Hide cursor
}

int kbhit() {
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
}

int getch() {
    int r;
    unsigned char c;
    if ((r = read(STDIN_FILENO, &c, sizeof(c))) < 0) {
        return r;
    } else {
        return c;
    }
}

/* --- Time Helpers --- */
long long current_timestamp_ms() {
    struct timeval te; 
    gettimeofday(&te, NULL);
    return te.tv_sec * 1000LL + te.tv_usec / 1000;
}

void print_scaled_line(const char *line, int scale_x) {
    for (int i = 0; line[i] != '\0'; i++) {
        for (int k = 0; k < scale_x; k++) {
            putchar(line[i]);
        }
    }
}

void print_digit_line(int digit, int line, int scale_x) {
    print_scaled_line(digits[digit][line], scale_x);
}

void print_colon_line(int line, int scale_x) {
    print_scaled_line(colon[line], scale_x);
}

/* --- Main Logic --- */

int main(int argc, char *argv[]) {
    if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        printf("Usage: stopwatch [OPTIONS]\n\n");
        printf("A simple terminal stopwatch.\n\n");
        printf("Controls:\n");
        printf("  p         Pause/Resume\n");
        printf("  q         Quit\n");
        printf("  a         Add minutes (enter number, e.g. 30 or -5)\n");
        return 0;
    }

    set_conio_terminal_mode();

    long long start_time = current_timestamp_ms();
    long long pause_start_time = 0;
    long long total_paused_time = 0;
    long long time_offset = 0; // For adding/subtracting minutes
    int paused = 0;
    
    // Buffer for reading 'a' command input
    char input_buffer[32];
    int input_pos = 0;
    int awaiting_number = 0; // Flag if we are currently reading a number for 'a'

    // Clear screen initially
    printf("\033[2J");

    while (1) {
        long long now = current_timestamp_ms();
        long long current_elapsed;
        
        if (paused) {
            current_elapsed = (pause_start_time - start_time) - total_paused_time + time_offset;
        } else {
            current_elapsed = (now - start_time) - total_paused_time + time_offset;
        }

        // Calculate display values
        long long total_seconds = current_elapsed / 1000;
        int hours = total_seconds / 3600;
        int minutes = (total_seconds % 3600) / 60;

    // Move cursor to top-left
    printf("\033[H");
    
    if (paused) {
        printf("\033[31m"); // Red
    } else {
        printf("\033[0m"); // Reset
    }

    // --- Render ASCII Art ---
    // Format: HH:MM
    
    int d_h0 = hours / 10;
    int d_h1 = hours % 10;
    int d_m0 = minutes / 10;
    int d_m1 = minutes % 10;

    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int term_h = w.ws_row;
    int term_w = w.ws_col;
    
    // Base dimensions for HH:MM (4 digits width 9, 1 colon width 3)
    // 9*4 + 3 = 39. Plus some spacing maybe? currently digits have no extra margin in array, 
    // but the array strings have padding?
    // "  #####  " <- length 9.
    // So 39 width exactly.
    int base_w = 39;
    int base_h = 7;
    
    int scale_x = term_w / base_w;
    if (scale_x < 1) scale_x = 1;
    
    int scale_y = (term_h - 4) / base_h; // -4 for generic padding/prompt
    if (scale_y < 1) scale_y = 1;
    
    int total_w = base_w * scale_x;
    int total_h = base_h * scale_y;
    int pad_x = (term_w - total_w) / 2;
    int pad_y = (term_h - total_h - 2) / 2;
    if (pad_y < 0) pad_y = 0;

    // Vertical Padding
    for (int i = 0; i < pad_y; i++) {
        printf("\033[K\n");
    }

    for (int row = 0; row < 7 * scale_y; row++) {
        int font_row = row / scale_y;
        
        // Horizontal Padding
        for (int i = 0; i < pad_x; i++) putchar(' ');

        // Hours
        print_digit_line(d_h0, font_row, scale_x);
        print_digit_line(d_h1, font_row, scale_x);
        print_colon_line(font_row, scale_x);
        
        // Minutes
        print_digit_line(d_m0, font_row, scale_x);
        print_digit_line(d_m1, font_row, scale_x);

        // Clear rest of line
        printf("\033[K\r\n");
    }

    printf("\033[0m"); // Reset color
    printf("\033[K\n"); // Empty line after clock
    
    // Center the prompt too if we want, or just left align? 
    // Left align is safer for input reading UX usually, but visually better centered?
    // Let's keep prompt left/standard but clear the screen after it.
    
    // Add prompt
    if (awaiting_number) {
        printf("\033[KEnter minutes to add: %s_", input_buffer);
    } else {
        printf("\033[K"); 
    }
    
    // Clear everything below
    printf("\033[J");
    
    // Flush to ensure it draws immediately (though usleep/loop usually handles it, explicit flush is good)
    fflush(stdout);

    // Input Handling
    if (kbhit()) {
            char c = getch();
            
            if (awaiting_number) {
                if ((c >= '0' && c <= '9') || c == '-' || c == '+') {
                    if (input_pos < 30) {
                        input_buffer[input_pos++] = c;
                        input_buffer[input_pos] = '\0';
                    }
                } else if (c == 10 || c == 13) { // Enter
                    int minutes_to_add = atoi(input_buffer);
                    time_offset += minutes_to_add * 60 * 1000LL;
                    awaiting_number = 0;
                    input_buffer[0] = '\0';
                    input_pos = 0;
                } else if (c == 127 || c == 8) { // Backspace
                    if (input_pos > 0) {
                        input_buffer[--input_pos] = '\0';
                    }
                } else if (c == 27) { // Escape to cancel
                    awaiting_number = 0;
                    input_buffer[0] = '\0';
                    input_pos = 0;
                }
            } else {
                if (c == 'q' || c == 'Q') {
                    break;
                } else if (c == 'p' || c == 'P') {
                    if (paused) {
                        paused = 0;
                        total_paused_time += (now - pause_start_time);
                    } else {
                        paused = 1;
                        pause_start_time = now;
                    }
                } else if (c == 'a' || c == 'A') {
                    awaiting_number = 1;
                    input_buffer[0] = '\0';
                    input_pos = 0;
                }
            }
        }
        
        usleep(50000); // 50ms update rate approx 20fps
    }

    return 0;
}
