#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <ncurses.h>
#include <time.h>

#define USERS_FILE "/home/cumrun/rogue_game/users/users.txt"
#define MAX_USERS 100

typedef struct {
    char username[50];
    char password[50];
    char email[100];
    int score;
    int gold;
    int games_played;
    time_t last_played_time;
} User;

User users[MAX_USERS];
int user_count = 0;
char logged_in_user[50] = "";

// تابع‌های کمکی
int get_input(char *buffer, int max_len);
void display_menu();
bool is_valid_password(const char *password, char errors[][100], int *error_count);
bool is_valid_email(const char *email);
bool is_username_taken(const char *username);
void register_user();
void start_game();
void login_user();
void pre_game_menu();
void load_users();
void save_users();
int compare_users(const void *a, const void *b);
void display_scoreboard();

int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    start_color();

    init_pair(1, COLOR_BLACK, COLOR_YELLOW); // طلایی
    init_pair(2, COLOR_BLACK, COLOR_WHITE);  // نقره‌ای
    init_pair(3, COLOR_BLACK, COLOR_RED);    // برنزی

    load_users(); // بارگذاری کاربران در شروع برنامه
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

   fprintf(fp, "%s,%s,%s,0,0,0,%ld\n", 
        username, 
        password, 
        email,
        (long)time(NULL));
    fclose(fp);
    load_users();
    clear();
    mvprintw(rows / 2, (cols - strlen("User registered successfully!")) / 2, "User registered successfully!");
    refresh();
    napms(1500);
}

void start_game() {
    int choice;
    int rows, cols;
    while (1) {
        clear();
        getmaxyx(stdscr, rows, cols);
        mvprintw(rows / 2 - 3, (cols - strlen("===== Start Game Menu =====")) / 2, "===== Start Game Menu =====");
        mvprintw(rows / 2 - 1, (cols - strlen("1. Play as a Guest")) / 2, "1. Play as a Guest");
        mvprintw(rows / 2, (cols - strlen("2. Login")) / 2, "2. Login");
        mvprintw(rows / 2 + 1, (cols - strlen("3. Register New User")) / 2, "3. Register New User");
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
                clear();
                mvprintw(rows / 2, (cols - strlen("Starting game as a guest...")) / 2, "Starting game as a guest...");
                refresh();
                napms(2000);
                // اضافه کردن منطق بازی برای مهمان
                return;
            case 2:
                login_user(); // فراخوانی تابع لاگین
                return;
            case 3:
                register_user();
                return;
            default:
                clear();
                mvprintw(rows / 2, (cols - strlen("Invalid choice. Please try again.")) / 2, "Invalid choice. Please try again.");
                refresh();
                napms(1500);
        }
    }
}

void login_user() {
    int rows, cols;
    char username[50];
    char password[50];
    char stored_username[50], stored_password[50], stored_email[100];
    int stored_score, stored_gold, stored_games_played;
    time_t stored_last_played_time;
    bool user_found = false;

    // درخواست نام کاربری
    while (1) {
        clear();
        getmaxyx(stdscr, rows, cols);
        mvprintw(rows / 2 - 3, (cols - strlen("===== Login =====")) / 2, "===== Login =====");
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
            mvprintw(rows / 2, (cols - strlen("Username cannot be empty.")) / 2, "Username cannot be empty.");
            refresh();
            napms(1500);
            continue;
        }

        // بررسی وجود نام کاربری در فایل
        FILE *fp = fopen(USERS_FILE, "r");
        if (fp == NULL) {
            clear();
            mvprintw(rows / 2, (cols - strlen("Error: Could not open users file.")) / 2, "Error: Could not open users file.");
            refresh();
            napms(1500);
            return;
        }

        user_found = false;
        while (fscanf(fp, "%49[^,],%49[^,],%99[^,],%d,%d,%d,%ld\n",
                      stored_username, stored_password, stored_email,
                      &stored_score, &stored_gold, &stored_games_played,
                      &stored_last_played_time) != EOF) {
            if (strcmp(stored_username, username) == 0) {
                user_found = true;
                break;
            }
        }
        fclose(fp);

        if (!user_found) {
            clear();
            mvprintw(rows / 2, (cols - strlen("Error: Username not found.")) / 2, "Error: Username not found.");
            refresh();
            napms(1500);
            continue;
        }
        break;
    }

    // درخواست رمز عبور
    while (1) {
        clear();
        getmaxyx(stdscr, rows, cols);
        mvprintw(rows / 2 - 3, (cols - strlen("===== Login =====")) / 2, "===== Login =====");
        mvprintw(rows / 2 - 1, (cols - strlen("Enter Password: ")) / 2, "Enter Password: ");
        refresh();
        move(rows / 2 - 1, (cols - strlen("Enter Password: ")) / 2 + strlen("Enter Password: "));

        if (!get_input(password, sizeof(password))) {
            clear();
            mvprintw(rows / 2, (cols - strlen("Returning to main menu.")) / 2, "Returning to main menu.");
            refresh();
            napms(1500);
            return;
        }

        if (strlen(password) == 0) {
            clear();
            mvprintw(rows / 2, (cols - strlen("Password cannot be empty.")) / 2, "Password cannot be empty.");
            refresh();
            napms(1500);
            continue;
        }

        // بررسی رمز عبور
        if (strcmp(stored_password, password) == 0) {
            // موفقیت در لاگین
            strcpy(logged_in_user, username);

            // به‌روزرسانی زمان آخرین بازی
            FILE *fp = fopen(USERS_FILE, "r+");
            if (fp != NULL) {
                fseek(fp, 0, SEEK_SET);
                for (int i = 0; i < user_count; i++) {
                    if (strcmp(users[i].username, username) == 0) {
                        users[i].last_played_time = time(NULL);
                        save_users(); // ذخیره تغییرات
                        break;
                    }
                }
                fclose(fp);
            }

            clear();
            mvprintw(rows / 2, (cols - strlen("Login successful! Redirecting to pre-game menu...")) / 2, "Login successful! Redirecting to pre-game menu...");
            refresh();
            napms(2000);
            pre_game_menu(); // انتقال به منوی قبل از بازی
            return;
        } else {
            clear();
            mvprintw(rows / 2, (cols - strlen("Error: Incorrect password.")) / 2, "Error: Incorrect password.");
            refresh();
            napms(1500);
            continue;
        }
    }
}



void pre_game_menu() {
    int choice;
    int rows, cols;
    while (1) {
        clear();
        getmaxyx(stdscr, rows, cols);
        mvprintw(rows / 2 - 4, (cols - strlen("===== Pre-Game Menu =====")) / 2, "===== Pre-Game Menu =====");
        mvprintw(rows / 2 - 2, (cols - strlen("1. New Game")) / 2, "1. New Game");
        mvprintw(rows / 2 - 1, (cols - strlen("2. Resume Game")) / 2, "2. Resume Game");
        mvprintw(rows / 2, (cols - strlen("3. Setting")) / 2, "3. Setting");
        mvprintw(rows / 2 + 1, (cols - strlen("4. Scoreboard")) / 2, "4. Scoreboard");
        mvprintw(rows / 2 + 3, (cols - strlen("======================")) / 2, "======================");
        mvprintw(rows / 2 + 5, (cols - strlen("Enter your choice (ESC to exit): ")) / 2, "Enter your choice (ESC to exit): ");
        refresh();
        
        char input[10];
        move(rows / 2 + 5, (cols - strlen("Enter your choice (ESC to exit): ")) / 2 + strlen("Enter your choice (ESC to exit): "));
        
        // اگر کاربر ESC بزند، به منوی اصلی برگرد
        if (!get_input(input, sizeof(input))) {
            return; // خروج از تابع و بازگشت به منوی اصلی
        }
        
        choice = atoi(input);
        switch (choice) {
            case 1:
                clear();
                mvprintw(rows / 2, (cols - strlen("New Game selected. Logic to be implemented.")) / 2, "New Game selected. Logic to be implemented.");
                refresh();
                napms(2000);
                return;
            case 2:
                clear();
                mvprintw(rows / 2, (cols - strlen("Resume Game selected. Logic to be implemented.")) / 2, "Resume Game selected. Logic to be implemented.");
                refresh();
                napms(2000);
                return;
            case 3:
                clear();
                mvprintw(rows / 2, (cols - strlen("Setting selected. Logic to be implemented.")) / 2, "Setting selected. Logic to be implemented.");
                refresh();
                napms(2000);
                return;
            case 4:
                display_scoreboard(); // نمایش جدول امتیازات
                return;
            default:
                clear();
                mvprintw(rows / 2, (cols - strlen("Invalid choice. Please try again.")) / 2, "Invalid choice. Please try again.");
                refresh();
                napms(1500);
        }
    }
}

void load_users() {
    FILE *fp = fopen(USERS_FILE, "r");
    if (fp == NULL) return;

    user_count = 0;
    while (fscanf(fp, "%49[^,],%49[^,],%99[^,],%d,%d,%d,%ld\n",
                  users[user_count].username,
                  users[user_count].password,
                  users[user_count].email,
                  &users[user_count].score,
                  &users[user_count].gold,
                  &users[user_count].games_played,
                  &users[user_count].last_played_time) == 7) {
        user_count++;
        if (user_count >= MAX_USERS) break;
    }
    fclose(fp);
}

void save_users() {
    FILE *fp = fopen(USERS_FILE, "w");
    if (fp == NULL) return;

    for (int i = 0; i < user_count; i++) {
        fprintf(fp, "%s,%s,%s,%d,%d,%d,%ld\n",
                users[i].username,
                users[i].password,
                users[i].email,
                users[i].score,
                users[i].gold,
                users[i].games_played,
                users[i].last_played_time);
    }
    fclose(fp);
}

int compare_users(const void *a, const void *b) {
    User *user_a = (User *)a;
    User *user_b = (User *)b;

    if (user_a->score != user_b->score) {
        return user_b->score - user_a->score;
    }
    if (user_a->gold != user_b->gold) {
        return user_b->gold - user_a->gold;
    }
    return strcmp(user_a->username, user_b->username);
}

void display_scoreboard() {
    load_users();
    qsort(users, user_count, sizeof(User), compare_users);

    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    clear();

    mvprintw(0, (cols - 25)/2, "===== Scoreboard =====");
    
    // هدر جدول
    mvprintw(2, 5, "Rank");
    mvprintw(2, 20, "Username");
    mvprintw(2, 40, "Score");
    mvprintw(2, 55, "Gold");
    mvprintw(2, 65, "Last Played");

    for (int i = 0; i < user_count; i++) {
        int y = 4 + i;
        
        // رنگ‌آمیزی رتبه‌ها
        if (i < 3) attron(COLOR_PAIR(i+1));
        
        // نمایش رتبه
        mvprintw(y, 5, "%d", i+1);
        
        // نمایش نام کاربری با هایلایت اگر کاربر فعلی باشد
        if (strcmp(users[i].username, logged_in_user) == 0) 
            attron(A_BOLD | A_UNDERLINE);
        mvprintw(y, 20, "%s", users[i].username);
        if (strcmp(users[i].username, logged_in_user) == 0) 
            attroff(A_BOLD | A_UNDERLINE);
        
        // نمایش اطلاعات
        mvprintw(y, 40, "%d", users[i].score);
        mvprintw(y, 55, "%d", users[i].gold);
        
        // نمایش زمان
        char time_buf[20];
        strftime(time_buf, 20, "%Y-%m-%d %H:%M", localtime(&users[i].last_played_time));
        mvprintw(y, 65, "%s", time_buf);
        
        // بازگردانی رنگ
        if (i < 3) attroff(COLOR_PAIR(i+1));
    }
    
    mvprintw(rows-2, 5, "Press any key to return...");
    refresh();
    getch();
    pre_game_menu();
}
