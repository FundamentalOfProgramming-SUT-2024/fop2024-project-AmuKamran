#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdbool.h>
#include <locale.h>
#include <time.h>
#include <limits.h>
#include<ncursesw/ncurses.h>

#define USERS_FILE "/home/cumrun/rogue_game/users/users.txt"
#define MAX_USERS 100
#define MAX_MESSAGES 5
#define MESSAGE_LENGTH 100
#define MAX_MONSTERS 6
#define MAX_WEAPONS 6
#define MAX_THROWN_WEAPONS 10
#define MAX_CHARMS 4

typedef struct {
    char username[50];
    char password[50];
    char email[100];
    int score;
    int gold;
    int food;
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
    bool is_cursed;
} Room;

typedef struct {
    int x;
    int y;
    char type;
    int hp;
    int power;
    int follow_steps;
    bool alive;
    char tile_beneath;
} monster;

typedef struct {
    char type;      
    int damage;     
    bool throwable;   
    int ammo; 
    int range;
    int x, y; 
} Weapon;

typedef struct {
    char type; 
    int x, y;
} Charm;

typedef struct {
    char tiles[MAP_HEIGHT][MAP_WIDTH];
    bool discovered[MAP_HEIGHT][MAP_WIDTH];
    Room rooms[MAX_ROOMS];
    int room_count;
    int stairs_x, stairs_y;
    int battle_gate_x, battle_gate_y;
    int prev_room_width; 
    int prev_room_height;
    Room cursed_room;
    int monster_count;
    monster monsters[MAX_MONSTERS];
    int weapon_count;
    Weapon weapons[MAX_WEAPONS];
    Weapon thrown_weapons[MAX_THROWN_WEAPONS];
    int thrown_weapon_count;
    Charm charms[MAX_CHARMS];
    int charm_count;
} Map;

typedef struct {
    int x, y;
    int hp;
    int gold;
    int food;
    int keys;
    int hunger;
    bool map_revealed;
    time_t last_heal_time;
    Weapon *current_weapon;
    Weapon *weapons[10];
    int weapon_count;
    int health_charms;
    int speed_charms;
    int damage_charms;
} Player;


Weapon weapons_list[] = {
    {'d', 12, true, 10, 5},  
    {'w', 15, true, 8, 10},  
    {'a', 2, true, 5, 10}, 
    {'s', 10, false, 1, 0}
};

Weapon default_weapon = {'m', 5, false, 1, 0};


Map current_map;
Player player;

monster monsters[MAX_MONSTERS];
int monster_count;
bool hited = false;
bool a_monster_has_been_killed = false;

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

char last_message[100] = "";
time_t last_msg_time = 0;

void init_map();
bool can_place_room(Room room);
void create_room(Room room);
void generate_map(bool keep_dimensions); 
void draw_map();
void game_play();
void discover_current_room(int px, int py);
void place_door(Room *room); 
void create_random_branch(int x, int y);
void add_corridor_walls(int x, int y);
void create_winding_corridor(int x1, int y1, int x2, int y2);
void display_message(const char *format, ...);

Room* find_room_for_monster(int x, int y);
void move_monsters();
void throw_weapon(Weapon *wp, int direction);

int main() {
    setlocale(LC_ALL,"");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    start_color();

    init_pair(4, COLOR_WHITE, COLOR_BLACK); 
    init_pair(2, COLOR_CYAN, COLOR_BLACK);  // نقره‌ای
    init_pair(3, COLOR_MAGENTA, COLOR_BLACK);    // برنزی
    init_pair(1, COLOR_YELLOW, COLOR_BLACK); // زرد
    init_pair(5, COLOR_RED, COLOR_BLACK); // نارنجی
    load_users(); // بارگذاری کاربران در شروع برنامه
    display_menu();
    endwin();
    return 0;
}

int get_input(char *buffer, int max_len) {
    int ch;
    int len = 0;
    while ((ch = getch()) != '\n') {
        if (ch == '\t') { 
            return -1;
        }
        if (ch == KEY_BACKSPACE || ch == 127) {
            if (len > 0) {
                len--;
                addch('\b');
                addch(' ');
                addch('\b');
            }
        }
        else if (isprint(ch) && len < max_len - 1) {
            addch(ch);
            buffer[len++] = ch;
        }
    }
    buffer[len] = '\0';
    return len;
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

void generate_random_password(char *password, int length) {
    const char lowercase[] = "abcdefghijklmnopqrstuvwxyz";
    const char uppercase[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const char digits[] = "0123456789";
    
    // اطمینان از وجود حداقل یک کاراکتر از هر نوع
    password[0] = lowercase[rand() % (sizeof(lowercase) - 1)];
    password[1] = uppercase[rand() % (sizeof(uppercase) - 1)];
    password[2] = digits[rand() % (sizeof(digits) - 1)];

    // پر کردن بقیه کاراکترها
    const char all_chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 3; i < length; i++) {
        password[i] = all_chars[rand() % (sizeof(all_chars) - 1)];
    }

    // به هم ریختن ترتیب کاراکترها
    for (int i = 0; i < length; i++) {
        int j = rand() % length;
        char temp = password[i];
        password[i] = password[j];
        password[j] = temp;
    }
    
    password[length] = '\0';
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
        mvprintw(rows / 2 + 5, (cols - strlen("Press TAB to generate random password")) / 2,"Press TAB to generate random password");
        mvprintw(rows / 2 + 4, (cols - strlen("Enter Password: ")) / 2, "Enter Password: ");
        refresh();
        move(rows / 2 + 4, (cols - strlen("Enter Password: ")) / 2 + strlen("Enter Password: "));
        int input_result = get_input(password, sizeof(password));
    
        if (input_result == -1) { // اگر کاربر Tab زده
            generate_random_password(password, 10);
            mvprintw(rows / 2 + 4, (cols - strlen("Enter Password: ")) / 2 + strlen("Enter Password: "), 
               "%s", password);
            refresh();
        
        while (getch() != '\n') {}
        }
        else if (input_result == 0) {
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

   fprintf(fp, "%s,%s,%s,0,0,0,0,%ld\n", 
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
        int nothing;
        user_found = false;
        while (fscanf(fp, "%49[^,],%49[^,],%99[^,],%d,%d,%d,%d,%ld\n",
                      stored_username, stored_password, stored_email,
                      &stored_score, &stored_gold,&nothing ,&stored_games_played,
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
    while (fscanf(fp, "%49[^,],%49[^,],%99[^,],%d,%d,%d,%d,%ld\n",
                  users[user_count].username,
                  users[user_count].password,
                  users[user_count].email,
                  &users[user_count].score,
                  &users[user_count].gold,
                  &users[user_count].food,
                  &users[user_count].games_played,
                  &users[user_count].last_played_time) == 8) {
        user_count++;
        if (user_count >= MAX_USERS) break;
    }
    fclose(fp);
}

void save_users() {
    FILE *fp = fopen(USERS_FILE, "w");
    if (fp == NULL) return;

    for (int i = 0; i < user_count; i++) {
        fprintf(fp, "%s,%s,%s,%d,%d,%d,%d,%ld\n",
                users[i].username,
                users[i].password,
                users[i].email,
                users[i].score,
                users[i].gold,
                users[i].food,
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
        if (i < 3) {
        attron(COLOR_PAIR(i + 1));
        const char* medals[] = {" 🥇", " 🥈", " 🥉"};
        mvprintw(y, 20, "%s%s", users[i].username, medals[i]);
        attroff(COLOR_PAIR(i + 1));
        } else {
        mvprintw(y, 20, "%s", users[i].username);
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
    current_map.monster_count = 0;
    memset(current_map.monsters, 0, sizeof(current_map.monsters));
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
    for (int y = room.y; y < room.y + room.height; y++) {
        for (int x = room.x; x < room.x + room.width; x++) {
            if (x == room.x || x == room.x + room.width - 1 ||
                y == room.y || y == room.y + room.height - 1) {
                current_map.tiles[y][x] = '#'; // دیوار
            } else {
                if (room.is_cursed) {
                 
                    current_map.tiles[y][x] = (rand() % 100 < 50) ? '^' : '*';
                } else {
                    current_map.tiles[y][x] = '*'; // کف معمولی
                }
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

bool is_valid_weapon_position(int x, int y) {
    // بررسی مرزهای نقشه
    if (x < 0 || x + 1 >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) {
        return false;
    }
    
    // بررسی اینکه هر دو خانه کف اتاق باشند
    if (current_map.tiles[y][x] != '*' || current_map.tiles[y][x + 1] != '*') {
        return false;
    }
    
    // بررسی تداخل با سایر اسلحه‌ها
    for (int i = 0; i < current_map.weapon_count; i++) {
        Weapon wp = current_map.weapons[i];
        if ((wp.x == x || wp.x == x + 1) && wp.y == y) {
            return false;
        }
    }
    
    // بررسی تداخل با پله، تله و سایر موجودیت‌ها
    if ((x == current_map.stairs_x && y == current_map.stairs_y) ||
        (x + 1 == current_map.stairs_x && y == current_map.stairs_y)) {
        return false;
    }
    if ((x == current_map.battle_gate_x && y == current_map.battle_gate_y) ||
        (x + 1 == current_map.battle_gate_x && y == current_map.battle_gate_y)) {
        return false;
    }

    
    return true;
}


void generate_map(bool keep_dimensions) { 
    init_map();
    current_map.room_count = 0;
    srand(time(NULL));

    // اگر نیاز به حفظ ابعاد اتاق قبلی داریم
    if(keep_dimensions && current_map.prev_room_width > 0) {
        Room first_room;
        first_room.width = current_map.prev_room_width;
        first_room.height = current_map.prev_room_height;
        first_room.x = 1 + rand() % (MAP_WIDTH - first_room.width - 2);
        first_room.y = 1 + rand() % (MAP_HEIGHT - first_room.height - 2);
        first_room.is_cursed = false;
        
        if(can_place_room(first_room)) {
            create_room(first_room);
        }
    }
    // تولید اتاق‌ها
    int attempts = 0;
    while (current_map.room_count < MIN_ROOMS && attempts < 1000) {
        Room room;
        room.width = ROOM_MIN_SIZE + rand() % (ROOM_MAX_SIZE - ROOM_MIN_SIZE);
        room.height = ROOM_MIN_SIZE + rand() % (ROOM_MAX_SIZE - ROOM_MIN_SIZE);
        room.x = 1 + rand() % (MAP_WIDTH - room.width - 2);
        room.y = 1 + rand() % (MAP_HEIGHT - room.height - 2);
        room.discovered = false;
        room.is_cursed = false;
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
    int cursed_room_added = 0;
    while (cursed_room_added == 0 && attempts < 1000) {
        Room cursed_room;
        cursed_room.width = 7; // اندازه ثابت برای اتاق طلسم
        cursed_room.height = 7;
        cursed_room.x = 1 + rand() % (MAP_WIDTH - cursed_room.width - 2);
        cursed_room.y = 1 + rand() % (MAP_HEIGHT - cursed_room.height - 2);
        cursed_room.is_cursed = true;
        cursed_room.discovered = false;

        if (can_place_room(cursed_room)) {
            create_room(cursed_room);
            current_map.cursed_room = cursed_room; // ذخیره اتاق طلسم
            cursed_room_added = 1;
        }
        attempts++;
    }

 // ایجاد آرایه برای الگوریتم MST
    int connections[MAX_ROOMS] = {0};
    int costs[MAX_ROOMS][MAX_ROOMS] = {0};

    // محاسبه هزینه اتصالات
    for (int i = 0; i < current_map.room_count; i++) {
        for (int j = i+1; j < current_map.room_count; j++) {
            int x1 = current_map.rooms[i].x + current_map.rooms[i].width/2;
            int y1 = current_map.rooms[i].y + current_map.rooms[i].height/2;
            int x2 = current_map.rooms[j].x + current_map.rooms[j].width/2;
            int y2 = current_map.rooms[j].y + current_map.rooms[j].height/2;
            
            costs[i][j] = abs(x1 - x2) + abs(y1 - y2);
        }
    }

    // الگوریتم Prim با راهروهای پیچیده
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
            Room prev = current_map.rooms[from];
            Room current = current_map.rooms[to];
            
            int x1 = prev.x + prev.width/2;
            int y1 = prev.y + prev.height/2;
            int x2 = current.x + current.width/2;
            int y2 = current.y + current.height/2;
            
            // ایجاد راهرو پیچدرپیچ
            create_winding_corridor(x1, y1, x2, y2);
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
    // قرار دادن تله
    int battle_room_index;
    do {
        battle_room_index = rand() % current_map.room_count;
    } while (battle_room_index == current_map.room_count-1); // اتاق پله نباشد

    Room battle_room = current_map.rooms[battle_room_index];
    current_map.battle_gate_x = battle_room.x + battle_room.width/2;
    current_map.battle_gate_y = battle_room.y + battle_room.height/2;
    current_map.tiles[current_map.battle_gate_y][current_map.battle_gate_x] = '^';

    int food_count = 0;
    int gold_count = 0;
    int white_gold_count = 0;

   while (food_count < 4 || gold_count < 3 || white_gold_count < 1) {
        int x = rand() % MAP_WIDTH;
        int y = rand() % MAP_HEIGHT;

        if (current_map.tiles[y][x] == '.') {
            if (food_count < 4) {
                current_map.tiles[y][x] = 'f';
                food_count++;
            } 
            else if (gold_count < 3) {
                current_map.tiles[y][x] = 'g';
                gold_count++;
            }
            else if (white_gold_count < 1) {
                current_map.tiles[y][x] = 'G';
                white_gold_count += 1;
            }
        }
    }

    int min_monsters = 4;
    int max_monsters = 6;
    int num_monsters = (rand() % (max_monsters - min_monsters + 1)) + min_monsters;
    // لیست اتاقهای مجاز برای قرارگیری هیولا (به جز اتاق پله و اتاق طلسم)
    int valid_rooms[current_map.room_count];
    int valid_room_count = 0;

    for (int i = 0; i < current_map.room_count; i++) {
        if (i != current_map.room_count-1 && 
            !(current_map.rooms[i].x == current_map.cursed_room.x && 
            current_map.rooms[i].y == current_map.cursed_room.y)) { 
            valid_rooms[valid_room_count++] = i;
        }
    }

    num_monsters = (num_monsters > valid_room_count) ? valid_room_count : num_monsters;

    // مخلوط کردن لیست اتاقها
    for (int i = 0; i < valid_room_count; i++) {
        int j = rand() % valid_room_count;
        int temp = valid_rooms[i];
        valid_rooms[i] = valid_rooms[j];
        valid_rooms[j] = temp;
    }

   current_map.monster_count = 0;

    char monster_chars[] = {'D', 'F', 'A', 'S', 'U'};
    for (int i = 0; i < num_monsters; i++) {
        if (i >= valid_room_count || current_map.monster_count >= MAX_MONSTERS) break;
        
        Room room = current_map.rooms[valid_rooms[i]];
        
        int attempts = 0;
        while (attempts < 100) {
            int mx = room.x + 1 + rand() % (room.width - 2);
            int my = room.y + 1 + rand() % (room.height - 2);
            
            if (current_map.tiles[my][mx] == '*') {
                char monster_type = monster_chars[rand() % 5];
                
                // ایجاد هیولا با ویژگی‌ها
                monster m;
                m.x = mx;
                m.y = my;
                m.type = monster_type;
                m.alive = true;
                
                
                // تنظیم ویژگی‌ها بر اساس نوع
                switch(monster_type) {
                    case 'D':
                        m.hp = 5;
                        m.power = 5;
                        m.follow_steps = 50;
                        break;
                    case 'F':
                        m.hp = 10;
                        m.power = 10;
                        m.follow_steps = 30;
                        break;
                    case 'A':
                        m.hp = 15;
                        m.power = 15;
                        m.follow_steps = 100;
                        break;
                    case 'S':
                        m.hp = 20;
                        m.power = 20;
                        m.follow_steps = 1000;
                        break;
                    case 'U':
                        m.hp = 30;
                        m.power = 30;
                        m.follow_steps = 100;
                        break;
                }
                
                current_map.monsters[current_map.monster_count++] = m;
                m.tile_beneath = current_map.tiles[my][mx];
                current_map.tiles[my][mx] = monster_type;
                break;
            }
            attempts++;
        }
    }

    int min_weapons = 4, max_weapons = 6;
    int num_weapons = rand() % (max_weapons - min_weapons + 1) + min_weapons;
    
    current_map.weapon_count = 0;
    
    for(int i=0; i<num_weapons; i++) {
        Weapon wp;
        int r = rand() % 4;
        switch(r) {
            case 0: // خنجر
                wp.type = 'd';
                wp.damage = 3;
                wp.throwable = true;
                wp.ammo = 1;
                wp.range = 10; 
                break;
            case 1: // چوب جادو
                wp.type = 'w';
                wp.damage = 4;
                wp.throwable = true;
                wp.ammo = 3;
                wp.range = 10; 
                break;
            case 2: // تیر معمولی
                wp.type = 'a';
                wp.damage = 2;
                wp.throwable = true;
                wp.ammo = 5;
                wp.range = 10; 
                break;
            case 3: // شمشیر
                wp.type = 's';
                wp.damage = 6;
                wp.throwable = false;
                wp.ammo = 1;
                wp.range = 10; 
                break;
        }
        

        int attempts = 0;
        do {
            Room room = current_map.rooms[rand() % current_map.room_count];
            wp.x = room.x + 1 + rand() % (room.width - 2);
            wp.y = room.y + 1 + rand() % (room.height - 2);
            attempts++;
        } while(!is_valid_weapon_position(wp.x, wp.y) && attempts < 100);

        if(attempts < 100) {
            current_map.weapons[current_map.weapon_count++] = wp;
            current_map.tiles[wp.y][wp.x] = wp.type; 
        }
    }

    int min_charms = 2, max_charms = 4;
    int num_charms = rand() % (max_charms - min_charms + 1) + min_charms;
    current_map.charm_count = 0;

    for(int i=0; i<num_charms; i++) {
            Charm ch;
            int r = rand() % 3;
            switch(r) {
                case 0: ch.type = '&'; break; 
                case 1: ch.type = '$'; break; 
                case 2: ch.type = '%'; break; 
            }
            
        int attempts = 0;
        do {
            Room room = current_map.rooms[rand() % current_map.room_count];
            ch.x = room.x + 1 + rand() % (room.width - 2);
            ch.y = room.y + 1 + rand() % (room.height - 2);
            attempts++;
        } while(!is_valid_weapon_position(ch.x, ch.y) && attempts < 100);

        if(attempts < 100) {
            current_map.charms[current_map.charm_count++] = ch;
            current_map.tiles[ch.y][ch.x] = ch.type;
        }
    }
}


void draw_map() {
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (current_map.discovered[y][x] || player.map_revealed) {
                char tile = current_map.tiles[y][x];
                bool is_cursed_wall = false;

                if (x >= current_map.cursed_room.x && 
                    x < current_map.cursed_room.x + current_map.cursed_room.width &&
                    y >= current_map.cursed_room.y && 
                    y < current_map.cursed_room.y + current_map.cursed_room.height) {
                    
                    is_cursed_wall = (x == current_map.cursed_room.x ||
                                     x == current_map.cursed_room.x + current_map.cursed_room.width - 1 ||
                                     y == current_map.cursed_room.y ||
                                     y == current_map.cursed_room.y + current_map.cursed_room.height - 1);
                }

                if (tile == '#') {
                    if (is_cursed_wall) {
                        attron(COLOR_PAIR(6)); 
                    } else {
                        attron(COLOR_PAIR(1)); 
                    }
                    mvaddch(y+2, x, '#');
                    attroff(COLOR_PAIR(3) | COLOR_PAIR(1));
                }
                // Handle other tiles
                else {
                   
                    if (tile == 'D' || tile == 'F' || tile == 'A' || tile == 'S' || tile == 'U') {
                        for (int m = 0; m < current_map.monster_count; m++) {
                            if (current_map.monsters[m].x == x && 
                                current_map.monsters[m].y == y &&
                                current_map.monsters[m].alive) {
                                attron(COLOR_PAIR(6)); // Monster color
                                mvaddch(y+2, x, current_map.monsters[m].type);
                                attroff(COLOR_PAIR(6));
                                break;
                            }
                        }
                    } else if (tile == '$'){
                        mvprintw(y+2,x,"\U000026A1");
                        x++;
                    } else if (tile == '&'){
                        mvprintw(y+2,x,"\U0001FA78");
                        x++;
                    } else if (tile == '%'){
                        mvprintw(y+2,x,"\U0001F4A5");
                        x++;
                    }
                    
                    else if (tile == 'd'){
                        mvprintw(y+2,x,"\U0001F5E1");
                        x++;
                    } else if (tile == 'w'){
                        mvprintw(y+2,x,"\u2728");
                        x++;
                    } else if (tile == 'a'){
                        mvprintw(y+2,x,"\U0001F3F9");
                        x++;
                    } else if (tile == 's'){
                        mvprintw(y+2,x,"\u2694");
                        x++;
                    }
                    // Handle gold, food, etc.
                    else if (tile == 'g') {
                        attron(COLOR_PAIR(3)); // Gold color
                        mvaddch(y+2, x, tile);
                        attroff(COLOR_PAIR(3));
                    } else if (tile == 'f') {
                        attron(COLOR_PAIR(3)); // Food color
                        mvaddch(y+2, x, tile);
                        attroff(COLOR_PAIR(3));
                    } else {
                        attron(COLOR_PAIR(1)); // Default color
                        mvaddch(y+2, x, tile);
                        attroff(COLOR_PAIR(1));
                    }
                }
            }
        }
    }
}

void display_message(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(last_message, sizeof(last_message), format, args);
    va_end(args);
    last_msg_time = time(NULL);
}

// ============Game_Play============
bool is_weapon_tile(char c) {
    return c == 'd' || c == 'w' || c == 'a' || c == 's';
}

Weapon get_weapon_by_char(char c) {
    for(int i = 0; i < (sizeof(weapons_list)/sizeof(Weapon)); i++)  {
        if(weapons_list[i].type == c) return weapons_list[i];
    }
    return default_weapon; 
}

void add_weapon_to_player(Player *p, Weapon w) {
    for(int i=0; i<p->weapon_count; i++) {
        if(p->weapons[i]->type == w.type) {
            p->weapons[i]->ammo += w.ammo;
            return;
        }
    }
    if(p->weapon_count < MAX_WEAPONS) {
        p->weapons[p->weapon_count] = malloc(sizeof(Weapon));
        *p->weapons[p->weapon_count] = w;
        p->weapon_count++;
    }
}

void add_charm_to_player(Player *p, char type) {
    switch(type) {
        case '&': p->health_charms++; break;
        case '$': p->speed_charms++; break;
        case '%': p->damage_charms++; break;
    }
}


void game_play() {
    init_pair(4, COLOR_CYAN, COLOR_BLACK); 
    init_pair(3, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(6, COLOR_RED, COLOR_BLACK);
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK); 
    init_pair(2, COLOR_RED, COLOR_BLACK);  
    init_pair(3, COLOR_GREEN, COLOR_BLACK); 

    generate_map(false);
    player.x = current_map.rooms[0].x + 1;
    player.y = current_map.rooms[0].y + 1;
    player.hp = 100;
    player.gold = 0;
    player.keys = 0;
    player.food = 0;
    player.hunger = 100;
    player.current_weapon = &default_weapon;
    player.weapon_count = 0;
    player.last_heal_time = time(NULL);
    player.map_revealed = false;
    bool backpack_open = false;
    bool speed_mode = false;
    bool velocity_mode = false;
    bool pick = false;
    WINDOW *backpack_win;

    int map_width = MAP_WIDTH;
    int map_height = MAP_HEIGHT;

    int win_height = 40;
    int win_width = 70;
    int start_y = 2;
    int start_x = map_width + 5; 
    int map_display_height = MAP_HEIGHT + 2;
    backpack_win = newwin(win_height, win_width, start_y, start_x);
    int ch;
   while ((ch = getch()) != 'q') {
            wclear(backpack_win);
            if (backpack_open) {
                switch(ch) {
                    case '\t': 
                        backpack_open = false;
                        clear();
                        break;
                    case 'f': 
                        if (player.food > 0) {
                            player.food--;
                            player.hunger += 30;
                            if (player.hunger > 100)
                            player.hunger = 100;
                        }
                        break;
                    default:
                        if (isalpha(ch)) {
                            char key = tolower(ch);

                            if (player.current_weapon) {

                                if (player.current_weapon->type == key) {
                                    add_weapon_to_player(&player, *player.current_weapon);
                                    player.current_weapon = NULL;
                                } else {
                                    display_message("Can't use two weapons at the same time!");
                                }
                            }
                       
                            else {
                                bool found = false;
                                for (int i = 0; i < player.weapon_count; i++) {
                                    if (player.weapons[i]->type == key) {
                                        if (player.weapons[i]->ammo < 1) {
                                            display_message("The ammo has been finished!!!");
                                            found = true;
                                            break;
                                        }
                                        player.current_weapon = player.weapons[i];
                                        found = true;
                                        for (int j = i; j < player.weapon_count - 1; j++) {
                                            player.weapons[j] = player.weapons[j + 1];
                                        }
                                        player.weapon_count--;
                                        break;
                                    }
                                }
                                if (!found) {
                                    display_message("The weapon doesn't exist!!!");
                                }
                            }
                        }
                        break;
                }
            }

        else {
            if (ch == '\t'){
                backpack_open = true;
            }
        }
        int dx = 0, dy = 0;
        bool is_main_direction = false;

       switch(ch) {
            case '8': dy = -1; break;
            case '5': dy = 1; break;
            case '4': dx = -1; break;
            case '6': dx = 1; break;
            case '7': dx = -1; dy = -1; break;
            case '9': dx = 1; dy = -1; break;
            case '1': dx = -1; dy = 1; break;
            case '3': dx = 1; dy = 1; break;
            case 'M': player.map_revealed = !player.map_revealed; clear(); break;
            case 'S': 
                speed_mode = !speed_mode;
                velocity_mode = false;
                break;
            case 'V': 
                velocity_mode = !velocity_mode;
                speed_mode = false;
                break;
            case 'P': pick = !pick; break;
            case ' ': 
            if (player.current_weapon && player.current_weapon->throwable) {
                
                attron(COLOR_PAIR(3));
                mvprintw(30, 0, "Select direction (w/a/s/d): ");
                refresh();
                int dir = getch();
                
                int direction;
                switch(dir) {
                    case 'w': direction = 0; break; // بالا
                    case 's': direction = 1; break; // پایین
                    case 'a': direction = 2; break; // چپ
                    case 'd': direction = 3; break; // راست
                    default: 
                        mvprintw(30, 0, "Invalid direction!          ");
                        continue;
                }
                
    
                throw_weapon(player.current_weapon, direction);
                mvprintw(30, 0, "                             ");
            } else {
                attron(COLOR_PAIR(2));
                mvprintw(30, 0, "No throwable weapon equipped!");
            }
            break;
        }

        bool in_cursed_room = (player.x >= current_map.cursed_room.x &&
                              player.x < current_map.cursed_room.x + current_map.cursed_room.width &&
                              player.y >= current_map.cursed_room.y &&
                              player.y < current_map.cursed_room.y + current_map.cursed_room.height);

        if (in_cursed_room) {
            player.hp -= 2;
            if (player.hp < 0) player.hp = 0;
            if(current_map.tiles[player.y][player.x] == '^') player.hp -= 10;
        }

        is_main_direction = ( (dx != 0 && dy == 0) || (dy != 0 && dx == 0) );

        if ((speed_mode || velocity_mode) && is_main_direction) {
            while (1) {
                int next_x = player.x + dx;
                int next_y = player.y + dy;

                if (next_x < 0 || next_x >= MAP_WIDTH || next_y < 0 || next_y >= MAP_HEIGHT) break;

                char tile = current_map.tiles[next_y][next_x];
                bool blocked = false;

                if (speed_mode) {
                    blocked = (tile == '#' || (next_x == current_map.stairs_x && next_y == current_map.stairs_y));
                } else if (velocity_mode) {
                    blocked = (tile == '#' ||  tile == 'g' || tile == 'f' || (next_x == current_map.stairs_x && next_y == current_map.stairs_y));
                }

                if (blocked) break;

                player.x = next_x;
                player.y = next_y;
                current_map.discovered[player.y][player.x] = true;

                if (current_map.tiles[player.y][player.x] == '.') {
                    for (int step = 1; step <= 3; step++) {
                        int next_step_x = player.x + (dx * step);
                        int next_step_y = player.y + (dy * step);
                        if (next_step_x >= 0 && next_step_x < MAP_WIDTH && 
                            next_step_y >= 0 && next_step_y < MAP_HEIGHT &&
                            current_map.tiles[next_step_y][next_step_x] == '.') {
                            current_map.discovered[next_step_y][next_step_x] = true;
                        } else break;
                    }
                }

                if (current_map.tiles[player.y][player.x] == 'g' && pick) {
                    display_message("                        ");
                    int message_line = map_display_height + 2;
                    mvprintw(message_line, 0, "> %s", last_message);
                    refresh();
                    display_message("Gold!!!");
                    player.gold++;
                    current_map.tiles[player.y][player.x] = '.';
                }

                if (current_map.tiles[player.y][player.x] == 'G' && pick) {
                    display_message("                        ");
                    int message_line = map_display_height + 2;
                    mvprintw(message_line, 0, "> %s", last_message);
                    refresh();
                    display_message("White gold!!!");
                    player.gold += 5;
                    current_map.tiles[player.y][player.x] = '.';
                }

                if (current_map.tiles[player.y][player.x] == 'f' && pick) {
                    display_message("                        ");
                    int message_line = map_display_height + 2;
                    mvprintw(message_line, 0, "> %s", last_message);
                    refresh();
                    display_message("Food!!!");
                    if (player.food < 5) {
                        player.food++;
                        current_map.tiles[player.y][player.x] = '.';
                    }
                }

                if (player.x == current_map.stairs_x && player.y == current_map.stairs_y) {
                    display_message("                        ");
                    int message_line = map_display_height + 2;
                    mvprintw(message_line, 0, "> %s", last_message);
                    refresh();
                    display_message("New floor!");
                    Room current_room;
                    for (int i = 0; i < current_map.room_count; i++) {
                        Room r = current_map.rooms[i];
                        if (player.x >= r.x && player.x < r.x + r.width &&
                            player.y >= r.y && player.y < r.y + r.height) {
                            current_room = r;
                            break;
                        }
                    }

                    current_map.prev_room_width = current_room.width;
                    current_map.prev_room_height = current_room.height;
                    generate_map(true);
                    Room new_room = current_map.rooms[0];
                    float rel_x = (float)(player.x - current_room.x) / current_room.width;
                    float rel_y = (float)(player.y - current_room.y) / current_room.height;
                    player.x = new_room.x + (int)(rel_x * new_room.width);
                    player.y = new_room.y + (int)(rel_y * new_room.height);
                    player.x = (player.x < new_room.x + 1) ? new_room.x + 1 : (player.x > new_room.x + new_room.width - 2) ? new_room.x + new_room.width - 2 : player.x;
                    player.y = (player.y < new_room.y + 1) ? new_room.y + 1 : (player.y > new_room.y + new_room.height - 2) ? new_room.y + new_room.height - 2 : player.y;
                    break;
                }

                if (player.x == current_map.battle_gate_x && player.y == current_map.battle_gate_y) {
                    player.hp -= 10;
                     current_map.battle_gate_x = '*';
                     current_map.battle_gate_y = '*';
                     refresh();
                }

                player.hunger--;
                if (player.hunger <= 0) {
                    player.hp -= 2;
                    player.hunger = 0;
                }
            }
        } else {
            int new_x = player.x + dx;
            int new_y = player.y + dy;

            if (new_x >= 0 && new_x < MAP_WIDTH && new_y >= 0 && new_y < MAP_HEIGHT) {
                if (current_map.tiles[new_y][new_x] != '#') {
                    player.x = new_x;
                    player.y = new_y;
                    current_map.discovered[new_y][new_x] = true;

                    if (player.x == current_map.stairs_x && player.y == current_map.stairs_y) {
                        display_message("                        ");
                        int message_line = map_display_height + 2;
                        mvprintw(message_line, 0, "> %s", last_message);
                        refresh();
                        display_message("New floor!");
                        Room current_room;
                        for (int i = 0; i < current_map.room_count; i++) {
                            Room r = current_map.rooms[i];
                            if (player.x >= r.x && player.x < r.x + r.width &&
                               player.y >= r.y && player.y < r.y + r.height) {
                                    current_room = r;
                                    break;
                            }
                         }
                        current_map.prev_room_width = current_room.width;
                        current_map.prev_room_height = current_room.height;
                        generate_map(true);
                        clear();
                        Room new_room = current_map.rooms[0];
                        float rel_x = (float)(player.x - current_room.x) / current_room.width;
                        float rel_y = (float)(player.y - current_room.y) / current_room.height;
                        player.x = new_room.x + (int)(rel_x * new_room.width);
                        player.y = new_room.y + (int)(rel_y * new_room.height);
                        player.x = (player.x < new_room.x + 1) ? new_room.x + 1 : (player.x > new_room.x + new_room.width - 2) ? new_room.x + new_room.width - 2 : player.x;
                        player.y = (player.y < new_room.y + 1) ? new_room.y + 1 : (player.y > new_room.y + new_room.height - 2) ? new_room.y + new_room.height - 2 : player.y;
                    }

                    // چک دروازه نبرد در حرکت عادی
                    if (player.x == current_map.battle_gate_x && player.y == current_map.battle_gate_y) {
                        player.hp -= 10;
                        current_map.battle_gate_x = '*';
                        current_map.battle_gate_y = '*';
                    }

                    for (int i = 0; i < current_map.thrown_weapon_count; i++) {
                        Weapon *wp = &current_map.thrown_weapons[i];
                        if (wp->x == new_x && wp->y == new_y) {
                            add_weapon_to_player(&player,*wp);
                            current_map.tiles[new_y][new_x] = '.';
                            current_map.thrown_weapon_count--;
                            current_map.thrown_weapons[i] = current_map.thrown_weapons[current_map.thrown_weapon_count];
                            break;
                        }
                    }

                    if (current_map.tiles[new_y][new_x] == '.') {
                        for (int step = 1; step <= 3; step++) {
                            int next_step_x = player.x + (dx * step);
                            int next_step_y = player.y + (dy * step);
                            if (next_step_x >= 0 && next_step_x < MAP_WIDTH && 
                                next_step_y >= 0 && next_step_y < MAP_HEIGHT &&
                                current_map.tiles[next_step_y][next_step_x] == '.') {
                                current_map.discovered[next_step_y][next_step_x] = true;
                            } else break;
                        }
                    }

                    if (is_weapon_tile(current_map.tiles[new_y][new_x]) && !pick){
                        clear();
                    }

                    if (is_weapon_tile(current_map.tiles[new_y][new_x]) && pick) {
                        char weapon_char = current_map.tiles[new_y][new_x];
                        Weapon w = get_weapon_by_char(weapon_char);
                        add_weapon_to_player(&player, w);
                        current_map.tiles[new_y][new_x] = '*';
                        display_message("                        ");
                        int message_line = map_display_height + 2;
                        mvprintw(message_line, 0, "> %s", last_message);
                        refresh();
                        display_message("Weapon picked up!");
                        clear();
                    }

                    if (current_map.tiles[new_y][new_x] == 'g' && pick) {
                        display_message("                        ");
                        int message_line = map_display_height + 2;
                        mvprintw(message_line, 0, "> %s", last_message);
                        refresh();
                        display_message("Gold!!!");
                        player.gold++;
                        current_map.tiles[new_y][new_x] = '.';
                    }

                     if ((current_map.tiles[new_y][new_x] == '&' || 
                        current_map.tiles[new_y][new_x] == '$' || 
                        current_map.tiles[new_y][new_x] == '%') && pick) {
                        
                        for(int i=0; i<current_map.charm_count; i++) {
                            if(current_map.charms[i].x == new_x && current_map.charms[i].y == new_y) {
                                add_charm_to_player(&player, current_map.charms[i].type);
                                current_map.tiles[new_y][new_x] = '*';
                                current_map.charms[i] = current_map.charms[current_map.charm_count - 1];
                                current_map.charm_count--;
                                break;
                            }
                        }
                    }

                    if (current_map.tiles[player.y][player.x] == 'G' && pick) {
                    display_message("                        ");
                    int message_line = map_display_height + 2;
                    mvprintw(message_line, 0, "> %s", last_message);
                    refresh();
                    display_message("White gold!!!");
                    display_message("White gold!!!");
                    player.gold += 5;
                    current_map.tiles[player.y][player.x] = '.';
                    }

                    if (current_map.tiles[new_y][new_x] == 'f' && pick) {
                        display_message("                        ");
                        int message_line = map_display_height + 2;
                        mvprintw(message_line, 0, "> %s", last_message);
                        refresh();
                        display_message("Food!!!");
                        if (player.food < 5) {
                            player.food++;
                            current_map.tiles[new_y][new_x] = '.';
                        }
                    }

                    if (dx != 0 || dy != 0) {
                        player.hunger--;
                        if (player.hunger <= 0) {
                            player.hp -= 2;
                            player.hunger = 0;
                        }
                    }
                }
            }
        }
        move_monsters();
        if (hited){
            display_message("You got hit by monster");
            hited = false;
        }
        draw_map();
        mvprintw(0, 0, "Level:1  HP:%d  Hunger:%d%%  Gold:%d  Keys:%d", 
                player.hp, player.hunger, player.gold, player.keys);
        mvprintw(1, 0, "Press 'm' to toggle map | TAB for backpack | 'q' to quit");
       
       
        if (a_monster_has_been_killed){
        clear();
        display_message("You killed the monster!!!");
        a_monster_has_been_killed = false;
        }

        int message_line = map_display_height + 2;
        mvprintw(message_line, 0, "> %s", last_message);
        
        if(time(NULL) - last_msg_time > 2) {
            display_message("                                                                            ");
            strcpy(last_message, "                                                                       ");
            last_msg_time = time(NULL);
            refresh();
        }


        if (player.hunger >= 40 && (time(NULL) - player.last_heal_time) >= 5) {
            player.hp += 5;
            if (player.hp > 100) {
                player.hp = 100;
            }
            player.last_heal_time = time(NULL);
        }

        if(backpack_open) {
            box(backpack_win, 0, 0);
            mvwprintw(backpack_win, 1, 2, "[ Backpack ]");
            mvwprintw(backpack_win, 3, 2, "Food: %d", player.food);
            mvwprintw(backpack_win, 5, 2, "Press F to eat");
            mvwprintw(backpack_win, 6, 2, "1 food = +30%% hunger");
            mvwprintw(backpack_win, 8, 2, "Weapons:");
            if (player.current_weapon)
            mvwprintw(backpack_win, 9, 2, "Equipped: %c: %d damage, %d ammo, %d range", 
            player.current_weapon->type, player.current_weapon->damage,player.current_weapon->ammo,player.current_weapon->range);
            mvwprintw(backpack_win, 10, 2, "Throwables:");
            int j = 0;
            for(int i = 0; i < player.weapon_count; i++) {
                Weapon *w = player.weapons[i];
                if (w->throwable){
                    mvwprintw(backpack_win, 11 + j, 2, 
                        "%c: %d damage, %d ammo, %d range", 
                        w->type, w->damage, w->ammo, w->range
                    ); j++;
                }
            }
            mvwprintw(backpack_win, 11 + j, 2, "Non-Thrown:");
             for(int i = 0; i < player.weapon_count; i++) {
                Weapon *w = player.weapons[i];
                if (!w->throwable){
                mvwprintw(backpack_win, 12 + j, 2, 
                    "%c: %d damage, %d ammo, %d range", 
                    w->type, w->damage, w->ammo, w->range
                ); j++;
                }
            }
            int charms_line = 12 + j; 
            mvwprintw(backpack_win, charms_line, 2, "Charms:");
            mvwprintw(backpack_win, charms_line + 1, 2, "Health: %d", player.health_charms);
            mvwprintw(backpack_win, charms_line + 2, 2, "Speed: %d", player.speed_charms);
            mvwprintw(backpack_win, charms_line + 3, 2, "Damage: %d", player.damage_charms);
            wrefresh(backpack_win);
        }

        attron(COLOR_PAIR(3));
        mvaddch(player.y + 2, player.x, '@');
        attroff(COLOR_PAIR(3));

        discover_current_room(player.x, player.y);
        refresh();
    }
    delwin(backpack_win);
    endwin();
}

Room* find_room_for_monster(int x, int y) {
    for (int i = 0; i < current_map.room_count; i++) {
        Room *room = &current_map.rooms[i];
        if (x >= room->x && x < room->x + room->width &&
            y >= room->y && y < room->y + room->height) {
            return room;
        }
    }
    return NULL;
}

void move_monsters() {
    Room *player_room = find_room_for_monster(player.x, player.y);
    if (!player_room) {
        return; // بازیکن در راهرو است، هیولاها حرکت نمی‌کنند
    }

    for (int i = 0; i < current_map.monster_count; i++) {
        monster *m = &current_map.monsters[i];
        if (!m->alive || m->follow_steps <= 0) {
            continue;
        }

        Room *monster_room = find_room_for_monster(m->x, m->y);
        if (monster_room != player_room) {
            continue; // هیولا در اتاق دیگری است، حرکت نمی‌کند
        }

        int best_dx = 0, best_dy = 0;
        int min_distance = abs(m->x - player.x) + abs(m->y - player.y);

        int directions[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};

        for (int j = 0; j < 4; j++) {
            int dx = directions[j][0];
            int dy = directions[j][1];
            int new_x = m->x + dx;
            int new_y = m->y + dy;

            // بررسی محدوده اتاق
            if (new_x < monster_room->x || new_x >= monster_room->x + monster_room->width ||
                new_y < monster_room->y || new_y >= monster_room->y + monster_room->height) {
                continue; // خارج از اتاق
            }

            // بررسی نوع سلول مقصد
            char tile = current_map.tiles[new_y][new_x];
            if (tile != '*' && tile != '.') {
                continue; // غیر قابل عبور
            }

            // بررسی وجود هیولای دیگر در مقصد
            bool occupied = false;
            for (int k = 0; k < current_map.monster_count; k++) {
                if (k != i && current_map.monsters[k].alive &&
                    current_map.monsters[k].x == new_x && current_map.monsters[k].y == new_y) {
                    occupied = true;
                    break;
                }
            }
            if (occupied) {
                continue;
            }

            // بررسی فاصله جدید
            int new_distance = abs(new_x - player.x) + abs(new_y - player.y);
            if (new_distance < min_distance) {
                min_distance = new_distance;
                best_dx = dx;
                best_dy = dy;
            }
        }

        if (best_dx != 0 || best_dy != 0) {
            int new_x = m->x + best_dx;
            int new_y = m->y + best_dy;

            // برخورد با بازیکن
            if (new_x == player.x && new_y == player.y) {
                player.hp -= m->power;
                hited = true;
                if (player.current_weapon->range == 0)
                m->hp -= player.current_weapon->damage;
                if (m-> hp <= 0){
                m->alive = false;
                a_monster_has_been_killed = true;
                }
                if (!m->alive)
                current_map.tiles[m->y][m->x] = '*';
            } else {
                // حرکت هیولا
                current_map.tiles[m->y][m->x] = m->tile_beneath;
                m->x = new_x;
                m->y = new_y;
                m->tile_beneath = current_map.tiles[m->y][m->x];
                current_map.tiles[m->y][m->x] = m->type;
            }

            m->follow_steps--;
        }
    }
}

void throw_weapon(Weapon *wp, int direction) {
    if (wp->ammo <= 0 || wp->range == 0) return;

    int dx = 0, dy = 0;
    switch(direction) {
        case 0: dy = -1; break; // بالا
        case 1: dy = 1; break;  // پایین
        case 2: dx = -1; break; // چپ
        case 3: dx = 1; break;  // راست
        default: return;
    }

    int x = player.x;
    int y = player.y;
    int steps = 0;

    while (steps < wp->range) {
        x += dx;
        y += dy;


        if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) break;

  
        if (current_map.tiles[y][x] == '#') break;

        // بررسی برخورد با هیولا
        bool hit_monster = false;
        for (int i = 0; i < current_map.monster_count; i++) {
            monster *m = &current_map.monsters[i];
            if (m->alive && m->x == x && m->y == y) {
                m->hp -= wp->damage;
                if (m->hp <= 0) {
                    m->alive = false;
                    a_monster_has_been_killed = true;
                    current_map.tiles[y][x] = m->tile_beneath;
                }
                hit_monster = true;
                break;
            }
        }
        if (hit_monster) {
            int map_display_height = MAP_HEIGHT + 2;
            display_message("You hit monster by a throwable weapon!!!");
            int message_line = map_display_height + 2;
            mvprintw(message_line, 0, "> %s", last_message);
            wp->ammo--;
            return;
        }

        steps++;
    }


    if (steps < wp->range) {
        x -= dx;
        y -= dy;
    }

    if (current_map.tiles[y][x] == '.' || current_map.tiles[y][x] == '*') {
        Weapon thrown = *wp;
        thrown.x = x;
        thrown.y = y;
        thrown.ammo = 1;
        current_map.thrown_weapons[current_map.thrown_weapon_count++] = thrown;
        current_map.tiles[y][x] = thrown.type;
    }

    wp->ammo--;
    if (wp->ammo <= 0) {
        player.current_weapon = NULL;
    }
}


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

void create_winding_corridor(int x1, int y1, int x2, int y2) {
        int current_x = x1;
        int current_y = y1;
        int steps_since_turn = 0;
        
        while (current_x != x2 || current_y != y2) {
            // 60% احتمال حرکت به سمت هدف پس از 3 قدم مستقیم
            if ((rand() % 100 < 60 || steps_since_turn > 3) && 
                (current_x != x2 || current_y != y2)) {
                
                if (rand() % 2 == 0 && current_x != x2) {
                    current_x += (current_x < x2) ? 1 : -1;
                } else if (current_y != y2) {
                    current_y += (current_y < y2) ? 1 : -1;
                }
                steps_since_turn++;
            } else {
                // تغییر جهت تصادفی
                int dir = rand() % 4;
                switch(dir) {
                    case 0: if (current_x < MAP_WIDTH-2) current_x++; break;
                    case 1: if (current_x > 1) current_x--; break;
                    case 2: if (current_y < MAP_HEIGHT-2) current_y++; break;
                    case 3: if (current_y > 1) current_y--; break;
                }
                steps_since_turn = 0;
            }
            
            // ایجاد راهرو با دیوارهای جانبی
            if (current_map.tiles[current_y][current_x] == '#') {
                current_map.tiles[current_y][current_x] = '.';
                add_corridor_walls(current_x, current_y);
            }
            
            // ایجاد انشعابات تصادفی
            if (rand() % 100 < 5) {
                create_random_branch(current_x, current_y);
            }
        }
    }

    void add_corridor_walls(int x, int y) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (x+dx >= 0 && x+dx < MAP_WIDTH &&
                    y+dy >= 0 && y+dy < MAP_HEIGHT &&
                    current_map.tiles[y+dy][x+dx] == '#') {
                    current_map.tiles[y+dy][x+dx] = '#';
                }
            }
        }
    }

    void create_random_branch(int x, int y) {
        int length = 3 + rand() % 5;
        int dir = rand() % 4;
        int dx[] = {0, 0, -1, 1};
        int dy[] = {-1, 1, 0, 0};
        
        for (int i = 0; i < length; i++) {
            x += dx[dir];
            y += dy[dir];
            
            if (x < 1 || x >= MAP_WIDTH-1 || y < 1 || y >= MAP_HEIGHT-1) break;
            
            if (current_map.tiles[y][x] == '#') {
                current_map.tiles[y][x] = '.';
                add_corridor_walls(x, y);
            }
        }
    }
