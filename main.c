#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <ncurses.h>
#include <time.h>
#include <limits.h>

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


// =============MAP===========

#define MAP_WIDTH 80
#define MAP_HEIGHT 24
#define MAX_ROOMS 10
#define MIN_ROOMS 6
#define ROOM_MIN_SIZE 6
#define ROOM_MAX_SIZE 12

typedef struct {
    int x, y;
    int width, height;
    bool discovered;
} Room;

typedef struct {
    char tiles[MAP_HEIGHT][MAP_WIDTH];
    bool discovered[MAP_HEIGHT][MAP_WIDTH];
    Room rooms[MAX_ROOMS];
    int room_count;
    int stairs_x, stairs_y;
} Map;

typedef struct {
    int x, y;
    int hp;
    int gold;
    int keys;
    bool map_revealed;
} Player;

Map current_map;
Player player;

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




void init_map();
bool can_place_room(Room room);
void create_room(Room room);
void generate_map();
void draw_map();
void game_play();
void discover_current_room(int px, int py);
void place_door(Room *room); 


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
                game_play();
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
                game_play();
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

    mvprintw(0, (cols - strlen("===== Scoreboard =====")) / 2, "===== Scoreboard =====");

    // نمایش هدر جدول
    mvprintw(2, 5, "Rank");
    mvprintw(2, 20, "Username");
    mvprintw(2, 40, "Score");
    mvprintw(2, 55, "Gold");
    mvprintw(2, 65, "Last Played");
    mvprintw(2, 85, "Title"); // ستون جدید برای عنوان

    for (int i = 0; i < user_count; i++) {
        int y = 4 + i;

        // رنگ‌آمیزی برای سه رتبه برتر
        if (i < 3) {
            attron(COLOR_PAIR(i + 1));
        }

        // نمایش رتبه
        mvprintw(y, 5, "%d", i + 1);

        // نمایش نام کاربری
        if (strcmp(users[i].username, logged_in_user) == 0) {
            attron(A_BOLD);
        }
        mvprintw(y, 20, "%s", users[i].username);
        if (strcmp(users[i].username, logged_in_user) == 0) {
            attroff(A_BOLD);
        }

        // نمایش امتیاز و طلا
        mvprintw(y, 40, "%d", users[i].score);
        mvprintw(y, 55, "%d", users[i].gold);

        // نمایش زمان آخرین بازی
        char time_str[20];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", localtime(&users[i].last_played_time));
        mvprintw(y, 65, "%s", time_str);

        // نمایش عنوان برای سه رتبه برتر
        if (i == 0) {
            mvprintw(y, 85, "Legend");
        } else if (i == 1) {
            mvprintw(y, 85, "Master");
        } else if (i == 2) {
            mvprintw(y, 85, "Pro");
        }

        // بازنشانی رنگ‌ها
        if (i < 3) {
            attroff(COLOR_PAIR(i + 1));
        }
    }

    mvprintw(rows - 2, 5, "Press ESC to return...");
    refresh();

    // منتظر فشار دادن کلید ESC
    int ch;
    while ((ch = getch()) != 27 && ch != '\n') {}

    pre_game_menu();
}


// توابع کمکی برای تولید نقشه
void init_map() {
    // ایجاد نقشه خالی با دیوارها
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            current_map.tiles[y][x] = '#';
            current_map.discovered[y][x] = false;
        }
    }
}

bool can_place_room(Room room) {
    // بررسی اندازه اتاق
    if (room.width < ROOM_MIN_SIZE || room.height < ROOM_MIN_SIZE) {
        return false;
    }
    
    // بررسی تداخل با اتاق‌های موجود
    for (int i = 0; i < current_map.room_count; i++) {
        Room existing = current_map.rooms[i];
        if (room.x < existing.x + existing.width + 2 &&
            room.x + room.width > existing.x - 2 &&
            room.y < existing.y + existing.height + 2 &&
            room.y + room.height > existing.y - 2) {
            return false;
        }
    }
    return true;
}

void create_room(Room room) {
    // ایجاد اتاق با دیوار کامل
    for (int y = room.y; y < room.y + room.height; y++) {
        for (int x = room.x; x < room.x + room.width; x++) {
            if (x == room.x || x == room.x + room.width - 1 ||
                y == room.y || y == room.y + room.height - 1) {
                current_map.tiles[y][x] = '#'; // دیوار
            } else {
                current_map.tiles[y][x] = '*'; // کف اتاق
            }
        }
    }
    current_map.rooms[current_map.room_count++] = room;
}

void place_door(Room *room) {
    int directions[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
    int best_door_x = -1, best_door_y = -1;
    int min_adjacent_doors = INT_MAX;

    // بررسی تمام جهات برای یافتن بهترین مکان در
    for (int d = 0; d < 4; d++) {
        int dx = directions[d][0];
        int dy = directions[d][1];
        
        int door_x = room->x + (dx == 0 ? room->width / 2 : (dx == 1 ? room->width - 1 : 0));
        int door_y = room->y + (dy == 0 ? room->height / 2 : (dy == 1 ? room->height - 1 : 0));

        // بررسی مرزهای نقشه
        if (door_x <= 0 || door_x >= MAP_WIDTH-1 || door_y <= 0 || door_y >= MAP_HEIGHT-1)
            continue;

        // شمارش درهای مجاور
        int adjacent_doors = 0;
        for (int y = door_y - 1; y <= door_y + 1; y++) {
            for (int x = door_x - 1; x <= door_x + 1; x++) {
                if (current_map.tiles[y][x] == '.') {
                    adjacent_doors++;
                }
            }
        }

        // انتخاب مکانی با کمترین درهای مجاور
        if (adjacent_doors < min_adjacent_doors) {
            min_adjacent_doors = adjacent_doors;
            best_door_x = door_x;
            best_door_y = door_y;
        }
    }

    // قرار دادن در اگر مکانی مناسب پیدا شد
    if (best_door_x != -1 && best_door_y != -1) {
        current_map.tiles[best_door_y][best_door_x] = '.';
    }
}

void generate_map() {
    init_map();
    current_map.room_count = 0;
    srand(time(NULL));

    // تولید اتاق‌ها
    int attempts = 0;
    while (current_map.room_count < MIN_ROOMS && attempts < 1000) {
        Room room;
        room.width = ROOM_MIN_SIZE + rand() % (ROOM_MAX_SIZE - ROOM_MIN_SIZE);
        room.height = ROOM_MIN_SIZE + rand() % (ROOM_MAX_SIZE - ROOM_MIN_SIZE);
        room.x = 1 + rand() % (MAP_WIDTH - room.width - 2);
        room.y = 1 + rand() % (MAP_HEIGHT - room.height - 2);
        room.discovered = false;

        if (can_place_room(room)) {
            create_room(room);
            attempts = 0;
        } else {
            attempts++;
        }
    }
    while (current_map.room_count < MIN_ROOMS) {
        Room room;
        room.width = ROOM_MIN_SIZE + rand() % (ROOM_MAX_SIZE - ROOM_MIN_SIZE + 1);
        room.height = ROOM_MIN_SIZE + rand() % (ROOM_MAX_SIZE - ROOM_MIN_SIZE + 1);
        
        // محاسبه موقعیت با احتساب اندازه اتاق
        room.x = 1 + rand() % (MAP_WIDTH - room.width - 2);
        room.y = 1 + rand() % (MAP_HEIGHT - room.height - 2);
        room.discovered = false;

        if (can_place_room(room)) {
            create_room(room);
        }
    }

    // ایجاد آرایه برای الگوریتم MST
    int connections[MAX_ROOMS] = {0};
    int costs[MAX_ROOMS][MAX_ROOMS] = {0};

    // محاسبه هزینه اتصال بین اتاق‌ها
    for (int i = 0; i < current_map.room_count; i++) {
        for (int j = i+1; j < current_map.room_count; j++) {
            int x1 = current_map.rooms[i].x + current_map.rooms[i].width/2;
            int y1 = current_map.rooms[i].y + current_map.rooms[i].height/2;
            int x2 = current_map.rooms[j].x + current_map.rooms[j].width/2;
            int y2 = current_map.rooms[j].y + current_map.rooms[j].height/2;
            
            costs[i][j] = abs(x1 - x2) + abs(y1 - y2);
        }
    }

    // الگوریتم Prim برای MST
    int selected[MAX_ROOMS] = {0};
    selected[0] = 1;
    
    for (int connections = 1; connections < current_map.room_count; connections++) {
        int min_cost = INT_MAX;
        int from = -1, to = -1;
        
        for (int i = 0; i < current_map.room_count; i++) {
            if (selected[i]) {
                for (int j = 0; j < current_map.room_count; j++) {
                    if (!selected[j] && costs[i][j] > 0 && costs[i][j] < min_cost) {
                        min_cost = costs[i][j];
                        from = i;
                        to = j;
                    }
                }
            }
        }
        
        if (from != -1 && to != -1) {
            selected[to] = 1;
            // اتصال اتاق‌ها
            Room prev = current_map.rooms[from];
            Room current = current_map.rooms[to];
            
            int x1 = prev.x + prev.width/2;
            int y1 = prev.y + prev.height/2;
            int x2 = current.x + current.width/2;
            int y2 = current.y + current.height/2;
            
            // ایجاد راهرو افقی
            while (x1 != x2) {
                if (current_map.tiles[y1][x1] == '#') {
                    current_map.tiles[y1][x1] = '.';
                }
                x1 += (x1 < x2) ? 1 : -1;
            }
            
            // ایجاد راهرو عمودی
            while (y1 != y2) {
                if (current_map.tiles[y1][x1] == '#') {
                    current_map.tiles[y1][x1] = '.';
                }
                y1 += (y1 < y2) ? 1 : -1;
            }
            
            place_door(&prev);
            place_door(&current);
        }
    }

    // قرار دادن درهای اضافی برای اتاق‌های بدون در
    for (int i = 0; i < current_map.room_count; i++) {
        Room *room = &current_map.rooms[i];
        bool has_door = false;
        
        // بررسی وجود در در اتاق
        for (int y = room->y; y < room->y + room->height; y++) {
            for (int x = room->x; x < room->x + room->width; x++) {
                if (current_map.tiles[y][x] == '.') {
                    has_door = true;
                    break;
                }
            }
            if (has_door) break;
        }
        
        // اگر اتاق در نداشت، یک در اضافه کنید
        if (!has_door) {
            int wall = rand() % 4; // انتخاب تصادفی یک دیوار
            int door_x, door_y;
            
            switch(wall) {
                case 0: // بالایی
                    door_x = room->x + 1 + rand() % (room->width - 2);
                    door_y = room->y;
                    break;
                case 1: // پایینی
                    door_x = room->x + 1 + rand() % (room->width - 2);
                    door_y = room->y + room->height - 1;
                    break;
                case 2: // چپ
                    door_x = room->x;
                    door_y = room->y + 1 + rand() % (room->height - 2);
                    break;
                case 3: // راست
                    door_x = room->x + room->width - 1;
                    door_y = room->y + 1 + rand() % (room->height - 2);
                    break;
            }
            
            // بررسی مجاورت با درهای دیگر
            bool safe = true;
            for (int y = door_y - 1; y <= door_y + 1; y++) {
                for (int x = door_x - 1; x <= door_x + 1; x++) {
                    if (y >= 0 && y < MAP_HEIGHT && x >= 0 && x < MAP_WIDTH) {
                        if (current_map.tiles[y][x] == '.') {
                            safe = false;
                            break;
                        }
                    }
                }
                if (!safe) break;
            }
            
            if (safe) {
                current_map.tiles[door_y][door_x] = '.';
                // ایجاد راهرو به صورت تصادفی
                int dir = rand() % 4;
                int dx[] = {0, 0, -1, 1};
                int dy[] = {-1, 1, 0, 0};
                
                int nx = door_x + dx[dir];
                int ny = door_y + dy[dir];
                
                if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT) {
                    current_map.tiles[ny][nx] = '.';
                }
            }
        }
    }

    // قرار دادن پله
    Room last_room = current_map.rooms[current_map.room_count-1];
    current_map.stairs_x = last_room.x + last_room.width/2;
    current_map.stairs_y = last_room.y + last_room.height/2;
    current_map.tiles[current_map.stairs_y][current_map.stairs_x] = '>';
}

// تابع نمایش نقشه
void draw_map() {
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (current_map.discovered[y][x] || player.map_revealed) {
                attron(COLOR_PAIR(1));
                mvaddch(y, x, current_map.tiles[y][x]);
                attroff(COLOR_PAIR(1));
            } else {
                mvaddch(y, x, ' ');
            }
        }
    }
}

// ============Game_Play============
void game_play() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK); // رنگ پیش‌فرض
    init_pair(2, COLOR_RED, COLOR_BLACK);   // برای تله‌ها
    init_pair(3, COLOR_GREEN, COLOR_BLACK); // برای درها

    generate_map();
    player.x = current_map.rooms[0].x + 1;
    player.y = current_map.rooms[0].y + 1;
    player.hp = 100;
    player.gold = 0;
    player.keys = 0;
    player.map_revealed = false;

    int ch;
    while ((ch = getch()) != 'q') {
        clear();
        
        // پردازش ورودی
        int dx = 0, dy = 0;
        switch(ch) {
            case KEY_LEFT:  dx = -1; break;
            case KEY_RIGHT: dx = 1; break;
            case KEY_UP:    dy = -1; break;
            case KEY_DOWN:  dy = 1; break;
            case 'm': player.map_revealed = !player.map_revealed; break;
        }

        // بررسی حرکت مجاز
        int new_x = player.x + dx;
        int new_y = player.y + dy;
        if (current_map.tiles[new_y][new_x] != '#') {
            player.x = new_x;
            player.y = new_y;
            current_map.discovered[new_y][new_x] = true;
        }

        // برخورد با پله
        if (player.x == current_map.stairs_x && player.y == current_map.stairs_y) {
            generate_map(); // تولید طبقه جدید
            player.x = current_map.rooms[0].x + 1;
            player.y = current_map.rooms[0].y + 1;
        }

        // رسم رابط کاربری
        draw_map();
        mvprintw(0, 0, "Level:1  Hits:%d  Gold:%d  Keys:%d", player.hp, player.gold, player.keys);
        mvprintw(1, 0, "Press 'm' to toggle map | 'q' to quit");

        // نمایش بازیکن
        attron(COLOR_PAIR(3));
        mvaddch(player.y, player.x, '@');
        attroff(COLOR_PAIR(3));

        discover_current_room(player.x, player.y);

        refresh();
    }

    endwin();
}

// اضافه کردن این تابع برای بررسی اتاق فعلی بازیکن و کشف آن
void discover_current_room(int px, int py) {
    for (int i = 0; i < current_map.room_count; i++) {
        Room r = current_map.rooms[i];
        // بررسی آیا بازیکن در این اتاق است
        if (px >= r.x && px < r.x + r.width &&
            py >= r.y && py < r.y + r.height) {
            
            // کشف تمام تایل‌های اتاق
            for (int y = r.y; y < r.y + r.height; y++) {
                for (int x = r.x; x < r.x + r.width; x++) {
                    current_map.discovered[y][x] = true;
                }
            }
            
            // کشف درهای اطراف اتاق
            for (int y = r.y - 1; y <= r.y + r.height; y++) {
                for (int x = r.x - 1; x <= r.x + r.width; x++) {
                    if (current_map.tiles[y][x] == '+') {
                        current_map.discovered[y][x] = true;
                    }
                }
            }
            break;
        }
    }
}
