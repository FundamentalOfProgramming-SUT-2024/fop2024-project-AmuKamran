#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <ncurses.h>

#define USERS_FILE "/home/cumrun/rogue_game/users/users.txt"

int get_input(char *buffer, int max_len);
void display_menu();
bool is_valid_password(const char *password, char errors[][100], int *error_count);
bool is_valid_email(const char *email);
bool is_username_taken(const char *username);
void register_user();
void start_game();

int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    display_menu();
    endwin();
    return 0;
}

int get_input(char *buffer, int max_len) {
    int ch, idx = 0;
    while ((ch = getch()) != '\n') {
        if (ch == 27) {
            buffer[0] = '\0';
            return 0;
        }
        if (ch == KEY_BACKSPACE || ch == 127) {
            if (idx > 0) {
                idx--;
                int y, x;
                getyx(stdscr, y, x);
                mvaddch(y, x - 1, ' ');
                move(y, x - 1);
                refresh();
            }
        } else if (isprint(ch) && idx < max_len - 1) {
            buffer[idx++] = ch;
            addch(ch);
            refresh();
        }
    }
    buffer[idx] = '\0';
    return 1;
}

void display_menu() {
    int choice;
    int rows, cols;
    while (1) {
        clear();
        getmaxyx(stdscr, rows, cols);
        mvprintw(rows / 2 - 3, (cols - strlen("===== Game Menu =====")) / 2, "===== Game Menu =====");
        mvprintw(rows / 2 - 1, (cols - strlen("1. Register New User")) / 2, "1. Register New User");
        mvprintw(rows / 2, (cols - strlen("2. Start Game")) / 2, "2. Start Game");
        mvprintw(rows / 2 + 1, (cols - strlen("3. Exit")) / 2, "3. Exit");
        mvprintw(rows / 2 + 3, (cols - strlen("======================")) / 2, "======================");
        mvprintw(rows / 2 + 5, (cols - strlen("Enter your choice: ")) / 2, "Enter your choice: ");
        refresh();
        char input[10];
        move(rows / 2 + 5, (cols - strlen("Enter your choice: ")) / 2 + strlen("Enter your choice: "));
        if (!get_input(input, sizeof(input))) {
            continue;
        }
        choice = atoi(input);
        switch (choice) {
            case 1:
                register_user();
                break;
            case 2:
                start_game();
                break;
            case 3:
                clear();
                mvprintw(rows / 2, (cols - strlen("Goodbye!")) / 2, "Goodbye!");
                refresh();
                napms(1500);
                return;
            default:
                clear();
                mvprintw(rows / 2, (cols - strlen("Invalid choice. Please try again.")) / 2, "Invalid choice. Please try again.");
                refresh();
                napms(1500);
        }
    }
}

bool is_valid_password(const char *password, char errors[][100], int *error_count) {
    *error_count = 0;
    if (strlen(password) < 7) {
        strcpy(errors[(*error_count)++], "• At least 7 characters");
    }
    bool has_digit = false, has_upper = false, has_lower = false;
    for (int i = 0; password[i] != '\0'; i++) {
        if (isdigit(password[i])) has_digit = true;
        if (isupper(password[i])) has_upper = true;
        if (islower(password[i])) has_lower = true;
    }
    if (!has_digit) {
        strcpy(errors[(*error_count)++], "• At least one digit");
    }
    if (!has_upper) {
        strcpy(errors[(*error_count)++], "• At least one uppercase letter");
    }
    if (!has_lower) {
        strcpy(errors[(*error_count)++], "• At least one lowercase letter");
    }
    return (*error_count) == 0;
}

bool is_valid_email(const char *email) {
    const char *at = strchr(email, '@');
    if (at == NULL) return false;
    const char *dot = strchr(at, '.');
    if (dot == NULL || dot == at + 1 || *(dot + 1) == '\0' || at == email) return false;
    return true;
}

bool is_username_taken(const char *username) {
    FILE *fp = fopen(USERS_FILE, "r");
    if (fp == NULL) return false;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        char stored_username[50];
        sscanf(line, "%49[^,],%*[^,],%*s", stored_username);
        if (strcmp(stored_username, username) == 0) {
            fclose(fp);
            return true;
        }
    }
    fclose(fp);
    return false;
}

void register_user() {
    int rows, cols;
    char username[50];
    char password[50];
    char email[100];
    while (1) {
        clear();
        getmaxyx(stdscr, rows, cols);
        mvprintw(rows / 2 - 3, (cols - strlen("===== Register New User =====")) / 2, "===== Register New User =====");
        mvprintw(rows / 2 - 1, (cols - strlen("Enter Username: ")) / 2, "Enter Username: ");
        refresh();
        move(rows / 2 - 1, (cols - strlen("Enter Username: ")) / 2 + strlen("Enter Username: "));
        if (!get_input(username, sizeof(username))) {
            clear();
            mvprintw(rows / 2, (cols - strlen("Returning to main menu.")) / 2, "Returning to main menu.");
            refresh();
            napms(1500);
            return;
        }
        if (strlen(username) == 0) {
            clear();
            mvprintw(rows / 2, (cols - strlen("Returning to main menu.")) / 2, "Returning to main menu.");
            refresh();
            napms(1500);
            return;
        }
        if (is_username_taken(username)) {
            clear();
            mvprintw(rows / 2 - 1, (cols - strlen("Error: Username already exists.")) / 2, "Error: Username already exists.");
            mvprintw(rows / 2, (cols - strlen("Press Enter to try again.")) / 2, "Press Enter to try again.");
            refresh();
            while (getch() != '\n') {}
            continue;
        }
        break;
    }
    while (1) {
        clear();
        getmaxyx(stdscr, rows, cols);
        mvprintw(rows / 2 - 4, (cols - strlen("===== Register New User =====")) / 2, "===== Register New User =====");
        mvprintw(rows / 2 - 2, (cols - strlen("Password Requirements:")) / 2, "Password Requirements:");
        mvprintw(rows / 2 - 1, (cols - strlen("• At least 7 characters")) / 2, "• At least 7 characters");
        mvprintw(rows / 2, (cols - strlen("• At least one digit")) / 2, "• At least one digit");
        mvprintw(rows / 2 + 1, (cols - strlen("• At least one uppercase letter")) / 2, "• At least one uppercase letter");
        mvprintw(rows / 2 + 2, (cols - strlen("• At least one lowercase letter")) / 2, "• At least one lowercase letter");
        mvprintw(rows / 2 + 4, (cols - strlen("Enter Password: ")) / 2, "Enter Password: ");
        refresh();
        move(rows / 2 + 4, (cols - strlen("Enter Password: ")) / 2 + strlen("Enter Password: "));
        if (!get_input(password, sizeof(password))) {
            clear();
            mvprintw(rows / 2, (cols - strlen("Returning to main menu.")) / 2, "Returning to main menu.");
            refresh();
            napms(1500);
            return;
        }
        if (strlen(password) == 0) {
            clear();
            mvprintw(rows / 2, (cols - strlen("Returning to main menu.")) / 2, "Returning to main menu.");
            refresh();
            napms(1500);
            return;
        }
        char errors[4][100];
        int error_count;
        if (!is_valid_password(password, errors, &error_count)) {
            clear();
            for(int i=0;i<error_count;i++) {
                mvprintw(rows / 2 - 2 + i, (cols - strlen(errors[i])) / 2, "%s", errors[i]);
            }
            mvprintw(rows / 2 + 2, (cols - strlen("Press Enter to try again.")) / 2, "Press Enter to try again.");
            refresh();
            while (getch() != '\n') {}
            continue;
        }
        break;
    }
    while (1) {
        clear();
        getmaxyx(stdscr, rows, cols);
        mvprintw(rows / 2 - 3, (cols - strlen("===== Register New User =====")) / 2, "===== Register New User =====");
        mvprintw(rows / 2 - 1, (cols - strlen("Enter Email (format: x@y.z): ")) / 2, "Enter Email (format: x@y.z): ");
        refresh();
        move(rows / 2 - 1, (cols - strlen("Enter Email (format: x@y.z): ")) / 2 + strlen("Enter Email (format: x@y.z): "));
        if (!get_input(email, sizeof(email))) {
            clear();
            mvprintw(rows / 2, (cols - strlen("Returning to main menu.")) / 2, "Returning to main menu.");
            refresh();
            napms(1500);
            return;
        }
        if (strlen(email) == 0) {
            clear();
            mvprintw(rows / 2, (cols - strlen("Returning to main menu.")) / 2, "Returning to main menu.");
            refresh();
            napms(1500);
            return;
        }
        if (!is_valid_email(email)) {
            clear();
            mvprintw(rows / 2 - 1, (cols - strlen("Error: Invalid email format. Please use x@y.z")) / 2, "Error: Invalid email format. Please use x@y.z");
            mvprintw(rows / 2, (cols - strlen("Press Enter to try again.")) / 2, "Press Enter to try again.");
            refresh();
            while (getch() != '\n') {}
            continue;
        }
        break;
    }
    FILE *fp = fopen(USERS_FILE, "a");
    if (fp == NULL) {
        clear();
        getmaxyx(stdscr, rows, cols);
        mvprintw(rows / 2 - 1, (cols - strlen("Error: Could not open users file.")) / 2, "Error: Could not open users file.");
        mvprintw(rows / 2, (cols - strlen("Press Enter to return to main menu.")) / 2, "Press Enter to return to main menu.");
        refresh();
        while (getch() != '\n') {}
        return;
    }
    fprintf(fp, "%s,%s,%s\n", username, password, email);
    fclose(fp);
    clear();
    mvprintw(rows / 2, (cols - strlen("User registered successfully!")) / 2, "User registered successfully!");
    refresh();
    napms(1500);
}

void start_game() {
    int rows, cols;
    clear();
    getmaxyx(stdscr, rows, cols);
    mvprintw(rows / 2, (cols - strlen("Starting the game...")) / 2, "Starting the game...");
    refresh();
    napms(2000);
}
