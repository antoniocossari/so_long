#ifndef SO_LONG_RUNNER_H
# define SO_LONG_RUNNER_H

# include "MLX42/include/MLX42/MLX42.h"
# include <stdlib.h>
# include <unistd.h>
# include <fcntl.h>
# include <stdio.h>
# include <string.h>
# include <sys/time.h>
# include <math.h>

# define WIN_WIDTH 1000
# define WIN_HEIGHT 800
# define TILE_SIZE 40

# define AUTO_STEP_MS 500
# define RISE_MS 1200
# define SHOT_COOLDOWN_MS 200
# define START_SOLDIERS 30
# define MAX_PROJECTILES 100
# define MAX_ENEMIES 50
# define MAX_SOLDIERS_DISPLAY 50

# define KEY_ESC MLX_KEY_ESCAPE
# define KEY_W MLX_KEY_W
# define KEY_A MLX_KEY_A
# define KEY_S MLX_KEY_S
# define KEY_D MLX_KEY_D
# define KEY_SPACE MLX_KEY_SPACE

typedef struct s_pos {
    int x;
    int y;
} t_pos;

typedef struct s_math_gate {
    char operation;
    int value;
    int collected;
} t_math_gate;

typedef struct s_projectile {
    float x;
    float y;
    float dx;
    float dy;
    int active;
} t_projectile;

typedef struct s_enemy {
    float x;
    float y;
    float dx;
    float dy;
    int hp;
    int type;
    int active;
    int move_timer;
} t_enemy;

typedef struct s_soldier {
    float x;
    float y;
    float target_x;
    float target_y;
    int slot;
} t_soldier;

typedef struct s_game {
    mlx_t *mlx;
    mlx_image_t *img;
    char **map;
    int map_width;
    int map_height;
    t_pos player;
    int army_count;
    t_soldier soldiers[MAX_SOLDIERS_DISPLAY];
    t_math_gate *math_gates;
    int collectibles_total;
    int collectibles_taken;
    int moves;
    long last_auto_step;
    long last_rise;
    long last_shot;
    int wall_level;
    t_projectile projectiles[MAX_PROJECTILES];
    t_enemy enemies[MAX_ENEMIES];
    int game_over;
    int victory;
} t_game;

long get_time_ms(void);
void key_press(mlx_key_data_t keydata, void *param);
void close_game(void *param);
void game_loop(void *param);

void init_game(t_game *game, char *map_file);
void load_map(t_game *game, char *filename);
void setup_math_gates(t_game *game);
void setup_enemies(t_game *game);

void update_player(t_game *game, long current_time);
void update_army(t_game *game);
void update_projectiles(t_game *game);
void update_enemies(t_game *game);
void update_wall(t_game *game, long current_time);

void handle_collectible(t_game *game, int x, int y);
void shoot_projectiles(t_game *game);
void check_collisions(t_game *game);

void render_game(t_game *game);
void render_map(t_game *game);
void render_player(t_game *game);
void render_army(t_game *game);
void render_projectiles(t_game *game);
void render_enemies(t_game *game);
void render_hud(t_game *game);

void free_game(t_game *game);

char *get_next_line(int fd);

#endif