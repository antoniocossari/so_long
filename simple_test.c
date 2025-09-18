#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/time.h>
#include <math.h>
#include <string.h>

#define MAP_WIDTH 20
#define MAP_HEIGHT 15
#define START_SOLDIERS 30

typedef struct {
    int x, y;
} t_pos;

typedef struct {
    char operation;
    int value;
    int collected;
} t_math_gate;

typedef struct {
    int x, y;
    int type;
    int hp;
    int active;
} t_enemy;

typedef struct {
    char map[MAP_HEIGHT][MAP_WIDTH + 1];
    t_pos player;
    int army_count;
    t_math_gate gates[10];
    t_enemy enemies[5];
    int collectibles_total;
    int collectibles_taken;
    int wall_level;
    int moves;
    int game_over;
    int victory;
} t_game;

long get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

void clear_screen(void) {
    printf("\033[2J\033[H");
}

void init_game(t_game *game) {
    // Mappa di test
    char test_map[MAP_HEIGHT][MAP_WIDTH + 1] = {
        "11111111111111111111",
        "1000000000000000001",
        "100C00000C000C0001",
        "1000000000000000001",
        "1000000000000000001",
        "100C00000C000C0001",
        "1000000000000000001",
        "1000000000000000001",
        "100C00000C000C0001",
        "1000000000000000001",
        "10000000E000000001",
        "1000000000000000001",
        "1000000000000000001",
        "1000000P000000001",
        "11111111111111111111"
    };

    for (int y = 0; y < MAP_HEIGHT; y++) {
        strcpy(game->map[y], test_map[y]);
    }

    // Trova player e setup
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (game->map[y][x] == 'P') {
                game->player.x = x;
                game->player.y = y;
                game->map[y][x] = '0';
            }
        }
    }

    game->army_count = START_SOLDIERS;
    game->collectibles_total = 9; // Conta le C nella mappa
    game->collectibles_taken = 0;
    game->wall_level = MAP_HEIGHT - 1;
    game->moves = 0;
    game->game_over = 0;
    game->victory = 0;

    // Setup porte matematiche
    int gate_idx = 0;
    for (int i = 0; i < 9; i++) {
        if (i % 3 == 0) {
            game->gates[i].operation = '+';
            game->gates[i].value = 20;
        } else if (i % 3 == 1) {
            game->gates[i].operation = '*';
            game->gates[i].value = 2;
        } else {
            game->gates[i].operation = '-';
            game->gates[i].value = 10;
        }
        game->gates[i].collected = 0;
    }

    // Setup nemici semplici
    for (int i = 0; i < 5; i++) {
        game->enemies[i].active = 0;
    }
}

void handle_collectible(t_game *game, int x, int y) {
    int gate_idx = 0;
    for (int cy = 0; cy < MAP_HEIGHT; cy++) {
        for (int cx = 0; cx < MAP_WIDTH; cx++) {
            if (game->map[cy][cx] == 'C') {
                if (cx == x && cy == y) {
                    t_math_gate *gate = &game->gates[gate_idx];
                    if (!gate->collected) {
                        int old_army = game->army_count;

                        if (gate->operation == '+')
                            game->army_count += gate->value;
                        else if (gate->operation == '*')
                            game->army_count *= gate->value;
                        else if (gate->operation == '-') {
                            game->army_count -= gate->value;
                            if (game->army_count < 0) game->army_count = 0;
                        }

                        printf("Army: %d %c%d = %d\n", old_army, gate->operation, gate->value, game->army_count);

                        gate->collected = 1;
                        game->map[y][x] = '0';
                        game->collectibles_taken++;

                        if (game->army_count <= 0) {
                            game->game_over = 1;
                        }
                    }
                    return;
                }
                gate_idx++;
            }
        }
    }
}

void update_wall(t_game *game) {
    static long last_rise = 0;
    long current_time = get_time_ms();

    if (current_time - last_rise >= 3000) { // Più lento per il test
        if (game->wall_level > 0) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                game->map[game->wall_level][x] = '1';
            }
            game->wall_level--;
            printf("Wall rising! Level: %d\n", game->wall_level);
        }
        last_rise = current_time;
    }
}

void auto_step(t_game *game) {
    static long last_step = 0;
    long current_time = get_time_ms();

    if (current_time - last_step >= 1500) { // Auto-step ogni 1.5 secondi
        if (game->player.y > 0 && game->map[game->player.y - 1][game->player.x] != '1' &&
            game->player.y - 1 > game->wall_level) {
            game->player.y--;
            game->moves++;
            printf("Auto-step! Moves: %d\n", game->moves);

            if (game->map[game->player.y][game->player.x] == 'C')
                handle_collectible(game, game->player.x, game->player.y);
            else if (game->map[game->player.y][game->player.x] == 'E' &&
                     game->collectibles_taken == game->collectibles_total) {
                game->victory = 1;
                printf("\n=== VICTORY! You've reached the exit! ===\n");
            }
        }
        last_step = current_time;
    }
}

void render_game(t_game *game) {
    clear_screen();

    printf("=== SO LONG RUNNER TEST ===\n");
    printf("Army: %d | Moves: %d | Gates: %d/%d | Wall Level: %d\n\n",
           game->army_count, game->moves, game->collectibles_taken,
           game->collectibles_total, game->wall_level);

    // Render mappa
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (x == game->player.x && y == game->player.y) {
                printf("P");
            } else if (y <= game->wall_level && game->map[y][x] == '1') {
                printf("#"); // Muro che sale
            } else {
                printf("%c", game->map[y][x]);
            }
        }
        printf("\n");
    }

    printf("\nControls: WASD = Move, Q = Quit\n");

    if (game->player.y <= game->wall_level) {
        game->game_over = 1;
        printf("\n=== GAME OVER! Wall caught you! ===\n");
    }

    if (game->game_over) {
        printf("\n=== GAME OVER ===\n");
    } else if (game->victory) {
        printf("\n=== VICTORY! ===\n");
    }
}

char get_input(void) {
    struct termios old_tio, new_tio;
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;
    new_tio.c_lflag &= (~ICANON & ~ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    char c = getchar();
    fcntl(STDIN_FILENO, F_SETFL, 0);

    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
    return c;
}

int main(void) {
    t_game game;
    init_game(&game);

    printf("Starting So Long Runner Test!\n");
    printf("Watch the wall rise and try to reach the exit 'E'!\n");
    printf("Collect gates 'C' to modify your army with math operations!\n");
    printf("Press any key to continue...\n");
    getchar();

    while (!game.game_over && !game.victory) {
        auto_step(&game);
        update_wall(&game);
        render_game(&game);

        char input = get_input();
        if (input != (char)-1) {
            t_pos new_pos = game.player;

            if (input == 'a' && new_pos.x > 0) new_pos.x--;
            else if (input == 'd' && new_pos.x < MAP_WIDTH - 1) new_pos.x++;
            else if (input == 'w' && new_pos.y > 0) new_pos.y--;
            else if (input == 's' && new_pos.y < MAP_HEIGHT - 1) new_pos.y++;
            else if (input == 'q') break;

            if (game.map[new_pos.y][new_pos.x] != '1' && new_pos.y > game.wall_level) {
                game.player = new_pos;
                game.moves++;

                if (game.map[new_pos.y][new_pos.x] == 'C')
                    handle_collectible(&game, new_pos.x, new_pos.y);
                else if (game.map[new_pos.y][new_pos.x] == 'E' &&
                         game.collectibles_taken == game.collectibles_total) {
                    game.victory = 1;
                }
            }
        }

        usleep(100000); // 100ms
    }

    printf("\nGame finished! Final army size: %d\n", game.army_count);
    printf("Total moves: %d\n", game.moves);

    return 0;
}